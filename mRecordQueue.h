/*
 * WMXZ 08-01-2019
 * modified to variable buffer size
 */
/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// WMXZ 08-01-2019 modified allow variable buffersize 
// and use (void*) readBuffer to allow general data types (not only int16_t)
 
#ifndef MRECORDQUEUE_H
#define MRECORDQUEUE_H

#include "core_pins.h"
#include "usb_serial.h"

#include "mAudioStream.h"

#ifndef MQ
  #define MQ 53 // was old value in Audio/record_queue.h
#endif

class mRecordQueue : public mAudioStream
{
public:
  mRecordQueue(void) : mAudioStream(1, inputQueueArray),
    userblock(NULL), head(0), tail(0), enabled(0) { }
   
  void begin(void) { clear(); enabled = 1;}
  void end(void) { enabled = 0; }
  uint16_t available(void);
  void clear(void);
  void * readBuffer(void);
  void freeBuffer(void);
  virtual void update(void);
  uint32_t dropCount;
private:
  audio_block_t *inputQueueArray[1];
  audio_block_t * volatile queue[MQ];
  audio_block_t *userblock;
  volatile uint16_t head, tail, enabled;  //inclesed for more than 256 buffers
};

uint16_t mRecordQueue::available(void)
{
  uint16_t h, t;

  h = head;
  t = tail;
  if (h >= t) return h - t;
  return MQ + h - t;
}

void mRecordQueue::clear(void)
{
  uint16_t t;

  if (userblock) {
    release(userblock);
    userblock = NULL;
  }
  t = tail;
  while (t != head) {
    if (++t >= MQ) t = 0;
    release(queue[t]);
  }
  tail = t;
}

void * mRecordQueue::readBuffer(void)
{
  uint16_t t;

  if (userblock) return NULL;
  t = tail;
  if (t == head) return NULL;
  if (++t >= MQ) t = 0;
  userblock = queue[t];
  tail = t;
  return userblock->data;
}

void mRecordQueue::freeBuffer(void)
{
  if (userblock == NULL) return;
  release(userblock);
  userblock = NULL;
}

void mRecordQueue::update(void)
{
  audio_block_t *block;
  uint16_t h;

  block = receiveReadOnly();
  if (!block) return;
  if (!enabled) {
    release(block);
    return;
  }
  h = head + 1;
  if (h >= MQ) h = 0;
  if (h == tail) {
    release(block); // drop incomming data
    dropCount++; // flag for main to know
  } else {
    queue[h] = block; // store incomming data
    head = h;
  }
}
#endif
