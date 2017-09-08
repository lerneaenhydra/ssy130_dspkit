/**
 * @file         microphone.c
 * @version      1.0
 * @date         2015
 * @author       Christoph Lauer
 * @compiler     armcc
 * @copyright    Christoph Lauer engineering
 */

#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_spi.h>
#include <misc.h>
#include "microphone.h"
#include "headphone.h"
#include "../../config.h"
#include "pdm_filter.h"
#include "backend/cbuf.h"
#include "util.h"

/* SPI Configuration defines */
#define SPI_SCK_PIN                       GPIO_Pin_10
#define SPI_SCK_GPIO_PORT                 GPIOB
#define SPI_SCK_GPIO_CLK                  RCC_AHB1Periph_GPIOB
#define SPI_SCK_SOURCE                    GPIO_PinSource10
#define SPI_SCK_AF                        GPIO_AF_SPI2
#define SPI_MOSI_PIN                      GPIO_Pin_3
#define SPI_MOSI_GPIO_PORT                GPIOC
#define SPI_MOSI_GPIO_CLK                 RCC_AHB1Periph_GPIOC
#define SPI_MOSI_SOURCE                   GPIO_PinSource3
#define SPI_MOSI_AF                       GPIO_AF_SPI2

#define AUDIO_REC_SPI_IRQHANDLER          SPI2_IRQHandler

/* The PDM filter used assumes that it will process 1 ms worth of samples. We
 * don't necessarily want this to be the case, so configure the filter for a
 * different sample rate (of our choosing), allowing us to set up a sample
 * length that is better. Furthermore, to avoid microphone SPI interrupt
 * overrruns perform all PCM processing in a lower-priority context. */
#define PDM_DEC_FAC				(64)	    // Decimation rate for PDM/PCM conversion
#define PDM_FILT_VIRT_FS		(16000)    	// Sample rate to simulate for PDM filter to achieve some desired block length
#define PDM_BLOCK_LENGTH      	(PDM_FILT_VIRT_FS * PDM_DEC_FAC / (1000 * 8))       // Length of a single PDM block (bytes)
#define PCM_BLOCK_LENGTH        (PDM_BLOCK_LENGTH/8)        // PCM buffer output size (16-bit words)
#define PDM_BUF_BLOCKS			(8)			// Number of blocks that can be stored in PDM buffer

static PDMFilter_InitStruct Filter;       	// PDM filter instance

static cbuf_s pdm_buf;
static cbuf_elem_t pdm_buf_pool[PDM_BLOCK_LENGTH * PDM_BUF_BLOCKS];

static bool micbuf_ovf = false;

static void WaveRecorder_GPIO_Init(void);
static void WaveRecorder_SPI_Init(void);
static void WaveRecorder_NVIC_Init(void);
static void WaveRecorderStart(void);
static void WaveRecorderInit(void);

void WaveRecorderBeginSampling (void){
	WaveRecorderInit();
	WaveRecorderStart();
}

static void WaveRecorderInit(){
	/* Reset and enable CRC module */
	CRC->CR = CRC_CR_RESET;
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

	//Set up the PDM buffer
	cbuf_new(&pdm_buf, pdm_buf_pool, NUMEL(pdm_buf_pool));

	/* Filter LP & HP Init */
	//Perform minimal input filtering
	Filter.LP_HZ = MIC_LP_FC * (1.0f * PDM_FILT_VIRT_FS/AUDIO_SAMPLE_RATE);	//Scale the desired cutoff frequency with the simulated sample rate send to the PDM filter
	Filter.HP_HZ = MIC_HP_FC * (1.0f * PDM_FILT_VIRT_FS/AUDIO_SAMPLE_RATE);
	Filter.Fs = PDM_FILT_VIRT_FS;	//Force the PDM filter to assume a samplerate of 16khz, as this implies reading 8*64 PDM bits and outputting 16 PCM words
	Filter.Out_MicChannels = 1;
	Filter.In_MicChannels = 1;

	PDM_Filter_Init((PDMFilter_InitStruct *)&Filter);

	/* Configure the GPIOs */
	WaveRecorder_GPIO_Init();

	/* Configure the interrupts (for timer) */
	WaveRecorder_NVIC_Init();

	/* Configure the SPI */
	WaveRecorder_SPI_Init();
}

static void WaveRecorderStart(void){
	/* Enable the Rx buffer not empty interrupt */
	SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
	/* The Data transfer is performed in the SPI interrupt routine */
	/* Enable the SPI peripheral */
	I2S_Cmd(SPI2, ENABLE);
}

void AUDIO_REC_SPI_IRQHANDLER(void){
	/* Check if data are available in SPI Data register */
	if (SPI_GetITStatus(SPI2, SPI_I2S_IT_RXNE) != RESET){
		uint16_t raw = SPI_I2S_ReceiveData(SPI2);
		if(!cbuf_write(&pdm_buf, (raw >> 8) & 0xFF) || !cbuf_write(&pdm_buf, raw & 0xFF)){
			micbuf_ovf = true;
		}
	}
}

void WaveRecorderPDMFiltCallback(void){
	if(micbuf_ovf){
		halt_error(DEBUG_MICBUG_OVERFLOWMSG);
		for(;;){};
	}
	while(cbuf_elems_used(&pdm_buf) > PDM_BLOCK_LENGTH){
		uint8_t pdm_bytes[PDM_BLOCK_LENGTH];
		int16_t pcm_words[PCM_BLOCK_LENGTH];
		uint_fast32_t i;
		for(i = 0; i < NUMEL(pdm_bytes); i++){
			cbuf_read(&pdm_buf, &pdm_bytes[i]);
		}
		PDM_Filter_64_LSB(pdm_bytes, (uint16_t*) pcm_words, MIC_VOLUME, &Filter);
		WaveRecorderCallback(pcm_words, NUMEL(pcm_words));
	}
}

static void WaveRecorder_GPIO_Init(void){
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIO clocks */
	RCC_AHB1PeriphClockCmd(SPI_SCK_GPIO_CLK | SPI_MOSI_GPIO_CLK, ENABLE);

	/* Enable GPIO clocks */
	RCC_AHB1PeriphClockCmd(SPI_SCK_GPIO_CLK | SPI_MOSI_GPIO_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	/* SPI SCK pin configuration */
	GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
	GPIO_Init(SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

	/* Connect SPI pins to AF5 */
	GPIO_PinAFConfig(SPI_SCK_GPIO_PORT, SPI_SCK_SOURCE, SPI_SCK_AF);

	/* SPI MOSI pin configuration */
	GPIO_InitStructure.GPIO_Pin =  SPI_MOSI_PIN;
	GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
	GPIO_PinAFConfig(SPI_MOSI_GPIO_PORT, SPI_MOSI_SOURCE, SPI_MOSI_AF);
}

static void WaveRecorder_SPI_Init(void){
	I2S_InitTypeDef I2S_InitStructure;

	/* Enable the SPI clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);

	/* SPI configuration */
	SPI_I2S_DeInit(SPI2);
	I2S_InitStructure.I2S_AudioFreq = AUDIO_SAMPLE_RATE*2;
	I2S_InitStructure.I2S_Standard = I2S_Standard_LSB;
	I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
	I2S_InitStructure.I2S_CPOL = I2S_CPOL_High;
	I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
	I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
	/* Initialize the I2S peripheral with the structure above */
	I2S_Init(SPI2, &I2S_InitStructure);

	/* Enable the Rx buffer not empty interrupt */
	SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
}

static void WaveRecorder_NVIC_Init(void){
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
	/* Configure the SPI interrupt priority */
	NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
