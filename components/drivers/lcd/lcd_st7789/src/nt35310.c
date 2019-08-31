#include "gpiohs.h"
#include "nt35310.h"
#include "spi.h"
#include "unistd.h"

#include "global_config.h"

/* clang-format off */
#define SPI_CHANNEL             0
#define SPI_SLAVE_SELECT        3
/* clang-format on */

static void set_dcx_control(void)
{
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_DCX, GPIO_PV_LOW);
}

static void set_dcx_data(void)
{
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_DCX, GPIO_PV_HIGH);
}

void tft_hard_init(void)
{
    /* setup lcd_dcx pin */
    gpiohs_set_drive_mode(CONFIG_LCD_GPIOHS_DCX, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_DCX, GPIO_PV_HIGH);

    /* setup lcd_rst pin */
    gpiohs_set_drive_mode(CONFIG_LCD_GPIOHS_RST, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_RST, GPIO_PV_HIGH);

    spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_set_clk_rate(SPI_CHANNEL, CONFIG_LCD_CLK_FREQ_MHZ * 1000000);
}

void tft_write_command(uint8_t cmd)
{
    set_dcx_control();
    spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(SPI_CHANNEL, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal_dma(DMAC_CHANNEL0, SPI_CHANNEL, SPI_SLAVE_SELECT, (uint8_t *)(&cmd), 1, SPI_TRANS_CHAR);
}

void tft_write_byte(uint8_t *data_buf, uint32_t length)
{
    set_dcx_data();
    spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(SPI_CHANNEL, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal_dma(DMAC_CHANNEL0, SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_CHAR);
}

void tft_write_half(uint16_t *data_buf, uint32_t length)
{
    set_dcx_data();
    spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 16, 0);
    spi_init_non_standard(SPI_CHANNEL, 16 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal_dma(DMAC_CHANNEL0, SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_SHORT);
}

void tft_write_word(uint32_t *data_buf, uint32_t length, uint32_t flag)
{
    set_dcx_data();
    spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);

    spi_init_non_standard(SPI_CHANNEL, 0 /*instrction length*/, 32 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal_dma(DMAC_CHANNEL0, SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf, length, SPI_TRANS_INT);
}

void tft_fill_data(uint32_t *data_buf, uint32_t length)
{
    set_dcx_data();
    spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    spi_init_non_standard(SPI_CHANNEL, 0 /*instrction length*/, 32 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_fill_data_dma(DMAC_CHANNEL0, SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf, length);
}
