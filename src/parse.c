#include "configuration.h"
#include "parse.h"

uint8_t slave_output[4];
uint8_t slave_input[4];

const uint16_t max_ampl_table[AMPL_SIZE] = //таблица констант для верхнего значения амплитуды
{
    300, //200мВ
    750, //400мВ
    1150, //600мВ
    1550, //800мВ
    1950, //1000мВ
    2400, //1200мВ
    2850, //1400мВ
    3300, //1600мВ
    3700, //1800мВ
    4095 //2000мВ
};

const uint16_t min_ampl_table[AMPL_SIZE] = //таблица констант для нижнего значения амплитуды
{
    0, //0мВ
    600, //200мВ
    1000, //400мВ
    1450, //600мВ
    1900, //800мВ
    2300, //1000мВ
    2750, //1200мВ
    3200, //1400мВ
    3600, //1600мВ
    3990 //1600мВ
};

void parse_SPI_parametrs(void) {
    signal_form = slave_input[0];
    freq = slave_input[1];
    start_ampl = min_ampl_table [slave_input[2]];
    finish_ampl = max_ampl_table[slave_input[3] - 1];
}