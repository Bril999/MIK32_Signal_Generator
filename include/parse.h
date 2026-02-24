#ifndef PARSE_H
#define PARSE_H

#include "mik32_hal.h"

#define AMPL_SIZE 10

void parse_SPI_parametrs(void);

extern const uint16_t max_ampl_table[AMPL_SIZE];
extern const uint16_t min_ampl_table[AMPL_SIZE];

#endif