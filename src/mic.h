#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include "pdm_filter.h"

#define DECIMATION_FACTOR       64
#define OUT_FREQ                32000
#define PDM_Input_Buffer_SIZE   ((OUT_FREQ/1000)*DECIMATION_FACTOR/8) //256
#define PCM_Output_Buffer_SIZE  (OUT_FREQ/1000) //32


uint32_t InternalBufferSize;
uint32_t Data_Status;
PDMFilter_InitStruct Filter;

void SPI2_IRQHandler(void);



void GPIO_Configure(void);

void I2S_Configure(void);

void NVIC_Configure(void);

void RCC_Configure(void);

void mic_start(void);
