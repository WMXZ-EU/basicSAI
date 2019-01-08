/*
 * WMXZ 08-01-2019
 */
#ifndef _SAIINPUT_H_
#define _SAIINOUT_H_

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

class saiInput_class: public mAudioStream
{
  int32_t rx_buffer[2*NW*NCH*NDAT];

public:

  saiInput_class(void):  mAudioStream(0, NULL)  {   }
  
  void setup(int fs) 
  { sai_setup(fs,NCH, NW, NBITS, TDM, CONF);
    sai_setupInput(0, rx_buffer, NCH, 2*NW*NDAT, CONF);
    //
    update_responsibility=update_setup(AUDIO_PRIO);
  }

  void rxProcessing(void *context, void * taddr);
  virtual void update(void) { return; }

protected:  
  static bool update_responsibility;

} __attribute__( ( aligned ( 0x100 ) ) );


//--------------------------------------------------------------------------------
bool saiInput_class::update_responsibility=false;

void saiInput_class::rxProcessing(void *context, void * taddr)
{ // taddr -> inp_block with demux
  int32_t *inp = (int32_t *) taddr;
  audio_block_t *out_block[NBL];
  
  for (int ii=0; ii < NBL; ii++) 
  { out_block[ii] = mAudioStream::allocate();
    if(out_block[ii] == NULL) 
    {
      for(int jj=0; jj < ii; jj++) release(out_block[jj]);
      return;
    }
  }
  
  for(int ii=0; ii < NBL; ii++)
  { int32_t *out = out_block[ii]->data;
    for(int jj=0; jj < NDAT; jj++)
    {
      out[jj] = inp[ii+jj*NW*NCH];
    }
    transmit(out_block[ii],ii);
    release(out_block[ii]);
  }

  if(update_responsibility) mAudioStream::update_all(); 
}

#endif
