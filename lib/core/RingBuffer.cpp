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

#include <cstdint>
#include <cstdlib>

#include "RingBuffer.h"

// round up to the next power of 2
// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static inline uint32_t next_pow2(uint32_t v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

RingBuffer::RingBuffer(void) : RingBuffer(SERIAL_BUFFER_SIZE) { }

RingBuffer::RingBuffer(uint32_t _size)
{
  size = pow2_roundup(_size);
  _aucBuffer = static_cast<uint8_t*>(calloc(size, 1));
  clear();
}

RingBuffer::~RingBuffer(void)
{
  free(_aucBuffer);
}

uint32_t RingBuffer::getSize(void)
{
  return size;
}

void RingBuffer::store_char(uint8_t c)
{
  int i = nextIndex(_iHead);

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  if ( i != _iTail )
  {
    _aucBuffer[_iHead] = c;
    _iHead = i;
  }
}

void RingBuffer::clear(void)
{
  _iHead = 0;
  _iTail = 0;
}

int RingBuffer::read_char(void)
{
  if(_iTail == _iHead)
    return -1;

  uint8_t value = _aucBuffer[_iTail];
  _iTail = nextIndex(_iTail);

  return value;
}

int RingBuffer::available(void)
{
  int delta = _iHead - _iTail;

  if(delta < 0)
    return size + delta;
  else
    return delta;
}

int RingBuffer::availableForStore(void)
{
  if (_iHead >= _iTail)
    return size - 1 - _iHead + _iTail;
  else
    return _iTail - _iHead - 1;
}

int RingBuffer::peek(void)
{
  if(_iTail == _iHead)
    return -1;

  return _aucBuffer[_iTail];
}

int RingBuffer::nextIndex(int index)
{
  // use & for modulus because size is locked to a power of 2
  return (uint32_t)(index + 1) & (size - 1);
}

bool RingBuffer::isFull(void)
{
  return (nextIndex(_iHead) == _iTail);
}
