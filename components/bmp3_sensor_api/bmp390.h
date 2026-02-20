#ifndef BMP390_H
#define BMP390_H

#include "bmp3.h"

uint8_t bmp390_spi_init(void);
int8_t bmp390_get_data(struct bmp3_data *data);

#endif // BMP390_H
