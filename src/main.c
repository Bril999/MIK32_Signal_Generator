#include "mik32_hal.h"
#include "mik32_hal_timer32.h"
#include "mik32_hal_dma.h"
#include "mik32_hal_dac.h"
#include "uart_lib.h"
#include "xprintf.h"

#include "mik32_hal_spi.h"
#include "mik32_hal_irq.h"
#include "mik32_hal_pcc.h"

SPI_HandleTypeDef hspi0;

uint8_t slave_output[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t slave_input[8]  = {0, 0, 0, 0, 0, 0, 0, 0};
uint16_t tim32_top = 320;       //значение, до которого всегда считает таймер
uint8_t dac_div = 31;           //делитель для ЦАП
uint16_t values_quantity = 20; //количество элементов в массиве
uint32_t word_src[20];          //массив данных о сигнале

TIMER32_HandleTypeDef htimer32;
TIMER32_CHANNEL_HandleTypeDef htimer32_channel;
DMA_InitTypeDef hdma;
DMA_ChannelHandleTypeDef hdma_ch0;
DAC_HandleTypeDef hdac1;

void SystemClock_Config(void);
static void Timer32_Init(void);
static void DMA_CH0_Init(DMA_InitTypeDef *hdma);
static void DMA_Init(void);
static void SPI0_Init(void);
static void DAC_Init(void);
void GPIO_Init();

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    DMA_Init();
    UART_Init(UART_0, 3333, UART_CONTROL1_TE_M | UART_CONTROL1_M_8BIT_M, 0, 0);
    DAC_Init();
    GPIO_Init();

    Timer32_Init();

    HAL_Timer32_Channel_Enable(&htimer32_channel);
    HAL_Timer32_Value_Clear(&htimer32);
    HAL_Timer32_Start(&htimer32);
    

    HAL_EPIC_MaskLevelSet(HAL_EPIC_SPI_0_MASK);
    HAL_IRQ_EnableInterrupts();

    
    xprintf("MIK32 SPI Slave (IRQ) example start\n");

    SPI0_Init();

    /* Включаем SPI один раз и не выключаем.
       HAL_SPI_Init() не обязательно включает модуль; включаем явно. */
    HAL_SPI_Enable(&hspi0);

    for (int i = 0; i < values_quantity; i++) word_src[i] = (4095 * i) / (values_quantity - 1); //повыносить в переменные

    while (1)
    {
        HAL_DMA_Start(&hdma_ch0, (void *)&word_src, (void *)&hdac1.Instance_dac->VALUE, sizeof(word_src) - 1); 
        if (HAL_DMA_Wait(&hdma_ch0, 10 * DMA_TIMEOUT_DEFAULT) != HAL_OK); //обязательно для корректной работы DMA
        /* Если SPI готов — начинаем обмен в прерывном режиме.
           HAL_SPI_Exchange_IT установит hspi0.State != READY пока идёт обмен. */
        if (hspi0.State == HAL_SPI_STATE_READY)
        {
            HAL_StatusTypeDef SPI_Status = HAL_SPI_Exchange_IT(&hspi0, slave_output, slave_input, sizeof(slave_output));
            if (SPI_Status != HAL_OK)
            {
                /* Что-то пошло не так — сбросим ошибки и вернём состояние */
                xprintf("HAL_SPI_Exchange_IT returned error\n");
                HAL_SPI_ClearError(&hspi0);
                hspi0.State = HAL_SPI_STATE_READY;
            }
        }

        /* Обработка завершённого обмена */
        if (hspi0.State == HAL_SPI_STATE_END)
        {
            xprintf("SPI IRQ: RX completed:\n");
            for (int i = 0; i < 8; i++)
            {
                xprintf(" slave_input[%d] = %02X\n", i, slave_input[i]);
            }
            xprintf("\n");

            /* Подготовиться к следующему обмену */
            hspi0.State = HAL_SPI_STATE_READY;
        }

        /* Обработка ошибок */
        if (hspi0.State == HAL_SPI_STATE_ERROR)
        {
            xprintf("SPI_Error: OVR %d, MODF %d\n",
                    (hspi0.ErrorCode & HAL_SPI_ERROR_OVR) ? 1 : 0,
                    (hspi0.ErrorCode & HAL_SPI_ERROR_MODF) ? 1 : 0);

            HAL_SPI_ClearError(&hspi0);
        /* В примере референса SPI временно выключают — можно сделать так же */
            HAL_SPI_Disable(&hspi0);
            HAL_DelayMs(1);
            HAL_SPI_Enable(&hspi0);
            hspi0.State = HAL_SPI_STATE_READY;
        }
    }
}

void SystemClock_Config(void)
{
    PCC_InitTypeDef PCC_OscInit = {0};

    PCC_OscInit.OscillatorEnable = PCC_OSCILLATORTYPE_ALL;
    PCC_OscInit.FreqMon.OscillatorSystem = PCC_OSCILLATORTYPE_OSC32M;
    PCC_OscInit.FreqMon.ForceOscSys = PCC_FORCE_OSC_SYS_UNFIXED;
    PCC_OscInit.FreqMon.Force32KClk = PCC_FREQ_MONITOR_SOURCE_OSC32K;
    PCC_OscInit.AHBDivider = 0;  //делитель частоты линии AHB
    PCC_OscInit.APBMDivider = 0; //делитель частоты линии APB_M
    PCC_OscInit.APBPDivider = 0; //делитель частоты линии APB_P
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 8;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;

    HAL_PCC_Config(&PCC_OscInit);
}

static void Timer32_Init(void)
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

static void DMA_CH0_Init(DMA_InitTypeDef *hdma)
{
    hdma_ch0.dma = hdma;

    /* Настройки канала */
    hdma_ch0.ChannelInit.Channel = DMA_CHANNEL_0;
    hdma_ch0.ChannelInit.Priority = DMA_CHANNEL_PRIORITY_VERY_HIGH;

    hdma_ch0.ChannelInit.ReadMode = DMA_CHANNEL_MODE_MEMORY;
    hdma_ch0.ChannelInit.ReadInc = DMA_CHANNEL_INC_ENABLE;
    hdma_ch0.ChannelInit.ReadSize = DMA_CHANNEL_SIZE_WORD; /* data_len должно быть кратно read_size */
    hdma_ch0.ChannelInit.ReadBurstSize = 2;                /* read_burst_size должно быть кратно read_size */
    hdma_ch0.ChannelInit.ReadRequest = DMA_CHANNEL_TIMER32_1_REQUEST;
    hdma_ch0.ChannelInit.ReadAck = DMA_CHANNEL_ACK_DISABLE;

    hdma_ch0.ChannelInit.WriteMode = DMA_CHANNEL_MODE_PERIPHERY;
    hdma_ch0.ChannelInit.WriteInc = DMA_CHANNEL_INC_DISABLE;
    hdma_ch0.ChannelInit.WriteSize = DMA_CHANNEL_SIZE_WORD; /* data_len должно быть кратно write_size */
    hdma_ch0.ChannelInit.WriteBurstSize = 2;                /* write_burst_size должно быть кратно read_size */
    hdma_ch0.ChannelInit.WriteRequest = DMA_CHANNEL_TIMER32_1_REQUEST;
    hdma_ch0.ChannelInit.WriteAck = DMA_CHANNEL_ACK_ENABLE;
}

static void DMA_Init(void)
{

    /* Настройки DMA */
    hdma.Instance = DMA_CONFIG;
    hdma.CurrentValue = DMA_CURRENT_VALUE_ENABLE;
    if (HAL_DMA_Init(&hdma) != HAL_OK)
    {
        xprintf("DMA_Init Error\n");
    }

    /* Инициализация канала */
    DMA_CH0_Init(&hdma);
}

static void SPI0_Init(void)
{
    hspi0.Instance = SPI_0;

    hspi0.Init.SPI_Mode = HAL_SPI_MODE_SLAVE;

    hspi0.Init.CLKPhase = SPI_PHASE_OFF;       // CPHA = 0
    hspi0.Init.CLKPolarity = SPI_POLARITY_LOW; // CPOL = 0
    hspi0.Init.ThresholdTX = 4;                //Уровень, при котором TX_FIFO считается незаполненным и формируется прерывание TX_FIFO_NOT_full

    if (HAL_SPI_Init(&hspi0) != HAL_OK)
    {
        xprintf("SPI_Init_Error\n");
    }
}

void trap_handler()
{
    if (EPIC_CHECK_SPI_0())
    {
        HAL_SPI_IRQHandler(&hspi0);
    }

    /* Сброс прерываний (очистка EPIC статуса) */
    HAL_EPIC_Clear(0xFFFFFFFF);
}

static void DAC_Init(void)
{
    hdac1.Instance = ANALOG_REG;

    hdac1.Instance_dac = HAL_DAC0;
    /* Выбор делителя частоты тактирования ЦАП, определяется как F_ЦАП=F_IN/(DIV+1) */
    hdac1.Init.DIV = dac_div;    // 1 МГц
    /* Выбор источника опорного напряжения: «1» - внешний; «0» - встроенный */                 
    hdac1.Init.EXTRef = DAC_EXTREF_ON;
    /* Выбор источника внешнего опорного напряжения: «1» - внешний вывод; «0» - настраиваемый ОИН */
    hdac1.Init.EXTClb = DAC_EXTCLB_DACREF;
    
    HAL_DAC_Init(&hdac1);
}

void GPIO_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Включаем тактирование только для GPIO_0 (P0.9 находится на GPIO_0)
    __HAL_PCC_GPIO_0_CLK_ENABLE();

    // Настраиваем пин 9
    GPIO_InitStruct.Pin = GPIO_PIN_9; //используем P0.9
    GPIO_InitStruct.Mode = HAL_GPIO_MODE_GPIO_OUTPUT; // режим GPIO - вывод
    GPIO_InitStruct.Pull = HAL_GPIO_PULL_UP; //подтяжка к питанию
    HAL_GPIO_Init(GPIO_0, &GPIO_InitStruct);
}