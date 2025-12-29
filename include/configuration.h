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

/* === Глобальные параметры === */
#define SIN_LUT_SIZE 100
extern const uint16_t sin_lut[SIN_LUT_SIZE];
extern uint16_t tim32_top;
extern uint8_t dac_div;
extern uint16_t values_quantity;
extern uint32_t word_src[20];

extern uint8_t signal_form;
extern uint16_t freq;
extern uint8_t amplitude;

/* === Хэндлы периферии === */
extern SPI_HandleTypeDef hspi0;
extern TIMER32_HandleTypeDef htimer32;
extern TIMER32_CHANNEL_HandleTypeDef htimer32_channel;
extern DMA_InitTypeDef hdma;
extern DMA_ChannelHandleTypeDef hdma_ch0;
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