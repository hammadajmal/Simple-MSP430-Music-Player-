#define MAX_BUF 128
class RingBuffer {
private:
  //Where to add data
  unsigned int head;
  //Where to read data from
  unsigned int tail;
  char buffer[MAX_BUF];
  //Need to distinguish between empty and full
  unsigned char empty;

public:
  bool isFull();
  bool isEmpty();
  void push(char value);
  char pop();
  RingBuffer();
};
