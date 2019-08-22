/* Copyright 2018 Sipeed Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include "ssd1963.h"
#include "gpiohs.h"
#include "spi.h"
#include "sleep.h"
#include "utils.h"

#if CONFIG_LCD_TYPE_SSD1963
/* clang-format off */
#define LCD_SPI_WORK_MODE       SPI_WORK_MODE_2
#define SPI_CHANNEL             0
#define SPI_SLAVE_SELECT        3
/* clang-format on */

static inline void spi_set_tmod(uint8_t spi_num, uint32_t tmod);
static inline void spi_send_data_cpu_align_1(spi_device_num_t spi_num, spi_chip_select_t chip_select,
                                             const uint8_t *tx_buff, size_t tx_len);
static inline void spi_send_data_cpu_align_4(spi_device_num_t spi_num, spi_chip_select_t chip_select,
                                             const uint8_t *tx_buff, size_t tx_len);

static void set_dcx(uint8_t val)
{
    gpiohs_set_pin(LCD_DCX_HS_NUM, val);
}

static void set_rst(uint8_t val)
{
    gpiohs_set_pin(LCD_RST_HS_NUM, val);
}

void tft_hard_init(void)
{
    /* set gpio mode */
    gpiohs_set_drive_mode(LCD_DCX_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(LCD_DCX_HS_NUM, GPIO_PV_HIGH);
    gpiohs_set_drive_mode(LCD_RST_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(LCD_RST_HS_NUM, GPIO_PV_LOW);

    /* init spi0 */
    set_rst(0);
    spi_init(SPI_CHANNEL, LCD_SPI_WORK_MODE, SPI_FF_OCTAL, 8, 0);
    spi_set_clk_rate(SPI_CHANNEL, 1000000 * 80);
    msleep(50);
    set_rst(1);
    msleep(50);
}

void tft_ssd1963_write_cmd(uint8_t cmd)
{
    set_dcx(0);
    spi_init(SPI_CHANNEL, LCD_SPI_WORK_MODE, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(SPI_CHANNEL, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_cpu_align_1(SPI_CHANNEL, SPI_SLAVE_SELECT, (uint8_t *)(&cmd), 1);
}

void tft_ssd1963_write_data(uint8_t *data_buf, uint32_t length)
{
    set_dcx(1);
    // spi_init(SPI_CHANNEL, LCD_SPI_WORK_MODE, SPI_FF_OCTAL, 8, 0);
    // spi_init_non_standard(SPI_CHANNEL, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
    //                       SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    if (length % 4)
    {
        if (length / 4)
        {
            uint32_t len = length - (length % 4);
            if (len)
                spi_send_data_cpu_align_4(SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf, len);
            spi_send_data_cpu_align_1(SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf + len, length % 4);
        }
        else
        {
            spi_send_data_cpu_align_1(SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf, length);
        }
    }
    else if (length != 0)
    {
        spi_send_data_cpu_align_4(SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf, length);
    }
}

///////////////////////////////////////////////////////////////////////////////
static inline void spi_set_tmod(uint8_t spi_num, uint32_t tmod)
{
    configASSERT(spi_num < SPI_DEVICE_MAX);
    volatile spi_t *spi_handle = spi[spi_num];
    uint8_t tmod_offset = 0;
    switch (spi_num)
    {
    case 0:
    case 1:
    case 2:
        tmod_offset = 8;
        break;
    case 3:
    default:
        tmod_offset = 10;
        break;
    }
    set_bit(&spi_handle->ctrlr0, 3 << tmod_offset, tmod << tmod_offset);
}

static inline void spi_send_data_cpu_align_1(spi_device_num_t spi_num, spi_chip_select_t chip_select,
                                             const uint8_t *tx_buff, size_t tx_len)
{
    spi_set_tmod(spi_num, SPI_TMOD_TRANS);

    volatile spi_t *spi_handle = spi[spi_num];

    spi_handle->ssienr = 0x01;
    spi_handle->ser = 1U << chip_select;
    uint32_t i = 0;
    while (tx_len)
    {
        spi_handle->dr[0] = tx_buff[i++];
        tx_len--;
        while (spi_handle->sr & 0x01)
            ;
    }
    while ((spi_handle->sr & 0x05) != 0x04)
        ;
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}

static inline void spi_send_data_cpu_align_4(spi_device_num_t spi_num, spi_chip_select_t chip_select,
                                             const uint8_t *tx_buff, size_t tx_len)
{
    spi_set_tmod(spi_num, SPI_TMOD_TRANS);

    volatile spi_t *spi_handle = spi[spi_num];

    spi_handle->ssienr = 0x01;
    spi_handle->ser = 1U << chip_select;
    uint32_t i = 0;
    while (tx_len)
    {
#if 1
        /* 如果不正确再尝试其他的方式 */
        spi_handle->dr[0] = tx_buff[i++];
        spi_handle->dr[0] = tx_buff[i++];
        spi_handle->dr[0] = tx_buff[i++];
        spi_handle->dr[0] = tx_buff[i++];
        tx_len -= 4;
#elif 1
        spi_handle->dr[0] = tx_buff[i++];
        tx_len--;
        while ((spi_handle->sr & 0x01))
            ;
#else
        /* 这里的nop根据自己的需要进行调整 */
        spi_handle->dr[0] = tx_buff[i++];
        tx_len--;
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");

        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");
        asm volatile("nop");

        // asm volatile("nop");
        // asm volatile("nop");
        // asm volatile("nop");
        // asm volatile("nop");
        // asm volatile("nop");

        // asm volatile("nop");
        // asm volatile("nop");
        // asm volatile("nop");
        // asm volatile("nop");
        // asm volatile("nop");
#endif
    }
    while ((spi_handle->sr & 0x05) != 0x04)
        ;
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}
#endif