#include <cstdio>
#include <cstring>

#include "RingBuffer.h"

void write_to_rb(RingBuffer& rb, const char *str)
{
  while (*str)
    rb.store_char(*str++);
}

void print_rb(RingBuffer& rb)
{
  while (rb.available())
    putchar(rb.read_char());
}

void print_rb(RingBuffer& rb, int count)
{
  while (rb.available() && count)
  {
    putchar(rb.read_char());
    count--;
  }
}

int main()
{
  RingBuffer rb(125);

  //for (int i = 0; i < 8; i++)
    //write_to_rb(rb, "MeowMow\n");

  write_to_rb(rb, "hello world\n");

  printf("head=%d, tail=%d available=%d\n", rb._iHead, rb._iTail, rb.available());

  print_rb(rb, strlen("hello world\n"));
  write_to_rb(rb, "another line\n");

  write_to_rb(rb, "line 3\n");

  printf("before resize head=%d, tail=%d available=%d\n", rb._iHead, rb._iTail, rb.available());
  rb.resize(256);
  printf("after resize head=%d, tail=%d available=%d\n", rb._iHead, rb._iTail, rb.available());
  print_rb(rb);

  return 0;
}
