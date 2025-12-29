#include "configuration.h"

//#define PI 3.14159265358979323846

uint8_t slave_output[4];
uint8_t slave_input[4];
uint8_t dma_active_channel = 0;

void parse_SPI_parametrs(void);
void generate_signal(uint8_t signal);

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
    
    xprintf("MIK32 SPI Slave (IRQ) example start\n");

    SPI0_Init();

    /* Включаем SPI один раз и не выключаем.
       HAL_SPI_Init() не обязательно включает модуль; включаем явно. */
    HAL_SPI_Enable(&hspi0);

    for (int i = min_value; i < values_quantity; i++) {
        word_src[i] = (max_value * i) / (values_quantity - 1);
        //xprintf("%d \n", word_src[i]);
    }
    HAL_EPIC_MaskLevelSet(HAL_EPIC_SPI_0_MASK | HAL_EPIC_DMA_MASK);
    HAL_IRQ_EnableInterrupts();
    //xprintf("status1 = %d", &hdma_ch0);
    HAL_DMA_Start(&hdma_ch0, (void *)&word_src, (void *)&hdac1.Instance_dac->VALUE, values_quantity*sizeof(uint16_t) - 1);
    //xprintf("status2 = %d", &hdma_ch0);

    while (1)
    {
        // HAL_DMA_Start(&hdma_ch0, (void *)&word_src, (void *)&hdac1.Instance_dac->VALUE, values_quantity*sizeof(uint16_t) - 1);
        // if (HAL_DMA_Wait(&hdma_ch0, DMA_TIMEOUT_DEFAULT) != HAL_OK); //обязательно для корректной работы DMA
        //xprintf("status = %d\n", HAL_DMA_GetChannelReadyStatus(&hdma_ch0));
        if (hspi0.State == HAL_SPI_STATE_READY)
        {
            HAL_StatusTypeDef SPI_Status = HAL_SPI_Exchange_IT(&hspi0, slave_output, slave_input, sizeof(slave_output));
            if (SPI_Status != HAL_OK)
            {
                HAL_SPI_ClearError(&hspi0);
                hspi0.State = HAL_SPI_STATE_READY;
            }
        }
    }
}

void trap_handler()
{   
    if (EPIC_CHECK_SPI_0())
    {
        xprintf("SPI Check\n");
        HAL_SPI_IRQHandler(&hspi0);
        parse_SPI_parametrs();
        generate_signal(signal_form);

        if (hspi0.State == HAL_SPI_STATE_END)
        {
            /* Подготовиться к следующему обмену */
            hspi0.State = HAL_SPI_STATE_READY;
        }
    }
    
    if (EPIC_CHECK_DMA())
    {
        if (dma_active_channel == 0)
        {
            //xprintf("status1 = %d\n", HAL_DMA_GetChannelIrq(&hdma_ch1));
            HAL_DMA_ClearLocalIrq(&hdma);
            //xprintf("status2 = %d\n", HAL_DMA_GetChannelIrq(&hdma_ch1));
            // for (uint32_t i = 0; i < 4; i++)
            // {
            //     xprintf("elem = %d\n", word_dst[i]);
            // }
            HAL_DMA_Start(&hdma_ch1, (void *)&word_src, (void *)&hdac1.Instance_dac->VALUE, values_quantity*sizeof(uint16_t) - 1);
            dma_active_channel = 1;
        }
        else if (dma_active_channel == 1)
        {
            //xprintf("status3 = %d\n", HAL_DMA_GetChannelIrq(&hdma_ch1));
            HAL_DMA_ClearLocalIrq(&hdma);
            //xprintf("status4 = %d\n", HAL_DMA_GetChannelIrq(&hdma_ch1));
            // for (uint32_t i = 0; i < 4; i++)
            // {
            //     xprintf("elem = %d\n", word_dst[i]);
            // }
            HAL_DMA_Start(&hdma_ch0, (void *)&word_src, (void *)&hdac1.Instance_dac->VALUE, values_quantity*sizeof(uint16_t) - 1);
            dma_active_channel = 0;
        }
    }
    /* Сброс прерываний (очистка EPIC статуса) */
    HAL_EPIC_Clear(0xFFFFFFFF);
}

void parse_SPI_parametrs(void) {
    signal_form = slave_input[0];
    freq = slave_input[1];
    start_ampl = slave_input[2];
    finish_ampl = slave_input[3];
}

void generate_signal(uint8_t signal) {
    switch (signal)
    {
    case 0x01: //Пила
        for (int i = min_value; i < values_quantity; i++) word_src[i] = (max_value * i) / (values_quantity - 1);
        break;
    case 0x02: //Треугольник
        for (int i = min_value; i < values_quantity; i++) word_src[i] = (i < values_quantity/2)
              ?  (max_value * i)/(values_quantity/2)
              :  (max_value * (values_quantity-i))/(values_quantity/2);
        break;
    case 0x03: //Синус
        //for (int i = 0; i < values_quantity; i++) word_src[i] = (sin(2*M_PI*i/values_quantity)+1) * 2047;
        break;
    case 0x04: //Меандр
        for (int i = min_value; i < values_quantity; i++) word_src[i] = (i < values_quantity/2) ? max_value : min_value;
        break;
    default:
        break;
    }
}
