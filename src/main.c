#include "mik32_hal_spi.h"
#include "mik32_hal_irq.h"
#include "mik32_hal.h"
#include "mik32_hal_dac.h"
#include "uart_lib.h"
#include "xprintf.h"
#include "mik32_hal_pcc.h"

SPI_HandleTypeDef hspi0;

uint8_t slave_output[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t slave_input[8]  = {0, 0, 0, 0, 0, 0, 0, 0};

DAC_HandleTypeDef hdac1;

void SystemClock_Config(void);
static void SPI0_Init(void);
static void DAC_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    DAC_Init();

    HAL_EPIC_MaskLevelSet(HAL_EPIC_SPI_0_MASK);
    HAL_IRQ_EnableInterrupts();

    UART_Init(UART_0, 3333, UART_CONTROL1_TE_M | UART_CONTROL1_M_8BIT_M, 0, 0);
    xprintf("MIK32 SPI Slave (IRQ) example start\n");

    SPI0_Init();

    /* Включаем SPI один раз и не выключаем.
       HAL_SPI_Init() не обязательно включает модуль; включаем явно. */
    HAL_SPI_Enable(&hspi0);

    while (1)
    {
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
            for (uint16_t DAC_Value = 0; DAC_Value <= 2047; DAC_Value += 500)
            {
                HAL_DAC_SetValue(&hdac1, DAC_Value);

                // HAL_DelayMs(50);
            }
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
    PCC_OscInit.AHBDivider = 0;
    PCC_OscInit.APBMDivider = 0;
    PCC_OscInit.APBPDivider = 0;
    PCC_OscInit.HSI32MCalibrationValue = 128;
    PCC_OscInit.LSI32KCalibrationValue = 8;
    PCC_OscInit.RTCClockSelection = PCC_RTC_CLOCK_SOURCE_AUTO;
    PCC_OscInit.RTCClockCPUSelection = PCC_CPU_RTC_CLOCK_SOURCE_OSC32K;

    HAL_PCC_Config(&PCC_OscInit);
}

static void SPI0_Init(void)
{
    hspi0.Instance = SPI_0;

    hspi0.Init.SPI_Mode = HAL_SPI_MODE_SLAVE;

    hspi0.Init.CLKPhase = SPI_PHASE_OFF;       // CPHA = 0
    hspi0.Init.CLKPolarity = SPI_POLARITY_LOW; // CPOL = 0
    hspi0.Init.ThresholdTX = 4;

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
    hdac1.Init.DIV = 31;    // 1 МГц
    /* Выбор источника опорного напряжения: «1» - внешний; «0» - встроенный */                 
    hdac1.Init.EXTRef = DAC_EXTREF_OFF;
    /* Выбор источника внешнего опорного напряжения: «1» - внешний вывод; «0» - настраиваемый ОИН */
    hdac1.Init.EXTClb = DAC_EXTCLB_DACREF;
    
    HAL_DAC_Init(&hdac1);
}