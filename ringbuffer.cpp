/*
 * ringbuffer.cpp
 *
 *  Created on: Apr 30, 2015
 *      Author: ben
 */

#include "ringbuffer.h"

RingBuffer::RingBuffer() {
	head = 0;
	tail = 0;
	empty = 1;
}

bool RingBuffer::isFull() {
	return (!empty && head == tail);
}


bool RingBuffer::isEmpty() {
	return empty;
}

//Push a byte into the ring buffer.
void RingBuffer::push(char value) {
  //Cannot push if the buffer is full
  if (isFull()) { return; }
  //Push back the length
  buffer[head] = value;
  //Reset head if it increments to MAX_BUF (size of the buffer)
  if (MAX_BUF == ++head) {
    head = 0;
  }
  //Just pushed a value, cannot be empty
  empty = 0;
}

//Pop a byte into the ring buffer.
char RingBuffer::pop() {
  //This is an error, trying to over-read the buffer
  if (empty) {
    return 0;
  }
  char value = buffer[tail];
  if (MAX_BUF == ++tail) {
    tail = 0;
  }
  //See if we just emptied the buffer
  if (head == tail) {
    empty = 1;
  }
  return value;
}

