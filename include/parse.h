#ifndef PARSE_H
#define PARSE_H

#include "mik32_hal.h"

#define MAX_AMPL_SIZE 10

void parse_SPI_parametrs(void);

extern const uint16_t max_ampl_table[MAX_AMPL_SIZE];

#endif