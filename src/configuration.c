#include "configuration.h"

/* ===== Глобальные параметры ===== */
uint16_t tim32_top = 320;
uint8_t dac_div = 31;
uint16_t values_quantity = 20;
uint32_t word_src[20];
uint16_t max_value = 3000;
uint16_t min_value = 1000;

uint8_t signal_form = 0x00;
uint8_t freq = 0x00;
uint8_t start_ampl = 0x00;
uint8_t finish_ampl = 0x00;

/* ===== Хэндлы ===== */
SPI_HandleTypeDef hspi0;
TIMER32_HandleTypeDef htimer32;
TIMER32_CHANNEL_HandleTypeDef htimer32_channel;
DMA_InitTypeDef hdma;
DMA_ChannelHandleTypeDef hdma_ch0;
DAC_HandleTypeDef hdac1;

/* ====================== CLOCK ====================== */
void SystemClock_Config(void)
{
    PCC_InitTypeDef PCC_OscInit = {0};

    PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
    PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
    PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
    PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_LSI32K;
    PCC_OscInit.AHBDivider = 0;
    PCC_OscInit.APBMDivider = 0;
    PCC_OscInit.APBPDivider = 0;
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 8;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;

    HAL_PCC_Config(&PCC_OscInit);
}

/* ====================== TIMER ====================== */
void Timer32_Init(void)
{
    htimer32.Instance = TIMER32_1;
    htimer32.Top = tim32_top;
    htimer32.State = TIMER32_STATE_DISABLE;
    htimer32.Clock.Source = TIMER32_SOURCE_PRESCALER;
    htimer32.Clock.Prescaler = 0;
    htimer32.InterruptMask = 0;
    htimer32.CountMode = TIMER32_COUNTMODE_FORWARD;
    HAL_Timer32_Init(&htimer32);

    htimer32_channel.TimerInstance = htimer32.Instance;
    htimer32_channel.ChannelIndex = TIMER32_CHANNEL_0;
    htimer32_channel.PWM_Invert = TIMER32_CHANNEL_NON_INVERTED_PWM;
    htimer32_channel.Mode = TIMER32_CHANNEL_MODE_COMPARE;
    htimer32_channel.CaptureEdge = TIMER32_CHANNEL_CAPTUREEDGE_RISING;
    htimer32_channel.OCR = htimer32.Top / 2;
    htimer32_channel.Noise = TIMER32_CHANNEL_FILTER_OFF;
    HAL_Timer32_Channel_Init(&htimer32_channel);
}

/* ====================== DMA ====================== */

static void DMA_CH0_InternalInit(DMA_InitTypeDef *hdma)
{
    hdma_ch0.dma = hdma;

    hdma_ch0.ChannelInit.Channel = DMA_CHANNEL_0;
    hdma_ch0.ChannelInit.Priority = DMA_CHANNEL_PRIORITY_VERY_HIGH;

    hdma_ch0.ChannelInit.ReadMode = DMA_CHANNEL_MODE_MEMORY;
    hdma_ch0.ChannelInit.ReadInc = DMA_CHANNEL_INC_ENABLE;
    hdma_ch0.ChannelInit.ReadSize = DMA_CHANNEL_SIZE_WORD;
    hdma_ch0.ChannelInit.ReadBurstSize = 2;
    hdma_ch0.ChannelInit.ReadRequest = DMA_CHANNEL_TIMER32_1_REQUEST;
    hdma_ch0.ChannelInit.ReadAck = DMA_CHANNEL_ACK_DISABLE;

    hdma_ch0.ChannelInit.WriteMode = DMA_CHANNEL_MODE_PERIPHERY;
    hdma_ch0.ChannelInit.WriteInc = DMA_CHANNEL_INC_DISABLE;
    hdma_ch0.ChannelInit.WriteSize = DMA_CHANNEL_SIZE_WORD;
    hdma_ch0.ChannelInit.WriteBurstSize = 2;
    hdma_ch0.ChannelInit.WriteRequest = DMA_CHANNEL_TIMER32_1_REQUEST;
    hdma_ch0.ChannelInit.WriteAck = DMA_CHANNEL_ACK_ENABLE;
}

void DMA_Init(void)
{
    hdma.Instance = DMA_CONFIG;
    hdma.CurrentValue = DMA_CURRENT_VALUE_ENABLE;

    if (HAL_DMA_Init(&hdma) != HAL_OK)
        xprintf("DMA_Init Error\n");

    DMA_CH0_InternalInit(&hdma);
}

/* ====================== SPI ====================== */

void SPI0_Init(void)
{
    hspi0.Instance = SPI_0;

    hspi0.Init.SPI_Mode = HAL_SPI_MODE_SLAVE;
    hspi0.Init.CLKPhase = SPI_PHASE_OFF;
    hspi0.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi0.Init.ThresholdTX = 4;

    if (HAL_SPI_Init(&hspi0) != HAL_OK)
        xprintf("SPI_Init_Error\n");
}

/* ====================== DAC ====================== */

void DAC_Init(void)
{
    hdac1.Instance = ANALOG_REG;

    hdac1.Instance_dac = HAL_DAC0;
    hdac1.Init.DIV = dac_div;
    hdac1.Init.EXTRef = DAC_EXTREF_ON;
    hdac1.Init.EXTClb = DAC_EXTCLB_DACREF;

    HAL_DAC_Init(&hdac1);
}

/* ====================== GPIO ====================== */

void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_PCC_GPIO_0_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_OUTPUT;
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_UP;
    HAL_GPIO_Init(GPIO_0, &GPIO_InitStruct);
}