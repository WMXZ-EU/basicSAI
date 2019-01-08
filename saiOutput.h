/*
 * WMXZ 08-01-2019
 */
#ifndef _SAIOUTPUT_H_
#define _SAIOUTPUT_H_

#include "stdint.h"
#include "sai_if.h"

#include "mAudioStream.h"

#define NDAT AUDIO_BLOCK_SAMPLES // number of samples / (words * channel)
//#define NDAT 128
#define CONF 0 // 0: SAI1; 1: SAI2
//#define NBUF (2*NW*NCH*NDAT)
#define AUDIO_PRIO 13
/*
 * e.g. 
 * single stereo: NW = 2; NCH = 1;
 * dual stereo:   NW = 2; NCH = 2;
 * quad stereo:   NW = 2; NCH = 4;
 * 8-ch TDM:      NW = 8; NCH = 1;
 * 4*8 ch TDM:    NW = 8; NCH = 4;
 */


class saiOutput_class: public mAudioStream
{
  int32_t tx_buffer[2*NW*NCH*NDAT];
  audio_block_t *inputQueueArray[NBL];

public:

  saiOutput_class(void):  mAudioStream(NBL, inputQueueArray)  {   }
  
  void setup(int fs) 
  { sai_setup(fs,NCH, NW, NBITS, TDM, CONF);
    sai_setupOutput(1, tx_buffer, NCH, 2*NW*NDAT, CONF);
    //
    update_responsibility=update_setup(AUDIO_PRIO);
  }

  void txProcessing(void *context, void * taddr);
  virtual void update(void) { return; }

protected:  
  static bool update_responsibility;

} __attribute__( ( aligned ( 0x100 ) ) );


//--------------------------------------------------------------------------------
bool saiOutput_class::update_responsibility=false;

void saiOutput_class::txProcessing(void *context, void * taddr)
{ // taddr -> inp_block with demux
  int32_t *out = (int32_t *) taddr;
  audio_block_t *inp_block[NBL];

  for (int ii=0; ii < NBL; ii++) 
  { inp_block[ii] = receiveReadOnly();
    if(inp_block[ii] == NULL) 
    {
      for(int jj=0; jj < ii; jj++) release(inp_block[jj]);
      return;
    }
  }
  
  for(int ii=0; ii < NBL; ii++)
  { int32_t *inp = inp_block[ii]->data;
    for(int jj=0; jj < NDAT; jj++)
    {
      out[ii+jj*NW*NCH] = inp[jj];
    }
    release(inp_block[ii]);
  }

  if(update_responsibility) mAudioStream::update_all(); 
}

#endif
