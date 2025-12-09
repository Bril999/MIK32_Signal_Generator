#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "mik32_hal.h"
#include "mik32_hal_timer32.h"
#include "mik32_hal_dma.h"
#include "mik32_hal_dac.h"
#include "mik32_hal_spi.h"
#include "mik32_hal_pcc.h"
#include "uart_lib.h"
#include "xprintf.h"
#include "mik32_hal_irq.h"
#include "mik32_hal_gpio.h"
//#include <math.h>

/* === Глобальные параметры === */
#define MAX_VALUES 128

extern uint16_t tim32_top;
extern uint8_t dac_div;
extern uint8_t values_quantity;
extern uint16_t word_src[MAX_VALUES];
extern uint16_t max_value;
extern uint16_t min_value;
extern volatile uint8_t dma_last_started;     // 0 -> a был запущен, 1 -> b был запущен
extern volatile uint8_t dma_busy;             // защита от повторного старта

extern uint8_t signal_form;
extern uint8_t freq;
extern uint8_t start_ampl;
extern uint8_t finish_ampl;

/* === Хэндлы периферии === */
extern SPI_HandleTypeDef hspi0;
extern TIMER32_HandleTypeDef htimer32;
extern TIMER32_CHANNEL_HandleTypeDef htimer32_channel;
extern DMA_InitTypeDef hdma;
extern DMA_ChannelHandleTypeDef hdma_ch0;
extern DMA_ChannelHandleTypeDef hdma_ch1;
extern DAC_HandleTypeDef hdac1;

/* === Инициализации === */
void SystemClock_Config(void);
void Timer32_Init(void);
void DMA_Init(void);
void SPI0_Init(void);
void DAC_Init(void);
void GPIO_Init(void);

/* === Функции логики === */
void parse_SPI_parametrs(void);
void generate_signal(uint8_t signal);

#endif