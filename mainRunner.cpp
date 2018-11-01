/*
 * Final Project
 * Hammad Ajmal
 * Neil Patel
 */

#include <msp430.h>
#include <string.h> //For mem copy
#include "MMC.h"
#include "hal_SPI.h"
#include "hal_MMC_hardware_board.h"
#include "ringbuffer.h" //Ring buffer class.
//Red onboard LED on if sd card is successfully read.
int i = 0;//index for block array.
int j = 0;//index for looping
volatile unsigned long int pointerToWord(unsigned char* p) {
                return ((unsigned long int)p[0]) << 24 |
                        ((unsigned long int)p[1]) << 16 |
                        ((unsigned long int)p[2]) << 8 |
                        ((unsigned long int)p[3]);
}
RingBuffer RB; //Buffer to get values to play.
const unsigned char LED = BIT0;
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TA1CCR0_INT(void) {

	TA1CCR2 = (RB.pop()+256); //pop next value for left channel and add 256 to convert to unsigned int (which makes it sound better).
	TA1CCR1 = (RB.pop()+256); //pop next value for left channel and add 256 to convert to unsigned int (which makes it sound better).
	//Tell main to update the buffer
	LPM1_EXIT;
}
 void main(void) {
	 LPM4_EXIT;
	 mmcUnmountBlock();//in case of reset.
	//Turn off the watchdog timer
	WDTCTL = WDTPW + WDTHOLD;

	P1DIR |= LED;
	P1OUT &= ~LED;
	//Set the green LED to output
	//P1DIR |= BIT6;
	//Set up output from Timer A0.1
	//P1SEL |= BIT6;
	//16MHz
	//Set DCO to 16 MHz calibrated
	DCOCTL = CALDCO_16MHZ;
	BCSCTL1 = CALBC1_16MHZ;
	//setup 2.1 and 2.4 to output from TA1.0
	P2DIR |= BIT4;
	P2DIR |= BIT1;
	P2SEL |= BIT4;
	P2SEL |= BIT1;
	TA1CCR0 = 2000;//8khz sample @16Mhz clock rate
	TA1CCTL0 = CCIE; //enable capture control interrupt
	TA1CTL = TASSEL_2 + MC_1 + ID_0; //count up and select timer 2
	_BIS_SR(LPM0_bits + GIE); //enter lpm0 cause keeping it low power, GIE enables all interrupts
	TA1CCTL1 = OUTMOD_7; //pwm
	TA1CCTL2 = OUTMOD_7; //pwm
	__enable_interrupt();
	//Reset SD card by cycling power
	//Credit to Boris Cherkasskiy and his blog post on Launchpad+MMC
	    P2DIR |= BIT2;
	    	P2OUT &= ~BIT2;
	    	__delay_cycles(1600000);	// 100ms @ 16MHz
	    	P2OUT |= BIT2;
	    	__delay_cycles(1600000);	// 100ms @ 16MHz

	//Initialize MMC library
	while (MMC_SUCCESS != mmcInit());
	//Verify card ready
	while (MMC_SUCCESS != mmcPing());
	//Check the memory card size
	volatile unsigned long size = mmcReadCardSize();

	//Toggle the LED to indicate that reading was successful
	P1OUT ^= LED;

	//Test that the SD card is working
	//Read in the OEM name and version in bytes 3-10

	// Read in a 512 byte sector
	// This is a multipart process.
	// First you must mount the block, telling the SD card the relative offset
	// of your subsequent reads. Then you read the block in multiple frames.
	// We do this so we don't need to allocate a full 512 byte buffer in the
	// MSP430's memory. Instead we'll use smaller 64 byte buffers. This
	// means that an entire block will take 8 reads. The spiReadFrame command
	// reads in the given number of bytes into the byte array passed to it.
	// After reading all of the data in the block it should be unmounted.
	//volatile char result = mmcReadBlock(0, 64, block);
	//volatile char result = mmcMountBlock(0, 512);
	//unsigned int long long j = ;
	unsigned char block[128] = {0};
	unsigned long int offset1 =566;
	volatile long int offset = (unsigned long int)offset1 * 512;
	//^^ 566*512 for actual text, 534*512 for file name, 62*512 for FAT16 MSDOS, 0*512 MBR
	volatile char result = mmcMountBlock(offset, 512);
	int x = 3; //bool check for sector end, we read 2 sectors before playing the song.
	//Read the 24 byte header
	spiReadFrame(block, 24);
	unsigned long int* words = (unsigned long int*)block;
	//These are stored in big-endian format
	volatile unsigned long int snd_offset = pointerToWord(block+4);
	volatile long int snd_size = pointerToWord(block+8);
	//We will only play 8bit PCM (decimal 2)
	volatile unsigned char data_encoding = pointerToWord(block+12);
	//We will only play rates up to 16kHz
	volatile unsigned int sample_rate = pointerToWord(block+16);
	volatile unsigned char channels = pointerToWord(block+20);
	//Fill the buffer before starting to play the song.
	spiReadFrame(block,128); //read in first 128 bytes of song to fill block with.
	for(j = 0; j<128; j++){
		RB.push(block[j]);//push in those values.
	}
	spiReadFrame(block,128); //pre read the next 128 bytes.
	volatile long int offset2=0; //song just started playing so this offset is 0.
	while (1){
		if(RB.isFull())LPM1;//enter LPM1 once buffer is full.
		RB.push(block[i]); //push next value to be played in buffer.
		++i; //increase block index


		if(i>127){ //if we hit the end of a block
        	spiReadFrame(block,128); //read in the next 128 values.
        	x++; //increase location in sector.
        	i = 0; //set block index back to 0.
    		if(x>=4){ //if we hit the end of the current sector unmount and increase the sector than remount.
    			mmcUnmountBlock();
    			++offset1; //next sector
    			offset = offset1*512; //figure out start address
    			offset2 = offset2 + 512; //figure out the end of the song.
    			if(offset2>(snd_size+512))break; //if offset2 is greater than send size we just break and exit to loop.
    			mmcMountBlock(offset,512); //mount the new block
    			x = 0; //set the current location in the sector back to 0.
    		}
        }
	}
	mmcUnmountBlock(); //once song is finished unmount block.
	LPM4; //enter LPM4 to use the least power.
}
