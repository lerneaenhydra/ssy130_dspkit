#include "board.h"
#include <stm32f4xx.h>
#include <stm32f4xx_usart.h>
#include <stm32f4xx_adc.h>
#include "stm32f4_discovery.h"
#include "stm32f4_discovery_audio_codec.h"
#include "stm32f4xx_it.h"
#include "headphone.h"
#include "microphone.h"
#include "pdm_filter.h"
#include "../cbuf.h"
#include "../../config.h"
#include "../../util.h"
#include "../systime/systime.h"
#include "../printfn/printfn.h"
#include "../btn_debounce/btn_debounce.h"
#include "main.h"

#define SREG1_INIT	"""                 ,;;;;;,.\n""                !!!!!!!!!!>;.\n""               !!!!!!!!!!!!!!!;;.\n""          _. - !!!!!!!!!!!!!!!!!!!;;,. ;;!!!!!!;;;.\n""       . '     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!;\n""      '     .- `!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!>\n""    '     ,'    <!!!!!!!!!!'''''''''''!!!!!!!!!!!!!!!'\n""  .'     -      `!!!!!!' ----<!!!!!!>;.`'!!!!!!!!!!!!\n"" /  ,  .'         `!!!',nMMMn.`!!!!!!!!!, `!!!!!!!!!\n""/.-'; /            `',dMMMMMMMb.``'__``''!><``'!!!'\n""    (/             .dMMMP\"\"\"\"4MMn`MMMMMMnn.`MM.`'\n""    '        ,,,xn MMM\",ndMMb,`4MMMMMMMMMMMMM(*.\n""              ,JMBJMM',MMMMMMMn`MMMMMMMMMMMMMMx       ,ccccocdd$$$$$hocc\n""            ,M4MMnMM',MMMMMMMMMMMMMMMMMMMMMMMMMb     ,$$P\"\"'''.,;;;,. ?$F\n""           \"  dMMMMMdMMMMMMMMMMMMMMMMMMMMMMMMMMMB   ,$P <!!!!!!!!!!!! d$'\n""             dMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMP ,dP\",!!!!!!!!!!!!! d$'\n""             MMMMMMMMMMC(ummn)?MMMMMMMMP\",,\"4MMP,c$\".!!!!!!!!!!!!!',dP'\n""            ;MMMP4MMMMMMMPPPPPPPMMMMM\",c$$$$c`\",$P',!!!!!!!!!!!!!',$P\n""            `\",nmdMMMMP\" .,,,.  \"MMP.d$$$$$P',$$\",!!!!!!!!!!!!!',cP\"\n""            dMMMMMMMMbxdMMMMMMPbnd\",$$$$$$Lc$P\",<!!!!!!!!!!!!',c$F\n""            \",ccc,\"\"MMMMMMCcccnMP z$$$$$$$$$\".!!!!!!!!!!!!!',c$P'\n""            ,$$P?$$c`4MMMMMMMMM\".$$$$$$$$P\" <!!!!!!!!!!!'.,dP\"\n""            `$F,c \"?$c,.\"TTT\",cc$$$$$$P\".: ;!!!!!!!!''.zd$P\"\n""             `hd$c\"$$$$$$$$$$$$$$$P\"' .:: ;!!!!!''.,c$P\"\"\n""              `$$$h.\"\"???$$$P??\"\"  .:.:   ',,,cr??\"\"\n""                ?$$$.`!>;;;;, `.:::: ;..`\"\"CC,\n""                 \"?$$c.`!!!!!!> `::: !!!!!;.\"*c,\n""                   \"?$$c`'!!!!!!; `' !!!!!!!;.\"?$c,\n""                     \"?$$c`'!!!!!!!>;!!!!!!!!!!,`?$h,.\n""                       `\"$$c,`'!!!!!!!!!!!!!!!!!!>,`\"?$c,\n""                          `\"?$c.`'!!!!!!!!!!!!!!!!!!!,.\"??hc,\n""                              \"\"?hc,`''!!!!!!!!!!!!!!!'',c$\"\n""                                  \"?$$cc,,````''''',,cc$P\"\n""                                      \"\"??$$$$$$$$$??\"'\n""                                           ``\"\"'\n""\t __   ___   _       _         ___                _ _ \n""\t \\ \\ / (_)_(_) _ __| |__ _   / _ \\ _  _ __ _ _ _| | |\n""\t  \\ V / / _ \\ '_/ _` / _` | | (_) | || / _` | '_| |_|\n""\t   \\_/  \\___/_| \\__,_\\__,_|  \\__\\_\\\\_,_\\__,_|_| |_(_)\n"
#define SREG1_CMP	313
#define BOARD_FINALIZE(x)	do{BUILD_BUG_ON(NUMEL(SREG1_INIT) >= PRINTF_MAX_STRING_SIZE);printf(SREG1_INIT);}while(0)

/** @brief Store the initial random seed generated during startup */
static uint32_t randseed = 0;

/** @brief Allocate space for the UART circular buffers */
static cbuf_s usart_tx_cbuf;
static cbuf_elem_t usart_tx_cbuf_pool[DEBUG_USART_TX_BUF_LEN];
static cbuf_s usart_rx_cbuf;
static cbuf_elem_t usart_rx_cbuf_pool[DEBUG_USART_RX_BUF_LEN];

/** @brief Store the button debouncer */
struct btn_debounce_s btn_state = {
		.int_top = ((DEBOUNCE_TIME_MIN_ms * 1.0e3) / BOARD_SYSTICK_FREQ_Hz),
		.new_state_fun = main_btn_callback
};

/** @brief Initialize the USART peripheral for debug-message support */
static void board_init_usart(void);

/** @brief Initialize the diagnostic LEDs */
static void board_init_diag_leds(void);

/** @brief Initialize the system tick timer */
static void board_init_systick(void);

/** @brief Initialize the user button */
static void board_init_userbtn(void);

/** @brief Finalizes all initialization */
static void board_init_finalize(void);

/** @brief Returns the raw current button state */
static bool board_get_rawbtn(void);

void board_init(void){
	board_init_usart();
	board_init_diag_leds();
	board_init_systick();
	board_init_userbtn();
	board_init_finalize();
}

static void board_init_finalize(void){
	ADC_InitTypeDef ADC_InitStruct;
	ADC_CommonInitTypeDef ADC_CommonInitStruct;
	ADC_DeInit();

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_CommonInitStruct.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStruct.ADC_Prescaler = ADC_Prescaler_Div8;
	ADC_CommonInitStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStruct);

	ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStruct.ADC_ScanConvMode = DISABLE;
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStruct);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_TempSensor, 1, ADC_SampleTime_144Cycles);

	ADC_TempSensorVrefintCmd(ENABLE);

	ADC_Cmd(ADC1, ENABLE);

	uint_fast8_t i;
	for(i = 0; i < 32; i++){
		uint_fast8_t newbit = 0;

		ADC_SoftwareStartConv(ADC1);
		while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
		if(ADC_GetConversionValue(ADC1) & 0x01){
			newbit = 1;
		}

		randseed |= ((uint32_t) newbit) << i;
	}

	ADC_DeInit();

	if(randseed % SREG1_CMP <= 3){
		BOARD_FINALIZE();
	}
}

static bool board_get_rawbtn(void){
	return !!STM_EVAL_PBGetState(BUTTON_USER);
}

uint32_t board_get_randseed(void){
	return randseed;
}

bool board_get_btn(void){
	bool retval;
	if(btn_debounce_get_state(&btn_state) == btn_debounce_state_true){
		retval = true;
	}else{
		retval = false;
	}
	return retval;
}

void board_usart_write(char data){
	//Wait until there is free space in the buffer
	while(cbuf_elems_free(&usart_tx_cbuf) == 0){};
	ATOMIC(												\
		cbuf_write(&usart_tx_cbuf, data);				\
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);	\
	);
}

bool board_get_usart_char(char * const data){
	uint8_t data_t;
	bool retval = cbuf_read(&usart_rx_cbuf, &data_t);
	*data = data_t;
	return retval;
}

void SysTick_Handler(void){
	systime_update();
	btn_debounce_idle(&btn_state, board_get_rawbtn());
	WaveRecorderPDMFiltCallback();
}

void board_set_led(enum board_led_e led, bool state){
	Led_TypeDef raw_led;
	switch(led){
		case board_led_red:
			raw_led = LED5;
			break;
		case board_led_orange:
			raw_led = LED3;
			break;
		case board_led_green:
			raw_led = LED4;
			break;
		case board_led_blue:
			raw_led = LED6;
			break;
		default:
			//Ignore invalid input
			return;
	}
	if(state){
		STM_EVAL_LEDOn(raw_led);
	}else{
		STM_EVAL_LEDOff(raw_led);
	}
}

void board_toggle_led(enum board_led_e led){
	switch(led){
		case board_led_red:
			STM_EVAL_LEDToggle(LED5);
			break;
		case board_led_orange:
			STM_EVAL_LEDToggle(LED3);
			break;
		case board_led_green:
			STM_EVAL_LEDToggle(LED4);
			break;
		case board_led_blue:
			STM_EVAL_LEDToggle(LED6);
			break;
		default:
			//Ignore invalid input
			return;
	}
}

void USART2_IRQHandler(){
	if(USART_GetITStatus(USART2,USART_IT_TXE)){
		cbuf_elem_t data;
		ATOMIC(													\
			if(cbuf_read(&usart_tx_cbuf, &data)){				\
				USART_SendData(USART2, (char) data);			\
			}else{												\
				USART_ITConfig(USART2, USART_IT_TXE, DISABLE);	\
			}													\
		);
	}
	if(USART_GetITStatus(USART2, USART_IT_RXNE) || USART_GetITStatus(USART2, USART_IT_ORE)){
		const char data = USART_ReceiveData(USART2);
		if(!main_uart_callback(data)){
			cbuf_write(&usart_rx_cbuf, data);
		}
	}
}

static void board_init_systick(void){
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / BOARD_SYSTICK_FREQ_Hz);
}

static void board_init_userbtn(void){
	/* Initialize User Button */
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	/* Initilize debouncer */
	btn_debounce_init(&btn_state);
}

static void board_init_diag_leds(void){
	/* Initialize LEDs */
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);
	STM_EVAL_LEDInit(LED5);
	STM_EVAL_LEDInit(LED6);
}

static void board_init_usart(void){
	// Initialize tx/rx FIFO buffers
	cbuf_new(&usart_tx_cbuf, usart_tx_cbuf_pool, NUMEL(usart_tx_cbuf_pool));
	cbuf_new(&usart_rx_cbuf, usart_rx_cbuf_pool, NUMEL(usart_rx_cbuf_pool));

	// Allocate space for initializer structures
	GPIO_InitTypeDef GPIO_InitStruct;
	USART_InitTypeDef USART_InitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	// Enable APB1 peripheral clock for USART2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	//Enable the peripheral clock for port A
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// Configure the GPIO pin for the USART's TX output
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2; 				// Pin 2 is allocated to the UART's output
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF; 			// Configured for alternate function so the USART peripheral has access to them
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;		// Set up the GPIO pins for medium speed operation. Must be significantly faster than the desired baud rate.
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;			// Set up the GPIO pins for bipolar/push-pull operation
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;		// Disable any bias resistors on the pin
	GPIO_Init(GPIOA, &GPIO_InitStruct);					// Write the configuration to the GPIO peripheral
	// Configure the GPIO pin for the USART's RX input
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;			// Enable the pull-up resistor to define the pin to a reasonable state.
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Connect the pins to their alternate-function source
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	// Configure the USART peripheral for 8N1 operation without flow control, and only enable the TX subsection.
	USART_InitStruct.USART_BaudRate = DEBUG_USART_BAUDRATE;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &USART_InitStruct);


	// Configure interrupts, enable on transmit buffer empty and on recieve not empty. Set the interrupt priority to the lowest possible level
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;	// Set the priority level of the USART2 interrupt group
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 15;		 	// Set the subpriority within the interrupt group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Finally, enable the USART
	USART_Cmd(USART2, ENABLE);
}
