#include "mic.h"

uint8_t PDM_Input_Buffer[PDM_Input_Buffer_SIZE]; // Wejsciowy sygnal PDM
uint16_t PCM_Output_Buffer[PCM_Output_Buffer_SIZE]; // Sygnal po konwersji do PCM

uint32_t InternalBufferSize = 0;
uint32_t Data_Status = 0;

void SPI2_IRQHandler(void) {
	u16 volume;
	u16 app;

	// Check if new data are available in SPI data register
	if (SPI_GetITStatus(SPI2, SPI_I2S_IT_RXNE) != RESET) {
		// Read received data and save it in internal table
		app = SPI_I2S_ReceiveData(SPI2);
		PDM_Input_Buffer[InternalBufferSize++] = (uint8_t) app;
		PDM_Input_Buffer[InternalBufferSize++] = (uint8_t) HTONS(app);
		// Check to prevent overflow condition
		if (InternalBufferSize >= PDM_Input_Buffer_SIZE) {
			InternalBufferSize = 0;
			//gain
			volume = 150;
			//konwersja na PCM
			PDM_Filter_64_LSB(PDM_Input_Buffer, PCM_Output_Buffer, volume,
					&Filter);
			Data_Status = 1;
		}
	}
}

void GPIO_Configure(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	// Configure MP45DT02's CLK / I2S2_CLK (PB10) line
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Configure MP45DT02's DOUT / I2S2_DATA (PC3) line
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2); // Connect pin 10 of port B to the SPI2 peripheral
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2); // Connect pin 3 of port C to the SPI2 peripheral
}

void I2S_Configure(void) {
	I2S_InitTypeDef I2S_InitStructure;

	SPI_I2S_DeInit(SPI2);
	I2S_InitStructure.I2S_AudioFreq = OUT_FREQ * 2;
	I2S_InitStructure.I2S_Standard = I2S_Standard_LSB;
	I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
	I2S_InitStructure.I2S_CPOL = I2S_CPOL_High;
	I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
	I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
	I2S_Init(SPI2, &I2S_InitStructure);

	// Enable the Rx buffer not empty interrupt
	SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
}

void NVIC_Configure(void) {
	NVIC_InitTypeDef NVIC_InitStructure;

	// Configure the interrupt priority grouping
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	// Configure the SPI2 interrupt channel
	NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
}

void RCC_Configure(void) {
	// Enable CRC module - required by PDM Library
	//RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC |
	RCC_AHB1Periph_CRC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	RCC_PLLI2SCmd(ENABLE);
}

void mic_start(void) {

	RCC_Configure();
	NVIC_Configure();
	GPIO_Configure();
	I2S_Configure();

	Filter.Fs = OUT_FREQ;
	Filter.HP_HZ = 10;
	Filter.LP_HZ = 16000;
	Filter.In_MicChannels = 1;
	Filter.Out_MicChannels = 1;
	PDM_Filter_Init(&Filter);

	I2S_Cmd(SPI2, ENABLE);

}
