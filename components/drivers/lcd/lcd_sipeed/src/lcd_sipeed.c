#include "lcd_sipeed.h"

#include <string.h>
#include <stdlib.h>

#include "gpiohs.h"
#include "sleep.h"
#include "timer.h"
#include "spi.h"

#include "mem_macro.h"

///////////////////////////////////////////////////////////////////////////////
extern void gpiohs_irq_disable(size_t pin);
extern void spi_send_data_normal(spi_device_num_t spi_num,
                                 spi_chip_select_t chip_select,
                                 const uint8_t *tx_buff, size_t tx_len);

extern void sipeed_spi_send_data_dma(uint8_t spi_num, uint8_t chip_select,
                                     uint8_t dma_chn, const uint8_t *data_buf, size_t buf_len);

///////////////////////////////////////////////////////////////////////////////
volatile uint8_t dis_flag = 0;

static int32_t PageCount = 0;
static uint8_t *disp_buf = NULL;
static uint8_t *disp_banner_buf = NULL;

///////////////////////////////////////////////////////////////////////////////
#if 0
static void lcd_test(void)
{
    uint8_t *lcd_addr = _IOMEM_PADDR(disp_buf);
    uint16_t unit_x = SIPEED_LCD_W / 16;
    for (int i = 0; i < SIPEED_LCD_H; i++)
    {
        if (1)
        {
            for (int j = 0; j < SIPEED_LCD_W; j++)
            {
                if (j < unit_x * 5)
                { //R
                    lcd_addr[(i * SIPEED_LCD_W + j) * 2 + 1] = (1 << (j / unit_x)) << 3;
                    lcd_addr[(i * SIPEED_LCD_W + j) * 2 + 0] = 0;
                }
                else if (j < unit_x * (5 + 6))
                { //G
                    lcd_addr[(i * SIPEED_LCD_W + j) * 2 + 1] = ((1 << ((j - unit_x * 5) / unit_x)) >> 3);
                    lcd_addr[(i * SIPEED_LCD_W + j) * 2 + 0] = ((1 << ((j - unit_x * 5) / unit_x)) << 5);
                }
                else
                { //B
                    lcd_addr[(i * SIPEED_LCD_W + j) * 2 + 1] = 0;
                    lcd_addr[(i * SIPEED_LCD_W + j) * 2 + 0] = 1 << ((j - unit_x * (5 + 6)) / unit_x);
                }
            }
        }
    }
    while (1)
    {
    };
    return;
}
#endif

static void lcd_sipeed_send_cmd(uint8_t CMDData)
{
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_DCX, GPIO_PV_HIGH);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(LCD_SIPEED_SPI_DEV, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal(LCD_SIPEED_SPI_DEV, LCD_SIPEED_SPI_SS, (uint8_t *)(&CMDData), 1);
}

static void lcd_sipeed_send_dat(uint8_t *DataBuf, uint32_t Length)
{
    sipeed_spi_send_data_dma(LCD_SIPEED_SPI_DEV, LCD_SIPEED_SPI_SS, DMAC_CHANNEL2, _IOMEM_PADDR(DataBuf), Length);
}

static void irq_RS_Sync(void)
{
    if (PageCount < SIPEED_LCD_H * LCD_YUP)
    {
        dis_flag = 1;
        if (PageCount >= 0)
        {
            lcd_sipeed_send_dat(&disp_buf[PageCount / LCD_YUP * SIPEED_LCD_W * 2], SIPEED_LCD_W * 2);
            dmac_wait_done(DMAC_CHANNEL2);
            for (int i = 0; i < 30; i++)
            {
                asm volatile("nop");
            };
            if (SIPEED_LCD_BANNER_W)
            {
                lcd_sipeed_send_dat(&disp_banner_buf[PageCount * SIPEED_LCD_BANNER_W * 2], SIPEED_LCD_BANNER_W * 2);
            }
        }
    }
    else if (PageCount > LCD_SIPEED_FRAME_END_LINE - 1 && dis_flag)
    {
        dis_flag = 0;
    }
    PageCount++;
    return;
}

//excute in 50Hz lcd refresh irq
static void lcd_sipeed_display(uint8_t block)
{
    gpiohs_irq_disable(CONFIG_LCD_GPIOHS_DCX);
    PageCount = -1;

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    lcd_sipeed_send_cmd(LCD_FRAME_START);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 1);
    spi_init_non_standard(LCD_SIPEED_SPI_DEV, 0, 32, 0, SPI_AITM_AS_FRAME_FORMAT);

    gpiohs_set_drive_mode(CONFIG_LCD_GPIOHS_DCX, GPIO_DM_INPUT);
    gpiohs_set_pin_edge(CONFIG_LCD_GPIOHS_DCX, GPIO_PE_RISING);
    gpiohs_set_irq(CONFIG_LCD_GPIOHS_DCX, 3, irq_RS_Sync);

    if (block)
    {
        while (PageCount < LCD_SIPEED_FRAME_END_LINE)
        {
            msleep(1);
        }; //等待的同时可以做其实事情	//50ms
    }
}

static int timer_callback(void *ctx)
{
    dis_flag = 1;
    lcd_sipeed_display(0);
    return 0;
}

static void lcd_sipeed_config(lcd_t *lcd)
{
    gpiohs_set_drive_mode(CONFIG_LCD_GPIOHS_DCX, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_DCX, GPIO_PV_HIGH);

    gpiohs_set_drive_mode(CONFIG_LCD_GPIOHS_RST, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_RST, GPIO_PV_LOW);
    msleep(100);
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_RST, GPIO_PV_HIGH);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    spi_set_clk_rate(LCD_SIPEED_SPI_DEV, LCD_SIPEED_SPI_FREQ);
    //480x272:40/50M测试可用,搭配100M，9M时钟
    //800x480:50/60/70M 配166~125/25M 或 40M 配111/100/83 :16.66
    //似乎FIFO最高速率到不了200M，只能使用6分频166M
    lcd_sipeed_send_cmd(LCD_BL_ON);
    lcd_sipeed_send_cmd(LCD_FRAME_START);
    lcd_sipeed_send_cmd(LCD_DISOLAY_ON);

    timer_init(1);

    uint8_t tmp = 40;

#if CONFIG_TYPE_SIPEED_7_INCH
    tmp = 25;
#endif

    timer_set_interval(1, 1, tmp * 1000 * 1000);
    //SPI传输时间(dis_flag=1)约20ms，需要留出10~20ms给缓冲区准备(dis_flag=0)
    //如果图像中断处理时间久，可以调慢FPGA端的像素时钟，但是注意不能比刷屏中断慢(否则就会垂直滚动画面)。
    //33M->18ms  25M->23ms  20M->29ms
    timer_irq_register(1, 1, 0, 1, timer_callback, NULL); //4th pri
    timer_set_enable(1, 1, 1);

    return;
}

static int lcd_sipeed_clear(lcd_t *lcd, uint16_t rgb565_color)
{
    uint16_t *addr = _IOMEM_PADDR(disp_buf);
    //clear iomem
    for (int i = 0; i < SIPEED_LCD_H; i++)
        for (int j = 0; j < SIPEED_LCD_W; j++)
            addr[i * SIPEED_LCD_W + j] = rgb565_color;
    //clear cache
    addr = _CACHE_PADDR(disp_buf);
    for (int i = 0; i < SIPEED_LCD_H; i++)
        for (int j = 0; j < SIPEED_LCD_W; j++)
            addr[i * SIPEED_LCD_W + j] = rgb565_color;
    return;
}

int lcd_sipeed_init(lcd_t *lcd)
{
    lcd->dir = 0x0;
    lcd->width = SIPEED_LCD_W;
    lcd->height = SIPEED_LCD_H;
    lcd->lcd_config = lcd_sipeed_config; //初始化配置
    lcd->lcd_clear = lcd_sipeed_clear;

    lcd->lcd_draw_picture = NULL;
    lcd->lcd_set_direction = NULL;
    lcd->lcd_set_area = NULL;

    return 0;
}

uint8_t lcd_sipeed_config_disp_buf(uint8_t *lcd_disp_buf, uint8_t *lcd_disp_banner_buf)
{
    if (lcd_disp_buf == NULL)
    {
        return 1;
    }

    disp_buf = _IOMEM_PADDR(lcd_disp_buf);
    disp_banner_buf = lcd_disp_banner_buf;
    return 0;
}
