#include "configuration.h"
#include "parse.h"

uint8_t slave_output[4];
uint8_t slave_input[4];

const uint16_t max_ampl_table[MAX_AMPL_SIZE] = 
{
    300,
    750,
    1150,
    1550,
    1950,
    2400,
    2850,
    3300,
    3700,
    4095
};

void parse_SPI_parametrs(void) {
    signal_form = slave_input[0];
    freq = slave_input[1];
    start_ampl = slave_input[2];
    finish_ampl = max_ampl_table[slave_input[3] - 1];
}