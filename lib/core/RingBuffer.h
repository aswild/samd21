/*
  Copyright (c) 2014 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <cstdint>

// Define constants and variables for buffering incoming serial data.  We're
// using a ring buffer (I think), in which head is the index of the location
// to which to write the next incoming character and tail is the index of the
// location from which to read.
#define SERIAL_BUFFER_SIZE 64

class RingBuffer
{
  public:
    RingBuffer(void);
    void store_char(uint8_t c);
    void clear(void);
    int read_char(void);
    int available(void);
    int availableForStore(void);
    int peek(void);
    bool isFull(void);

  private:
    uint8_t _aucBuffer[SERIAL_BUFFER_SIZE];
    volatile int _iHead;
    volatile int _iTail;

    int nextIndex(int index);
};


#endif /* RING_BUFFER_H */
