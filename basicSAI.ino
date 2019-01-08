// I2S input test with DMA
//
//
/*
void usdhc_configurePorts(void)
{
  GPIO_SD_B0_04_DAT2=0; // pin 1  (close to pin2)
  GPIO_SD_B0_05_DAT3=0;
  GPIO_SD_B0_00_CMD=0;
  //3.3V
  GPIO_SD_B0_01_CLK=0;
  //GND
  GPIO_SD_B0_02_DAT0=0;
  GPIO_SD_B0_03_DAT1=0; // pin 8 (close to pin 23)

}
typedef struct {
        const uint32_t VERID;           // 0
        const uint32_t PARAM;           // 0x04
        const uint32_t UNUSED0[2];      // 0x08
        volatile uint32_t CR;           // 0x10
        volatile uint32_t SR;           // 0x14
        volatile uint32_t IER;          // 0x18
        volatile uint32_t DER;          // 0x1c
        volatile uint32_t CFGR0;        // 0x20
        volatile uint32_t CFGR1;        // 0x24
        const uint32_t UNUSED1[2];      // 0x28
        volatile uint32_t DMR0;         // 0x30
        volatile uint32_t DMR1;         // 0x34
        const uint32_t UNUSED2[2];      // 0x38
        volatile uint32_t CCR;          // 0x40
        const uint32_t UNUSED3[5];      // 0x44
        volatile uint32_t FCR;          // 0x58
        volatile uint32_t FSR;          // 0x5C
        volatile uint32_t TCR;          // 0x60
        volatile uint32_t TDR;          // 0x64
        const uint32_t UNUSED4[2];      // 0x68
        volatile uint32_t RSR;          // 0x70
        volatile uint32_t RDR;          // 0x74
} IMXRT_LPSPI_t;
*/
#define NBITS 32
#define DATA_TYPE int32_t
#define NW 2  // number of words / channel
#define NCH 1 // number of channels // allowed 1,2,4
#define TDM 0 // use short conv-flag
#define NBL 2
#define MQU 200
#define MAUDIO (NBL*MQU+4)

#include "saiInput.h"
saiInput_class sai1;

#include "saiOutput.h"
saiOutput_class sai2;

#include "mRecordQueue.h"
  mRecordQueue queue[NBL];
  mAudioConnection     patchCord1(sai1,0, sai2,0);
  mAudioConnection     patchCord2(sai1,1, sai2,1);
  mAudioConnection     patchCord3(sai1,0, queue[0],0);
  mAudioConnection     patchCord4(sai1,1, queue[1],0);
//
/***********SAI interface routines*****************************************************/
uint32_t rxCount = 0;
uint32_t txCount = 0;
void sai_rxProcessing(void *context, void * taddr)
{ rxCount++; sai1.rxProcessing(context,taddr); }
void sai_txProcessing(void *context, void * taddr)
{ txCount++; sai2.txProcessing(context,taddr); }

//
DMAMEM audio_block_t audioData[MAUDIO]; 
void setup() {
  // put your setup code here, to run once:
  while(!Serial);
  Serial.println("T4_Test");

  mAudioStream::initialize_memory(audioData, MAUDIO);

  int fs=192000;
  sai1.setup(fs);
  sai2.setup(fs);

  queue[0].begin();
  queue[1].begin();
  
  pinMode(LED_BUILTIN, OUTPUT);
}

extern int32_t i2sCount1;
extern int32_t i2sCount2;
extern int32_t i2sCount3;
extern int32_t i2sCount4;
extern uint32_t max1Ptr;
extern uint32_t min1Ptr;
extern uint32_t max2Ptr;
extern uint32_t min2Ptr;

uint32_t Q[NBL]={0};
void loop() {
  // put your main code here, to run repeatedly:

  while(queue[0].available())
  {
    // fetch data from queue
    int32_t * data1 = (int32_t *)queue[0].readBuffer();
    queue[0].freeBuffer();
    Q[0]++;
    //
    for(int ii=1;ii<NBL; ii++)
    {
      while(!queue[ii].available()) asm("wfi");
      // fetch data from queue
      int32_t * data2 = (int32_t *)queue[ii].readBuffer();
      queue[ii].freeBuffer();
      Q[ii]++;
    }
  }
 
  static uint32_t to=millis();
  if(millis()-to>1000)
  { to=millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    //
    Serial.printf("%d %d _ %d %d %d %d - %x %x - %x %x - %d %d\n",
                    rxCount,txCount,
                    i2sCount1,i2sCount2,i2sCount3,i2sCount4, 
                    max1Ptr, min1Ptr, max2Ptr, min2Ptr,
                    Q[0],Q[1]);
                    
    rxCount=0;  txCount=0;
    i2sCount1=0;  i2sCount2=0;  i2sCount3=0;  i2sCount4=0;
    max1Ptr=0;  min1Ptr=0x40000000;
    max2Ptr=0;  min2Ptr=0x40000000;
    Q[0]=0;
    Q[1]=0;
  }
}
