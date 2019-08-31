#include "lcd_sipeed.h"

#include <string.h>

#include "gpiohs.h"
#include "sleep.h"
#include "board.h"
#include "timer.h"


extern void gpiohs_irq_disable(size_t pin);
extern void spi_send_data_normal(spi_device_num_t spi_num,
                                 spi_chip_select_t chip_select,
                                 const uint8_t *tx_buff, size_t tx_len);

///////////////////////////////////////////////////////////////////////////////
static uint32_t PageCount = 0;
static uint8_t *img_ptr = NULL; //TODO: check prt type???

volatile uint8_t dis_flag = 0;

static void lcd_sipeed_send_dat(uint8_t *DataBuf, uint32_t Length);

static void irq_RS_Sync(void)
{
    PageCount++;
    if(PageCount < LCD_H * 2)
    {
        dis_flag = 1;
        lcd_sipeed_send_dat(&img_ptr[PageCount / 2 * LCD_W * 2], LCD_W * 2);
    } else if(PageCount > LCD_SIPEED_FRAME_END_LINE && dis_flag)
    {
        dis_flag = 0;
    }
}

static void lcd_sipeed_send_cmd(uint8_t CMDData)
{
    gpiohs_set_pin(LCD_DCX_HS_NUM, GPIO_PV_HIGH);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(LCD_SIPEED_SPI_DEV, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal(LCD_SIPEED_SPI_DEV, LCD_SIPEED_SPI_SS, (uint8_t *)(&CMDData), 1);
}

static void lcd_sipeed_send_dat(uint8_t *DataBuf, uint32_t Length)
{
    sipeed_spi_send_data_dma(LCD_SIPEED_SPI_DEV, LCD_SIPEED_SPI_SS, DMAC_CHANNEL2, DataBuf - 0x40000000, Length);
}

static volatile uint64_t int_tim = 0;

static int timer_callback(void *ctx)
{
    int_tim = sysctl_get_time_us();
    dis_flag = 1;
    lcd_sipeed_display(lcd_image, 0);
}

void lcd_init(void)
{
    gpiohs_set_drive_mode(LCD_DCX_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(LCD_DCX_HS_NUM, GPIO_PV_HIGH);

    gpiohs_set_drive_mode(LCD_RST_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(LCD_RST_HS_NUM, GPIO_PV_LOW);
    msleep(500);
    gpiohs_set_pin(LCD_RST_HS_NUM, GPIO_PV_HIGH);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    spi_set_clk_rate(LCD_SIPEED_SPI_DEV, LCD_SIPEED_SPI_FREQ);
    //480x272:40/50M测试可用,搭配100M，9M时钟
    //800x480:50/60/70M 配166~125/25M 或 40M 配111/100/83 :16.66
    //似乎FIFO最高速率到不了200M，只能使用6分频166M
    lcd_sipeed_send_cmd(LCD_BL_ON);
    lcd_sipeed_send_cmd(LCD_FRAME_START);
    lcd_sipeed_send_cmd(LCD_DISOLAY_ON);

    timer_init(1);
    timer_set_interval(1, 1, 60 * 1000 * 1000);           //60ms
    timer_irq_register(1, 1, 0, 1, timer_callback, NULL); //4th pri
    timer_set_enable(1, 1, 1);
}

uint8_t lcd_covert_cam_order(uint32_t *addr, uint32_t length)
{
    if(NULL == addr)
        return -1;

    uint32_t *pend = addr + length;
    uint16_t *pdata;
    uint16_t tmp;
    for(; addr < pend; addr++)
    {
        pdata = (uint16_t *)(addr);
        tmp = pdata[0];
        pdata[0] = pdata[1];
        pdata[1] = tmp;
    } //1.7ms
    return 0;
}

void copy_image_cma_to_lcd(uint8_t *cam_img, uint8_t *lcd_img)
{
    uint32_t x = 0, y = 0;
    for(y = 0; y < LCD_H; y++)
    {
        memcpy(lcd_img + y * LCD_W * 2 - 0x40000000, cam_img + (y * IMG_W + x) * 2, IMG_W * 2);
    }
    return;
}

/* flush img_buf to lcd */
void lcd_sipeed_display(uint8_t *img_buf, uint8_t block)
{
    img_ptr = img_buf;
    gpiohs_irq_disable(LCD_DCX_HS_NUM);
    PageCount = 0;

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    lcd_sipeed_send_cmd(LCD_FRAME_START);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 1);
    spi_init_non_standard(LCD_SIPEED_SPI_DEV, 0, 32, 0, SPI_AITM_AS_FRAME_FORMAT);

    gpiohs_set_drive_mode(LCD_DCX_HS_NUM, GPIO_DM_INPUT);
    gpiohs_set_pin_edge(LCD_DCX_HS_NUM, GPIO_PE_RISING);
    gpiohs_set_irq(LCD_DCX_HS_NUM, 3, irq_RS_Sync);

    if(block)
    {
        while(PageCount < LCD_SIPEED_FRAME_END_LINE)
        {
            msleep(1);
        }; //等待的同时可以做其实事情	//50ms
    }
}
