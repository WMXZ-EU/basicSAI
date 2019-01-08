/*
 * WMXZ 08-01-2019
 */
#include "core_pins.h"
#include "usb_serial.h"
#include "sai_if.h"

typedef struct
{
  uint32_t CSR;
  uint32_t CR1,CR2,CR3,CR4,CR5;
  uint32_t DR[8];
  uint32_t FR[8];
  uint32_t MR;
} I2S_PORT;

typedef struct
{
  uint32_t VERID;
  uint32_t PARAM;
  I2S_PORT TX;
  uint32_t unused[9];
  I2S_PORT RX;
} I2S_STRUCT;

I2S_STRUCT *I2S1 = ((I2S_STRUCT *)0x40384000);
I2S_STRUCT *I2S2 = ((I2S_STRUCT *)0x40388000);
I2S_STRUCT *I2S3 = ((I2S_STRUCT *)0x4038C000);

I2S_STRUCT *i2s;

IMXRT_DMA_TCD_t     *dma       ((IMXRT_DMA_TCD_t *)0x400E9000);
uint32_t            *dma_cfg   ((uint32_t *)0x400EC000);

typedef struct 
{ uint32_t ch;
  uint32_t data1, data2;
  uint32_t context;
} DMA_t;

DMA_t dma_rx, dma_tx;

void sai_rx_isr(void); // forward declaration
void sai_tx_isr(void); // forward declaration

#define I2S_PIN (22) // set to (-1) to disable
void sai_rxProcessing(void * context, void * taddr);  // forward declaration
void sai_txProcessing(void * context, void * taddr);  // forward declaration

uint32_t dmaMux[3][2] = {{DMAMUX_SOURCE_SAI1_RX, DMAMUX_SOURCE_SAI1_TX},
                         {DMAMUX_SOURCE_SAI2_RX, DMAMUX_SOURCE_SAI2_TX},
                         {DMAMUX_SOURCE_SAI3_RX, DMAMUX_SOURCE_SAI3_TX}};
                         
/****************************************************************/
void set_audioClock(int nfact, int32_t nmult, uint32_t ndiv)
{ // sets PLL4
  CCM_ANALOG_PLL_AUDIO = 0;
  //CCM_ANALOG_PLL_AUDIO |= CCM_ANALOG_PLL_AUDIO_BYPASS;
  CCM_ANALOG_PLL_AUDIO |= CCM_ANALOG_PLL_AUDIO_ENABLE;
  CCM_ANALOG_PLL_AUDIO |= CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(2); // 0: 1/4; 1: 1/2; 0: 1/1
  CCM_ANALOG_PLL_AUDIO |= CCM_ANALOG_PLL_AUDIO_DIV_SELECT(nfact);    
  
  CCM_ANALOG_PLL_AUDIO_NUM   = nmult &CCM_ANALOG_PLL_AUDIO_NUM_MASK;
  CCM_ANALOG_PLL_AUDIO_DENOM = ndiv &CCM_ANALOG_PLL_AUDIO_DENOM_MASK;
  
  const int div_post_pll = 1; // other values: 2,4
  CCM_ANALOG_MISC2 &= ~(CCM_ANALOG_MISC2_DIV_MSB | CCM_ANALOG_MISC2_DIV_LSB);
  if(div_post_pll>1)
    CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_LSB;
  if(div_post_pll>3)
    CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_MSB;
}
//
void sai1_initClock(int n1, int n2) 
{ 
  CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

  // clear SAI1_CLK register locations
  CCM_CSCMR1 &= ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK);
  CCM_CS1CDR &= ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK);
  //
  CCM_CSCMR1 |= CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4,  
  CCM_CS1CDR |= CCM_CS1CDR_SAI1_CLK_PRED(n1-1); // &0x07
  CCM_CS1CDR |= CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f   
}

void sai2_initClock(int n1, int n2) 
{ 
  CCM_CCGR5 |= CCM_CCGR5_SAI2(CCM_CCGR_ON);

  // clear SAI1_CLK register locations
  CCM_CSCMR1 &= ~(CCM_CSCMR1_SAI2_CLK_SEL_MASK);
  CCM_CS2CDR &= ~(CCM_CS2CDR_SAI2_CLK_PRED_MASK | CCM_CS2CDR_SAI2_CLK_PODF_MASK);
  //
  CCM_CSCMR1 |= CCM_CSCMR1_SAI2_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4,  
  CCM_CS2CDR |= CCM_CS2CDR_SAI2_CLK_PRED(n1-1); // &0x07
  CCM_CS2CDR |= CCM_CS2CDR_SAI2_CLK_PODF(n2-1); // &0x3f   
}
void spdif0_initClock(int n1, int n2) 
{
  CCM_CCGR5 |= CCM_CCGR5_SPDIF(CCM_CCGR_ON);
  // clear SAI1_CLK register locations
  CCM_CDCDR &= ~(CCM_CDCDR_SPDIF0_CLK_SEL_MASK);
  CCM_CDCDR &= ~(CCM_CDCDR_SPDIF0_CLK_PRED_MASK | CCM_CDCDR_SPDIF0_CLK_PODF_MASK);
  //
  CCM_CDCDR |= CCM_CDCDR_SPDIF0_CLK_SEL(0); // &0x03 // (0,1,2): PLL4, PLL3PFD2, PLL5  
  CCM_CDCDR |= CCM_CDCDR_SPDIF0_CLK_PRED(n1-1); // &0x07
  CCM_CDCDR |= CCM_CDCDR_SPDIF0_CLK_PODF(n2-1); // &0x3f   
}

void sai1_selectMCLK(int isel)
{
  IOMUXC_GPR_GPR1 &= ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK);
  IOMUXC_GPR_GPR1 |= (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(isel));
}
void sai2_selectMCLK(int isel)
{
  IOMUXC_GPR_GPR1 &= ~(IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL_MASK);
  IOMUXC_GPR_GPR1 |= (IOMUXC_GPR_GPR1_SAI2_MCLK_DIR | IOMUXC_GPR_GPR1_SAI2_MCLK3_SEL(isel));
}

//
void sai1_configurePorts(void)
{   // SAI1
    CORE_PIN23_CONFIG = 3;  //1:MCLK
    CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
    CORE_PIN20_CONFIG = 3;  //1:RX_SYNC
    CORE_PIN7_CONFIG  = 3;  //1:RX_DATA0
    CORE_PIN6_CONFIG  = 3;  //1:TX_DATA0

//  CORE_PIN8_CONFIG  = 3;  //1:RX1_TX3_DATA // not used
//  CORE_PIN9_CONFIG  = 3;  //1:RX2_RX2_DATA // not used
//  CORE_PIN32_CONFIG = 3;  //1:RX3_TX1_DATA // not used

//  CORE_PIN26_CONFIG = 3;  //1:TX_BCLK
//  CORE_PIN27_CONFIG = 3;  //1:TX_SYNC
}
void sai2_configurePorts(void)
{ // SAI2
    CORE_PIN5_CONFIG  = 2;  //2:MCLK
    CORE_PIN4_CONFIG  = 2;  //2:TX_BCLK
    CORE_PIN3_CONFIG  = 2;  //2:TX_SYNC
    CORE_PIN2_CONFIG  = 2;  //2:TX_DATA0
    CORE_PIN33_CONFIG = 2;  //2:RX_DATA0
}

//#define DEBUG
//
void sai_rxConfig(int ndiv, int nch, int nbits, int nw, int tdm, int sync)
{
  i2s->RX.MR = 0;
  i2s->RX.CSR = 0;
  i2s->RX.CR1 = I2S_RCR1_RFW(1); 
  i2s->RX.CR2 = I2S_RCR2_SYNC(sync);// | I2S_RCR2_BCP ; // sync=0; rx is async; 

  i2s->RX.CR2 |= (I2S_RCR2_BCD | I2S_RCR2_DIV((ndiv-1)) | I2S_RCR2_MSEL(1));
  i2s->RX.CR3 = 0;
  for(int ii=0; ii<nch; ii++)
    i2s->RX.CR3 |= I2S_RCR3_RCE<<ii; // mark rx channel

  //
  int ns = (tdm>0)? 1: nbits;
 
  i2s->RX.CR4 = I2S_RCR4_FRSZ(nw-1) 
        | I2S_RCR4_SYWD(ns-1) 
        | I2S_RCR4_MF
        | I2S_RCR4_FSD;
  if(tdm>0)
    i2s->RX.CR4 |= I2S_RCR4_FSE;

  i2s->RX.CR5 = I2S_RCR5_WNW((nbits-1)) | I2S_RCR5_W0W((nbits-1)) | I2S_RCR5_FBT((nbits-1));
  #ifdef DEBUG
    Serial.println(i2s->RX.CR1,HEX);
    Serial.println(i2s->RX.CR2,HEX);
    Serial.println(i2s->RX.CR3,HEX);
    Serial.println(i2s->RX.CR4,HEX);
    Serial.println(i2s->RX.CR5,HEX);
  #endif
}

//
void sai_txConfig(int ndiv, int nch, int nbits, int nw, int tdm, int sync)
{
  i2s->TX.MR = 0;
  i2s->TX.CSR = 0;
  i2s->TX.CR1 = I2S_TCR1_RFW(1); 
  i2s->TX.CR2 = I2S_TCR2_SYNC(sync) ;//| I2S_TCR2_BCP ; // sync=0; tx is async; 

  i2s->TX.CR2 |= (I2S_TCR2_BCD | I2S_TCR2_DIV((ndiv-1)) | I2S_TCR2_MSEL(1));
  i2s->TX.CR3 = 0;
  for(int ii=0; ii<nch; ii++)
    i2s->TX.CR3 |= I2S_TCR3_TCE<<ii; // mark tx channel

  //
  int ns = (tdm>0)? 1: nbits;
  i2s->TX.CR4 = I2S_TCR4_FRSZ(nw-1) 
        | I2S_TCR4_SYWD(ns-1) 
        | I2S_TCR4_MF
        | I2S_TCR4_FSD;
  if(tdm>0)
    i2s->TX.CR4 |= I2S_TCR4_FSE;
  i2s->TX.CR5 = I2S_TCR5_WNW((nbits-1)) | I2S_TCR5_W0W((nbits-1)) | I2S_TCR5_FBT((nbits-1));
  #ifdef DEBUG
    Serial.println(i2s->TX.CR1,HEX);
    Serial.println(i2s->TX.CR2,HEX);
    Serial.println(i2s->TX.CR3,HEX);
    Serial.println(i2s->TX.CR4,HEX);
    Serial.println(i2s->TX.CR5,HEX);
  #endif
}

void dma_init(void)
{
  CCM_CCGR5 |= CCM_CCGR5_DMA(CCM_CCGR_ON);
  DMA_CR = 0;
  DMA_CR = DMA_CR_GRP1PRI | DMA_CR_EMLM | DMA_CR_EDBG | DMA_CR_ERCA;
  //
  if(I2S_PIN>=0) pinMode(I2S_PIN,OUTPUT);
}

void sai_setupInput(int ch, int32_t * buffer, int nch, int nloop, int iconf)
{ 
  //
  dma_rx.data1 = (uint32_t)&buffer[0];
  dma_rx.data2 = (uint32_t)&buffer[nch*nloop/2];
  dma_rx.context = nch*nloop/2;

  dma_cfg[ch] = 0;
  dma_cfg[ch] = ((dmaMux[iconf][0] & 0x7F) | DMAMUX_CHCFG_ENBL);

  // clear DMA registers
  dma[ch].CSR = 0;
  dma[ch].ATTR = 0;
  //
  // major loop
  dma[ch].BITER = nloop;
  dma[ch].CITER = dma[ch].BITER;
  //
  // minor loop
  dma[ch].NBYTES = 4*nch;
  if(nch>1)
  {
    dma[ch].NBYTES |= DMA_TCD_NBYTES_SMLOE ;
    dma[ch].NBYTES |= DMA_TCD_NBYTES_MLOFFYES_MLOFF(-4*nch);
  }
  //
  // Source 
  dma[ch].SADDR = (void*)&i2s->RX.DR[0];
  dma[ch].ATTR |= DMA_TCD_ATTR_SSIZE(2);
  if(nch>1)
  {
    dma[ch].SOFF = 4;
    dma[ch].SLAST = -4*nch;
  }
  else
  {
    dma[ch].SOFF = 0;
    dma[ch].SLAST = 0;
  }
  //
  // Destination 
  dma[ch].DADDR = buffer;
  dma[ch].ATTR |= DMA_TCD_ATTR_DSIZE(2);
  dma[ch].DOFF = 4;
  dma[ch].DLASTSGA = -4*nch*nloop;
//
  dma[ch].CSR |= DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
  
  attachInterruptVector((IRQ_NUMBER_t) ch, sai_rx_isr);
  NVIC_ENABLE_IRQ((IRQ_NUMBER_t) ch);
  NVIC_SET_PRIORITY((IRQ_NUMBER_t) ch, 7*16); // 8 is normal priority
  //
  i2s->RX.CSR |= I2S_RCSR_FRDE | I2S_RCSR_FR;
  i2s->RX.CSR |= I2S_RCSR_RE | I2S_RCSR_BCE;

  DMA_SERQ = dma_rx.ch = ch;
  dma[ch].CSR |=  DMA_TCD_CSR_START;
}

void sai_setupOutput(int ch, int32_t * buffer, int nch, int nloop, int iconf)
{
  dma_tx.data1 = (uint32_t)&buffer[0];
  dma_tx.data2 = (uint32_t)&buffer[nch*nloop/2];
  dma_tx.context = nch*nloop/2;

  dma_cfg[ch] = 0;
  dma_cfg[ch] = (dmaMux[iconf][1] & 0x7F) | DMAMUX_CHCFG_ENBL;
  
  DMA_CERQ = 1;
  DMA_CERR = 1;
  DMA_CEEI = 1;
  DMA_CINT = 1;

  // clear DMA registers
  dma[ch].CSR = 0;
  dma[ch].ATTR = 0;
  
  //
  // major loop
  dma[ch].BITER = nloop;
  dma[ch].CITER = dma[ch].BITER;
  //
  // minor loop
  dma[ch].NBYTES = 4*nch;
  if(nch>1)
  {
    dma[ch].NBYTES |= DMA_TCD_NBYTES_SMLOE ;
    dma[ch].NBYTES |= DMA_TCD_NBYTES_MLOFFYES_MLOFF(-4*nch);
  }
  //
  // Source 
  dma[ch].SADDR = buffer;
  dma[ch].ATTR |= DMA_TCD_ATTR_SSIZE(2);
  dma[ch].SOFF = 4;
  dma[ch].SLAST = -4*nch*nloop;
  //
  // Destination 
  dma[ch].DADDR = (void *)&i2s->TX.DR[0];
  dma[ch].ATTR |= DMA_TCD_ATTR_DSIZE(2);
  if(nch>1)
  {
    dma[ch].DOFF = 4;
    dma[ch].DLASTSGA = -4*nch;
  }
  else
  {
    dma[ch].DOFF = 0;
    dma[ch].DLASTSGA = 0;
  }
  //
  dma[ch].CSR |= DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

  attachInterruptVector((IRQ_NUMBER_t) ch, sai_tx_isr);
  NVIC_ENABLE_IRQ((IRQ_NUMBER_t) ch);
  NVIC_SET_PRIORITY((IRQ_NUMBER_t) ch, 7*16); // 8 is normal priority
  //
  i2s->TX.CSR |= I2S_TCSR_FRDE | I2S_TCSR_FR;
  i2s->TX.CSR |= I2S_TCSR_TE | I2S_TCSR_BCE;

  DMA_SERQ = dma_tx.ch = ch;
  dma[ch].CSR |=  DMA_TCD_CSR_START;
}

volatile int32_t i2sCount1=0;
volatile int32_t i2sCount2=0;
volatile int32_t i2sCount3=0;
volatile int32_t i2sCount4=0;
volatile uint32_t max1Ptr=0;
volatile uint32_t min1Ptr=0x40000000;
volatile uint32_t max2Ptr=0;
volatile uint32_t min2Ptr=0x40000000;

void sai_rx_isr(void)
{ uint32_t daddr, taddr;
  //
  DMA_CINT = dma_rx.ch;
  //
  daddr = (uint32_t) (dma[dma_rx.ch].DADDR);
  if(daddr>max1Ptr) max1Ptr=daddr;
  if(daddr<min1Ptr) min1Ptr=daddr;
  if (daddr < dma_rx.data2) 
  {
    // DMA is receiving to the first half of the buffer
    // need to process data from the second half
    taddr = dma_rx.data2;
    //
    i2sCount1++;
    if(I2S_PIN>=0) digitalWrite(I2S_PIN, HIGH);
  } 
  else 
  {
    // DMA is receiving to the second half of the buffer
    // need to process data from the first half
    taddr = dma_rx.data1;
    //
    i2sCount2++;
    if(I2S_PIN>=0) digitalWrite(I2S_PIN, LOW);
  }
  //up call
  sai_rxProcessing((void *) &dma_rx.context, (void *) taddr);
}

void sai_tx_isr(void)
{ uint32_t daddr, taddr;
  //
  DMA_CINT = dma_tx.ch;
  //
  daddr = (uint32_t) (dma[dma_tx.ch].SADDR);
  if(daddr>max2Ptr) max2Ptr=daddr;
  if(daddr<min2Ptr) min2Ptr=daddr;
  if (daddr < dma_tx.data2) 
  {
    // DMA is transmitting from the first half of the buffer
    // need to process data for the second half
    taddr = dma_tx.data2;
    //
    i2sCount3++;
  } 
  else 
  {
    // DMA is transmitting from the second half of the buffer
    // need to process data for the first half
    taddr = dma_tx.data1;
    //
    i2sCount4++;
  }
  //up call
  sai_txProcessing((void *) &dma_tx.context, (void *) taddr);
}

/************************************************************************/
void sai_setup(int fs, int nch, int nw, int nbits, int tdm, int conf)
{ static int sai_active=0;
  if(sai_active) return;
  sai_active=1;
  //
  int bit_clk = fs*(nw*nbits);
  int nov = 4; // factor of oversampling MCKL/BCKL
  int fs_mclk = nov*bit_clk; // here 49.152 MHz 
  Serial.printf("bits / word :       %d\n",nbits);
  Serial.printf("words / channel :   %d\n",nw);
  Serial.printf("number of channel : %d\n",nch);
  if (tdm==1)
    Serial.printf("Mode: TDM\n");
  else
    Serial.printf("Mode: I2S\n");
  //  
  Serial.printf("Frame rate :  %9d Hz\n",fs);
  Serial.printf("Bit Clock :   %9d Hz\n",bit_clk);
  Serial.printf("Master Clock: %9d Hz\n",fs_mclk);
  //
  int c0, c1, c2;
  int n1, n2;
  //
  n1 = 4; // to ensure that input to last divisor (i.e. n2) is < 300 MHz
  n2 = (4*192000)/fs; // to reduce clock further to become MCLK (n2=4 -> fs=192000)
  //
  // the PLL runs between 27*24 and 54*24 MHz (before dividers)
  // e.g. 49.152 = 32.768*24 / (n1*n2)
  c0 = 32;  
  c1 = 768;
  c2 = 1000;
  // Note: c0+c1/c2 must be between 27 and 54
  //       here: 32.768*24 MHz = 786.4320 MHz
  //
  set_audioClock(c0,c1,c2);
  int rx_sync, tx_sync;
  if(conf==0)
  {
    sai1_configurePorts();
    sai1_initClock(n1,n2);
    sai1_selectMCLK(0);
    //SAI1 has mainly RX clock on T4
    rx_sync=0;
    tx_sync=1;
    i2s = I2S1;  }
  else if(conf==1)
  {
    sai2_configurePorts();
    sai2_initClock(n1,n2);
    sai2_selectMCLK(0);
    // SAI2 has only TX clocks on T4
    tx_sync=0;
    rx_sync=1;
    i2s = I2S2;
  }
  //
  dma_init();
  int ndiv = nov/2;   // MCLK -> 2* BitClock
  sai_rxConfig(ndiv, nch, nbits, nw, tdm, rx_sync);
  sai_txConfig(ndiv, nch, nbits, nw, tdm, tx_sync);
}
