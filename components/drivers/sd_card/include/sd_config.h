#ifndef __SD_CONFIG_H
#define __SD_CONFIG_H

#include "global_config.h"

// Pin
#define TF_SPI_SCLK_PIN                     (CONFIG_TF_SPI_PIN_SCLK)
#define TF_SPI_MOSI_PIN                     (CONFIG_TF_SPI_PIN_MOSI)
#define TF_SPI_MISO_PIN                     (CONFIG_TF_SPI_PIN_MISO)
#define TF_SPI_CSXX_PIN                     (CONFIG_TF_SPI_PIN_CS)

// GPIOHS simulate numbers
#define TF_SPI_SCLK_HS_NUM                  (CONFIG_TF_SPI_GPIOHS_NUM_SCLK)
#define TF_SPI_MOSI_HS_NUM                  (CONFIG_TF_SPI_GPIOHS_NUM_MOSI)
#define TF_SPI_MISO_HS_NUM                  (CONFIG_TF_SPI_GPIOHS_NUM_MISO)
#define TF_SPI_CSXX_HS_NUM                  (CONFIG_TF_SPI_GPIOHS_NUM_CS)



#endif


