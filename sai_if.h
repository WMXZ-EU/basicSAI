/*
 * WMXZ 08-01-2019
 */
#ifndef _SAI_IF_H
#define _SAI_IF_H

// some missing macros/definitions
#define CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(n) ((uint32_t)(((n) & 0x03)<<19)) 
#define CCM_ANALOG_PLL_AUDIO_BYPASS ((uint32_t)(1<<16)) 
#define CCM_ANALOG_PLL_AUDIO_BYPASS_CLK_SRC(n) ((uint32_t)(((n) & 0x03)<<14)) 
#define CCM_ANALOG_PLL_AUDIO_ENABLE ((uint32_t)(1<<13)) 
#define CCM_ANALOG_PLL_AUDIO_POWERDOWN ((uint32_t)(1<<12)) 
#define CCM_ANALOG_PLL_AUDIO_DIV_SELECT(n) ((uint32_t)((n) & ((1<<6)-1))) 

#define CCM_ANALOG_MISC2_DIV_MSB (1u<<23)
#define CCM_ANALOG_MISC2_DIV_LSB (1u<<15)

#define CCM_ANALOG_PLL_AUDIO_NUM_MASK (((1<<29)-1))
#define CCM_ANALOG_PLL_AUDIO_DENOM_MASK (((1<<29)-1))

#define CCM_CSCMR1_SAI1_CLK_SEL_MASK (CCM_CSCMR1_SAI1_CLK_SEL(0x03))
#define CCM_CS1CDR_SAI1_CLK_PRED_MASK (CCM_CS1CDR_SAI1_CLK_PRED(0x07))
#define CCM_CS1CDR_SAI1_CLK_PODF_MASK (CCM_CS1CDR_SAI1_CLK_PODF(0x3f))

#define CCM_CSCMR1_SAI2_CLK_SEL_MASK (CCM_CSCMR1_SAI2_CLK_SEL(0x03))
#define CCM_CS2CDR_SAI2_CLK_PRED_MASK (CCM_CS2CDR_SAI2_CLK_PRED(0x07))
#define CCM_CS2CDR_SAI2_CLK_PODF_MASK (CCM_CS2CDR_SAI2_CLK_PODF(0x3f))

#define CCM_CSCMR1_SAI3_CLK_SEL_MASK (CCM_CSCMR1_SAI3_CLK_SEL(0x03))
#define CCM_CS1CDR_SAI3_CLK_PRED_MASK (CCM_CS1CDR_SAI3_CLK_PRED(0x07))
#define CCM_CS1CDR_SAI3_CLK_PODF_MASK (CCM_CS1CDR_SAI3_CLK_PODF(0x3f))

#define CCM_CDCDR_SPDIF0_CLK_SEL_MASK (CCM_CDCDR_SPDIF0_CLK_SEL(0x03))
#define CCM_CDCDR_SPDIF0_CLK_PRED_MASK (CCM_CDCDR_SPDIF0_CLK_PRED(0x07))
#define CCM_CDCDR_SPDIF0_CLK_PODF_MASK (CCM_CDCDR_SPDIF0_CLK_PODF(0x07))
//
//subset of missing definitions
// for receiver
#define I2S_RCR1_RFW(n)     ((uint32_t)n & 0x1f)    // Receive FIFO watermark
#define I2S_RCR2_DIV(n)     ((uint32_t)n & 0xff)    // Bit clock divide by (DIV+1)*2
#define I2S_RCR2_BCD      ((uint32_t)1<<24)   // Bit clock direction
#define I2S_RCR2_MSEL(n)    ((uint32_t)(n & 3)<<26)   // MCLK select, 0=bus clock, 1=I2S0_MCLK
#define I2S_RCR2_SYNC(n)    ((uint32_t)(n & 3)<<30)   // 0=async 1=sync with trasmitter
#define I2S_RCR3_RCE        ((uint32_t)0x10000)   // receive channel enable
#define I2S_RCR4_FSD      ((uint32_t)1)     // Frame Sync Direction
#define I2S_RCR4_FSE      ((uint32_t)8)     // Frame Sync Early
#define I2S_RCR4_MF     ((uint32_t)0x10)    // MSB First
#define I2S_RCR4_SYWD(n)    ((uint32_t)(n & 0x1f)<<8) // Sync Width
#define I2S_RCR4_FRSZ(n)    ((uint32_t)(n & 0x0f)<<16)  // Frame Size
#define I2S_RCR5_FBT(n)     ((uint32_t)(n & 0x1f)<<8) // First Bit Shifted
#define I2S_RCR5_W0W(n)     ((uint32_t)(n & 0x1f)<<16)  // Word 0 Width
#define I2S_RCR5_WNW(n)     ((uint32_t)(n & 0x1f)<<24)  // Word N Width

#define I2S_RCSR_RE     ((uint32_t)0x80000000)    // Receiver Enable
#define I2S_RCSR_FR     ((uint32_t)0x02000000)    // FIFO Reset
#define I2S_RCSR_FRDE     ((uint32_t)0x00000001)    // FIFO Request DMA Enable
#define I2S_RCSR_BCE      ((uint32_t)0x10000000)    // Bit Clock Enable

// for transmitter
#define I2S_TCR1_RFW(n)     ((uint32_t)n & 0x1f)    // Receive FIFO watermark
#define I2S_TCR2_DIV(n)     ((uint32_t)n & 0xff)    // Bit clock divide by (DIV+1)*2
#define I2S_TCR2_BCD      ((uint32_t)1<<24)   // Bit clock direction
#define I2S_TCR2_MSEL(n)    ((uint32_t)(n & 3)<<26)   // MCLK select, 0=bus clock, 1=I2S0_MCLK
#define I2S_TCR2_SYNC(n)    ((uint32_t)(n & 3)<<30)   // 0=async 1=sync with receiver
#define I2S_TCR3_TCE        ((uint32_t)0x10000)   // receive channel enable
#define I2S_TCR4_FSD      ((uint32_t)1)     // Frame Sync Direction
#define I2S_TCR4_FSE      ((uint32_t)8)     // Frame Sync Early
#define I2S_TCR4_MF     ((uint32_t)0x10)    // MSB First
#define I2S_TCR4_SYWD(n)    ((uint32_t)(n & 0x1f)<<8) // Sync Width
#define I2S_TCR4_FRSZ(n)    ((uint32_t)(n & 0x0f)<<16)  // Frame Size
#define I2S_TCR5_FBT(n)     ((uint32_t)(n & 0x1f)<<8) // First Bit Shifted
#define I2S_TCR5_W0W(n)     ((uint32_t)(n & 0x1f)<<16)  // Word 0 Width
#define I2S_TCR5_WNW(n)     ((uint32_t)(n & 0x1f)<<24)  // Word N Width

#define I2S_TCSR_TE     ((uint32_t)0x80000000)    // Receiver Enable
#define I2S_TCSR_BCE      ((uint32_t)0x10000000)    // Bit Clock Enable
#define I2S_TCSR_FR     ((uint32_t)0x02000000)    // FIFO Reset
#define I2S_TCSR_FRDE     ((uint32_t)0x00000001)    // FIFO Request DMA Enable

void sai_setupInput(int ch, int32_t * buffer, int nch, int nloop, int conf);
void sai_setupOutput(int ch, int32_t * buffer, int nch, int nloop, int conf);
void sai_setup(int fs, int nch, int nw, int nbits, int tdm, int conf);
#endif
