#include "lcd_sipeed.h"

#include <string.h>
#include <stdlib.h>

#include "fpioa.h"
#include "gpiohs.h"
#include "sleep.h"
#include "timer.h"
#include "spi.h"
#include "printf.h"

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

static void lcd_init_over_spi(void);

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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if CONFIG_TYPE_800_480_57_INCH
static void lcd_int_800480(void)
{
    //H
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HBP);
    lcd_sipeed_send_cmd(210);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_W);
    lcd_sipeed_send_cmd(800 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HFP);
    lcd_sipeed_send_cmd(44);

    //V
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VBP);
    lcd_sipeed_send_cmd((LCD_VBP));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_H);
    lcd_sipeed_send_cmd(480 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VFP);
    lcd_sipeed_send_cmd(22);

    //b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_DIVCFG);
    lcd_sipeed_send_cmd((LCD_SCALE_ENABLE << 7) | (LCD_SCALE_X << 6) | (LCD_SCALE_2X2 << 3) | (LCD_SCALE_NONE << 0));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_POS);
    lcd_sipeed_send_cmd(640 / 4);

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_PHASE);
    // lcd_sipeed_send_cmd((1 << LCD_PHASE_DE) | (1 << LCD_PHASE_VSYNC) | (1 << LCD_PHASE_HSYNC) | (1 << LCD_PHASE_PCLK));
    lcd_sipeed_send_cmd((1 << LCD_PHASE_DE) | (0 << LCD_PHASE_VSYNC) | (0 << LCD_PHASE_HSYNC) | (1 << LCD_PHASE_PCLK));

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_INITDONE);
    lcd_sipeed_send_cmd(0x01);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_START);
    lcd_sipeed_send_cmd(0x0d);
}

static void lcd_800480_irq_rs_sync(void)
{
    if (PageCount >= 0)
    {
        if (PageCount < 480)
        {
            dis_flag = 1;
            lcd_sipeed_send_dat(&disp_buf[(PageCount / 2) * SIPEED_LCD_W * 2], SIPEED_LCD_W * 2 + LCD_LINE_FIX_PIXEL * 2);
            dmac_wait_done(DMAC_CHANNEL2);
            for (int i = 0; i < 30; i++)
            {
                asm volatile("nop");
            };
            if (SIPEED_LCD_BANNER_W)
            {
                lcd_sipeed_send_dat(&disp_banner_buf[PageCount * SIPEED_LCD_BANNER_W * 2], SIPEED_LCD_BANNER_W * 2 + LCD_LINE_FIX_PIXEL * 2);
            }
        }
        else if ((PageCount >= (480 + LCD_VFP - 1)))
        {
            dis_flag = 0;
        }
    }

    PageCount++;
    return;
}
#endif

#if CONFIG_TYPE_480_800_4_INCH
static void lcd_init_480800(void)
{
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VBP);
    lcd_sipeed_send_cmd((LCD_VBP));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_H);
    lcd_sipeed_send_cmd(800 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VFP);
    lcd_sipeed_send_cmd(12);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HBP);
    lcd_sipeed_send_cmd(252);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_W);
    lcd_sipeed_send_cmd(480 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HFP);
    lcd_sipeed_send_cmd(40);

    //b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_DIVCFG);
    lcd_sipeed_send_cmd((LCD_SCALE_ENABLE << 7) | (LCD_SCALE_Y << 6) | (LCD_SCALE_2X2 << 3) | (LCD_SCALE_NONE << 0));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_POS);
    lcd_sipeed_send_cmd(640 / 4);

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_PHASE);
    lcd_sipeed_send_cmd((1 << LCD_PHASE_DE) | (1 << LCD_PHASE_VSYNC) | (1 << LCD_PHASE_HSYNC) | (1 << LCD_PHASE_PCLK));

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_INITDONE);
    lcd_sipeed_send_cmd(0x01);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_START);
    lcd_sipeed_send_cmd(0x0d);
}

static void lcd_480800_irq_rs_sync(void)
{
    if (PageCount >= 0)
    {
        if (PageCount < 640)
        {
            dis_flag = 1;
            lcd_sipeed_send_dat(&disp_buf[(PageCount / 2) * SIPEED_LCD_W * 2], SIPEED_LCD_W * 2 + LCD_LINE_FIX_PIXEL * 2);
        }
        else if (PageCount < 800)
        {
            dis_flag = 1;
            if (SIPEED_LCD_BANNER_W)
            {
                lcd_sipeed_send_dat(&disp_banner_buf[(PageCount - 640) * SIPEED_LCD_BANNER_W * 2], SIPEED_LCD_BANNER_W * 2 + LCD_LINE_FIX_PIXEL * 2);
            }
        }
        else if ((PageCount >= (800 + LCD_VFP - 1)))
        {
            dis_flag = 0;
        }
    }

    PageCount++;
    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void lcd_init_480800_hmirror(void)
{
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VBP);
    lcd_sipeed_send_cmd((LCD_VBP));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_H);
    lcd_sipeed_send_cmd(800 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VFP);
    lcd_sipeed_send_cmd(12);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HBP);
    lcd_sipeed_send_cmd(252);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_W);
    lcd_sipeed_send_cmd(480 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HFP);
    lcd_sipeed_send_cmd(40);

    //b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_DIVCFG);
    lcd_sipeed_send_cmd((LCD_SCALE_ENABLE << 7) | (LCD_SCALE_Y << 6) | (LCD_SCALE_NONE << 3) | (LCD_SCALE_2X2 << 0));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_POS);
    lcd_sipeed_send_cmd(160 / 4);

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_PHASE);
    lcd_sipeed_send_cmd((1 << LCD_PHASE_DE) | (1 << LCD_PHASE_VSYNC) | (1 << LCD_PHASE_HSYNC) | (1 << LCD_PHASE_PCLK));

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_INITDONE);
    lcd_sipeed_send_cmd(0x01);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_START);
    lcd_sipeed_send_cmd(0x0d);
}

static void lcd_480800_irq_rs_sync_hmirror(void)
{
    if (PageCount >= 0)
    {
        if (PageCount <= 160)
        {
            dis_flag = 1;
            if (SIPEED_LCD_BANNER_W)
            {
                if (PageCount == 160)
                {
                    goto _exit;
                }
                lcd_sipeed_send_dat(&disp_banner_buf[(159 - PageCount) * SIPEED_LCD_BANNER_W * 2], SIPEED_LCD_BANNER_W * 2 + LCD_LINE_FIX_PIXEL * 2);
            }
        }
        else if (PageCount < 800)
        {
            dis_flag = 1;
            lcd_sipeed_send_dat(&disp_buf[(319 - ((PageCount - 161) / 2)) * SIPEED_LCD_W * 2], SIPEED_LCD_W * 2 + LCD_LINE_FIX_PIXEL * 2);
        }
        else if ((PageCount >= (800 + LCD_VFP - 1)))
        {
            dis_flag = 0;
        }
    }
_exit:
    PageCount++;
    return;
}
#endif /* CONFIG_TYPE_480_800_4_INCH */
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if CONFIG_TYPE_480_854_5_INCH
static void lcd_init_480854(void)
{
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VBP);
    lcd_sipeed_send_cmd((LCD_VBP));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_H);
    lcd_sipeed_send_cmd(856 / 4); //多了2行
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VFP);
    lcd_sipeed_send_cmd(12);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HBP);
    lcd_sipeed_send_cmd(252);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_W);
    lcd_sipeed_send_cmd(480 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HFP);
    lcd_sipeed_send_cmd(40);

    //b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_DIVCFG);
    lcd_sipeed_send_cmd((LCD_SCALE_ENABLE << 7) | (LCD_SCALE_Y << 6) | (LCD_SCALE_2X2 << 3) | (LCD_SCALE_NONE << 0));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_POS);
    lcd_sipeed_send_cmd(640 / 4);

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_PHASE);
    lcd_sipeed_send_cmd((1 << LCD_PHASE_DE) | (1 << LCD_PHASE_VSYNC) | (1 << LCD_PHASE_HSYNC) | (1 << LCD_PHASE_PCLK));

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_INITDONE);
    lcd_sipeed_send_cmd(0x01);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_START);
    lcd_sipeed_send_cmd(0x0d);
}

static void lcd_480854_irq_rs_sync(void)
{
    if (PageCount >= 0)
    {
        if (PageCount < 640)
        {
            dis_flag = 1;
            lcd_sipeed_send_dat(&disp_buf[(PageCount / 2) * SIPEED_LCD_W * 2], SIPEED_LCD_W * 2 /*+ LCD_LINE_FIX_PIXEL * 2*/);
        }
        else if (PageCount < 854)
        {
            dis_flag = 1;
            if (SIPEED_LCD_BANNER_W)
            {
                // lcd_sipeed_send_dat(&disp_banner_buf[(PageCount - 640) * SIPEED_LCD_BANNER_W * 2], SIPEED_LCD_BANNER_W * 2 + LCD_LINE_FIX_PIXEL * 2);

                // //FIXME: 查找为什么要交错显示.
                uint16_t tt = (PageCount - 640);
                tt = (tt % 2) ? (tt - 1) : (tt + 1);
                lcd_sipeed_send_dat(&disp_banner_buf[tt * SIPEED_LCD_BANNER_W * 2], SIPEED_LCD_BANNER_W * 2 /*+ LCD_LINE_FIX_PIXEL * 2*/);
            }
        }
        else if ((PageCount >= (856 + LCD_VFP - 1))) /* 初始化配置的是856 */
        {
            dis_flag = 0;
        }
    }
    PageCount++;
    return;
}
#endif /* CONFIG_TYPE_480_854_5_INCH */
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if CONFIG_TYPE_360_640_4_4_INCH
static uint8_t cal_div_reg(uint8_t enable, uint8_t div_xy, uint8_t mula_M, uint8_t mula_N, uint8_t mulb_M, uint8_t mulb_N)
{
    if (mula_M > 2 || mulb_M > 2)
    {
        printk("mul_M should = 1 or 2\r\n");
    }
    mula_N -= (mula_M - 1);
    mulb_N -= (mulb_M - 1);
    return (enable << 7) | (div_xy << 6) | ((mula_M - 1) << 5) | ((mula_N - 1) << 3) | ((mulb_M - 1) << 2) | ((mulb_N - 1) << 0);
}

static void lcd_init_360640(void)
{
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VBP);
    lcd_sipeed_send_cmd((LCD_VBP + 2));
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_H);
    lcd_sipeed_send_cmd(640 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_VFP);
    lcd_sipeed_send_cmd(LCD_VFP);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HBP);
    lcd_sipeed_send_cmd(200);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_W);
    lcd_sipeed_send_cmd(360 / 4);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_HFP);
    lcd_sipeed_send_cmd(40);

    //b7:0 dis,1 en; b6: 0x,1y; b[5:3]: mul part1; b[2:0]: mul part2
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_DIVCFG);
    // lcd_sipeed_send_cmd((LCD_SCALE_ENABLE << 7) | (LCD_SCALE_Y << 6) | (LCD_SCALE_NONE << 3) | (LCD_SCALE_NONE << 0));
    lcd_sipeed_send_cmd(cal_div_reg(1, 1, 2, 3, 1, 1)); //y方向分割，第一部分1.5倍缩放

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_POS);
    lcd_sipeed_send_cmd(480 / 4);

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_PHASE);
    lcd_sipeed_send_cmd((1 << LCD_PHASE_DE) | (1 << LCD_PHASE_VSYNC) | (1 << LCD_PHASE_HSYNC) | (0 << LCD_PHASE_PCLK));

    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_INITDONE);
    lcd_sipeed_send_cmd(0x01);
}

static void lcd_360640_irq_rs_sync(void)
{
    if (PageCount >= 0)
    {
        if (PageCount < 480)
        {
            dis_flag = 1;
            lcd_sipeed_send_dat(&disp_buf[(PageCount * 2 / 3) * SIPEED_LCD_W * 2], SIPEED_LCD_W * 2);
        }
        else if (PageCount < 640)
        {
            dis_flag = 1;

            if (SIPEED_LCD_BANNER_W)
            {
                lcd_sipeed_send_dat(&disp_banner_buf[(PageCount - 480) * SIPEED_LCD_BANNER_W * 2], SIPEED_LCD_BANNER_W * 2);
            }
        }
        else if ((PageCount >= (640 + LCD_VFP - 1))) /* 初始化配置的是856 */
        {
            dis_flag = 0;
        }
    }
    PageCount++;
    return;
}
#endif /* CONFIG_TYPE_360_640_4_4_INCH */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//excute in 50Hz lcd refresh irq
static void lcd_sipeed_display(void)
{
    gpiohs_irq_disable(CONFIG_LCD_GPIOHS_DCX);
    PageCount = -(LCD_VBP - LCD_INIT_LINE);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    lcd_sipeed_send_cmd(LCD_WRITE_REG | LCD_ADDR_START);
    lcd_sipeed_send_cmd(0x0d);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 1);
    spi_init_non_standard(LCD_SIPEED_SPI_DEV, 0, 32, 0, SPI_AITM_AS_FRAME_FORMAT);

    gpiohs_set_drive_mode(CONFIG_LCD_GPIOHS_DCX, GPIO_DM_INPUT);
    gpiohs_set_pin_edge(CONFIG_LCD_GPIOHS_DCX, GPIO_PE_RISING);

    /*
#if CONFIG_TYPE_480_800_4_INCH
    gpiohs_set_irq(CONFIG_LCD_GPIOHS_DCX, 3, lcd_480800_irq_rs_sync);
// gpiohs_set_irq(CONFIG_LCD_GPIOHS_DCX, 3, lcd_480800_irq_rs_sync_hmirror);
#elif CONFIG_TYPE_480_854_5_INCH
    gpiohs_set_irq(CONFIG_LCD_GPIOHS_DCX, 3, lcd_480854_irq_rs_sync);
#elif CONFIG_TYPE_360_640_4_4_INCH
    gpiohs_set_irq(CONFIG_LCD_GPIOHS_DCX, 3, lcd_360640_irq_rs_sync);
#endif
*/

#if CONFIG_TYPE_800_480_57_INCH
    gpiohs_set_irq(CONFIG_LCD_GPIOHS_DCX, 3, lcd_800480_irq_rs_sync);
#else
    printk("Unknown LCD Type\r\n");
    while (1)
        ;
#endif
}

static int timer_callback(void *ctx)
{
    dis_flag = 1;
    lcd_sipeed_display();
    return 0;
}

static void lcd_sipeed_config(lcd_t *lcd)
{
    uint8_t tmp = 40;

    /* LCD */
    // fpioa_set_function(CONFIG_PIN_NUM_LCD_RST, FUNC_GPIOHS0 + CONFIG_LCD_GPIOHS_RST);
    // fpioa_set_function(CONFIG_PIN_NUM_LCD_DCX, FUNC_GPIOHS0 + CONFIG_LCD_GPIOHS_DCX);
    // fpioa_set_function(CONFIG_PIN_NUM_LCD_WRX, FUNC_SPI0_SS3);
    // fpioa_set_function(CONFIG_PIN_NUM_LCD_SCK, FUNC_SPI0_SCLK);

    // fpioa_set_io_driving(CONFIG_PIN_NUM_LCD_SCK, FPIOA_DRIVING_7);

    gpiohs_set_drive_mode(CONFIG_LCD_GPIOHS_DCX, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_DCX, GPIO_PV_HIGH);

    gpiohs_set_drive_mode(CONFIG_LCD_GPIOHS_RST, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_RST, GPIO_PV_LOW);
    msleep(100);
    gpiohs_set_pin(CONFIG_LCD_GPIOHS_RST, GPIO_PV_HIGH);

    spi_init(LCD_SIPEED_SPI_DEV, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    spi_set_clk_rate(LCD_SIPEED_SPI_DEV, LCD_SIPEED_SPI_FREQ);

    /*
#if CONFIG_LCD_NEED_SPI_INIT
    lcd_init_over_spi();
#endif

#if CONFIG_TYPE_480_800_4_INCH
    tmp = 28;
    lcd_init_480800();
// lcd_init_480800_hmirror();
#elif CONFIG_TYPE_480_854_5_INCH
    tmp = 25;
    lcd_init_480854();
#elif CONFIG_TYPE_360_640_4_4_INCH
    tmp = 28;
    lcd_init_360640();
#endif
*/

#if CONFIG_TYPE_800_480_57_INCH
    lcd_int_800480();
#if CONFIG_TYPE_SIPEED_7_INCH
    tmp = 28;
#endif /* CONFIG_TYPE_SIPEED_7_INCH */
#else
    printk("Unknown LCD Type\r\n");
    while (1)
        ;
#endif

    timer_init(1);
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

// ///////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////
// /* clang-format off */
// #define GPIOHS_OUT_HIGH(io) (*(volatile uint32_t *)0x3800100CU) |= (1 << (io))
// #define GPIOHS_OUT_LOWX(io) (*(volatile uint32_t *)0x3800100CU) &= ~(1 << (io))

// #define GET_GPIOHS_VALX(io) (((*(volatile uint32_t *)0x38001000U) >> (io)) & 1)
// /* clang-format on */

// ///////////////////////////////////////////////////////////////////////////////
// static uint8_t spi_rw_low_spd(uint16_t data)
// {
//     for (uint8_t i = 0; i < 9; i++)
//     {
//         GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_LCD_SPI_SCLK);
//         if (data & 0x100)
//         {
//             GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_LCD_SPI_MOSI);
//         }
//         else
//         {
//             GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_LCD_SPI_MOSI);
//         }
//         data <<= 1;
//         usleep(5);
//         GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_LCD_SPI_SCLK);
//         usleep(5);
//     }
//     return 0;
// }

// static void WriteComm_16(uint16_t cmd)
// {
//     uint16_t tmp = 0;
//     uint8_t dat[2];

//     dat[0] = (uint8_t)((cmd >> 8) & 0xFF);
//     dat[1] = (uint8_t)((cmd)&0xFF);

//     GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_LCD_SPI_CS);
//     tmp = (0 << 8) | dat[0];
//     spi_rw_low_spd(tmp);

//     tmp = (0 << 8) | dat[1];
//     spi_rw_low_spd(tmp);

//     GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_LCD_SPI_CS);
// }

// static void WriteComm(uint8_t cmd)
// {
//     uint16_t tmp = 0;

//     GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_LCD_SPI_CS);
//     tmp = (0 << 8) | cmd;
//     spi_rw_low_spd(tmp);

//     GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_LCD_SPI_CS);
// }

// static void WriteData(uint8_t dat)
// {
//     uint16_t tmp = 0;

//     GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_LCD_SPI_CS);
//     tmp = (1 << 8) | dat;
//     spi_rw_low_spd(tmp);

//     GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_LCD_SPI_CS);
// }

// static void lcd_st7701s_480_800_fake_ips_init(void);
// static void lcd_st7701s_480_854_true_ips_init(void);
// static void lcd_st7701s_360640_true_ips_init(void);

// ///////////////////////////////////////////////////////////////////////////////
// static void lcd_init_over_spi(void)
// {
//     // #if CONFIG_TYPE_480_800_4_INCH
//     //     fpioa_set_function(CONFIG_LCD_SPI_PIN_MOSI, FUNC_GPIOHS0 + CONFIG_LCD_SPI_GPIOHS_CS);
//     //     fpioa_set_function(CONFIG_LCD_SPI_PIN_CS, FUNC_GPIOHS0 + CONFIG_LCD_SPI_GPIOHS_MOSI);
//     // #elif CONFIG_TYPE_480_854_5_INCH
//     fpioa_set_function(CONFIG_PIN_NUM_LCD_SPI_CS, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_LCD_SPI_CS);
//     fpioa_set_function(CONFIG_PIN_NUM_LCD_SPI_MOSI, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_LCD_SPI_MOSI);
//     // #endif

//     fpioa_set_function(CONFIG_PIN_NUM_LCD_SPI_SCLK, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_LCD_SPI_SCLK);
//     fpioa_set_function(CONFIG_PIN_NUM_LCD_SPI_RST, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_LCD_SPI_RST);

//     //cs
//     gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_LCD_SPI_CS, GPIO_DM_OUTPUT);
//     gpiohs_set_pin(CONFIG_GPIOHS_NUM_LCD_SPI_CS, 1);

//     //sclk
//     gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_LCD_SPI_SCLK, GPIO_DM_OUTPUT);
//     gpiohs_set_pin(CONFIG_GPIOHS_NUM_LCD_SPI_SCLK, 1);

//     //mosi
//     gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_LCD_SPI_MOSI, GPIO_DM_OUTPUT);
//     gpiohs_set_pin(CONFIG_GPIOHS_NUM_LCD_SPI_MOSI, 1);

//     //rst
//     gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_LCD_SPI_RST, GPIO_DM_OUTPUT);
//     gpiohs_set_pin(CONFIG_GPIOHS_NUM_LCD_SPI_RST, 1);

//     GPIOHS_OUT_LOWX(CONFIG_GPIOHS_NUM_LCD_SPI_RST);
//     msleep(50);
//     GPIOHS_OUT_HIGH(CONFIG_GPIOHS_NUM_LCD_SPI_RST);

// #if CONFIG_TYPE_480_800_4_INCH
//     lcd_st7701s_480_800_fake_ips_init();
// #elif CONFIG_TYPE_480_854_5_INCH
//     lcd_st7701s_480_854_true_ips_init();
// #elif CONFIG_TYPE_360_640_4_4_INCH
//     lcd_st7701s_360640_true_ips_init();
// #endif
// }

// static void lcd_st7701s_480_800_fake_ips_init(void)
// {
//     printk("lcd is 480x800, st7701s, fake IPS, BL 12V\r\n");

//     WriteComm(0x11);
//     msleep(120); //Delay 120ms
//     //---------------------------------------Bank0 Setting-------------------------------------------------//
//     //------------------------------------Display Control setting----------------------------------------------//
//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteComm(0xC0);
//     WriteData(0x63);
//     WriteData(0x00);
//     WriteComm(0xC1);
//     WriteData(0x0C);
//     WriteData(0x02);
//     WriteComm(0xC2);
//     WriteData(0x31);
//     WriteData(0x08);

//     WriteComm(0xC7);
//     WriteData(0x10);

//     WriteComm(0xCC);
//     WriteData(0x10);
//     //-------------------------------------Gamma Cluster Setting-------------------------------------------//
//     WriteComm(0xB0);
//     WriteData(0x40);
//     WriteData(0x02);
//     WriteData(0x87);
//     WriteData(0x0E);
//     WriteData(0x15);
//     WriteData(0x0A);
//     WriteData(0x03);
//     WriteData(0x0A);
//     WriteData(0x0A);
//     WriteData(0x18);
//     WriteData(0x08);
//     WriteData(0x16);
//     WriteData(0x13);
//     WriteData(0x07);
//     WriteData(0x09);
//     WriteData(0x19);
//     WriteComm(0xB1);
//     WriteData(0x40);
//     WriteData(0x01);
//     WriteData(0x86);
//     WriteData(0x0D);
//     WriteData(0x13);
//     WriteData(0x09);
//     WriteData(0x03);
//     WriteData(0x0A);
//     WriteData(0x09);
//     WriteData(0x1C);
//     WriteData(0x09);
//     WriteData(0x15);
//     WriteData(0x13);
//     WriteData(0x91);
//     WriteData(0x16);
//     WriteData(0x19);
//     //---------------------------------------End Gamma Setting----------------------------------------------//
//     //------------------------------------End Display Control setting----------------------------------------//
//     //-----------------------------------------Bank0 Setting End---------------------------------------------//
//     //-------------------------------------------Bank1 Setting---------------------------------------------------//
//     //-------------------------------- Power Control Registers Initial --------------------------------------//
//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x11);
//     WriteComm(0xB0);
//     WriteData(0x4D);
//     //-------------------------------------------Vcom Setting---------------------------------------------------//
//     WriteComm(0xB1);
//     WriteData(0x64);
//     //-----------------------------------------End Vcom Setting-----------------------------------------------//
//     WriteComm(0xB2);
//     WriteData(0x07);
//     WriteComm(0xB3);
//     WriteData(0x80);
//     WriteComm(0xB5);
//     WriteData(0x47);
//     WriteComm(0xB7);
//     WriteData(0x85);
//     WriteComm(0xB8);
//     WriteData(0x21);
//     WriteComm(0xB9);
//     WriteData(0x10);
//     WriteComm(0xC1);
//     WriteData(0x78);
//     WriteComm(0xC2);
//     WriteData(0x78);
//     WriteComm(0xD0);
//     WriteData(0x88);
//     //---------------------------------End Power Control Registers Initial -------------------------------//
//     msleep(100);
//     //---------------------------------------------GIP Setting----------------------------------------------------//
//     WriteComm(0xE0);
//     WriteData(0x00);
//     WriteData(0xB4);
//     WriteData(0x02);
//     WriteComm(0xE1);
//     WriteData(0x06);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x05);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x20);
//     WriteData(0x20);
//     WriteComm(0xE2);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x03);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0xE3);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x33);
//     WriteData(0x33);
//     WriteComm(0xE4);
//     WriteData(0x44);
//     WriteData(0x44);
//     WriteComm(0xE5);
//     WriteData(0x09);
//     WriteData(0x31);
//     WriteData(0xBE);
//     WriteData(0xA0);
//     WriteData(0x0B);
//     WriteData(0x31);
//     WriteData(0xBE);
//     WriteData(0xA0);
//     WriteData(0x05);
//     WriteData(0x31);
//     WriteData(0xBE);
//     WriteData(0xA0);
//     WriteData(0x07);
//     WriteData(0x31);
//     WriteData(0xBE);
//     WriteData(0xA0);
//     WriteComm(0xE6);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x33);
//     WriteData(0x33);
//     WriteComm(0xE7);
//     WriteData(0x44);
//     WriteData(0x44);
//     WriteComm(0xE8);
//     WriteData(0x08);
//     WriteData(0x31);
//     WriteData(0xBE);
//     WriteData(0xA0);
//     WriteData(0x0A);
//     WriteData(0x31);
//     WriteData(0xBE);
//     WriteData(0xA0);
//     WriteData(0x04);
//     WriteData(0x31);
//     WriteData(0xBE);
//     WriteData(0xA0);
//     WriteData(0x06);
//     WriteData(0x31);
//     WriteData(0xBE);
//     WriteData(0xA0);
//     WriteComm(0xEA);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteComm(0xEB);
//     WriteData(0x02);
//     WriteData(0x02);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0xEC);
//     WriteData(0x02);
//     WriteData(0x00);
//     WriteComm(0xED);
//     WriteData(0xF5);
//     WriteData(0x47);
//     WriteData(0x6F);
//     WriteData(0x0B);
//     WriteData(0x8F);
//     WriteData(0x9F);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xF9);
//     WriteData(0xF8);
//     WriteData(0xB0);
//     WriteData(0xF6);
//     WriteData(0x74);
//     WriteData(0x5F);
//     WriteComm(0xEF);
//     WriteData(0x08);
//     WriteData(0x08);
//     WriteData(0x08);
//     WriteData(0x08);
//     WriteData(0x08);
//     WriteData(0x08);
//     WriteData(0x04);
//     WriteData(0x04);
//     WriteData(0x04);
//     WriteData(0x04);
//     WriteData(0x04);
//     WriteData(0x04);
//     //---------------------------------------------End GIP Setting-----------------------------------------------//
//     //------------------------------ Power Control Registers Initial End-----------------------------------//
//     //------------------------------------------Bank1 Setting----------------------------------------------------//
//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);

//     WriteComm(0x36);
//     WriteData(0x04);

//     WriteComm(0x3A);
//     WriteData(0x66);

//     WriteData(0x00);
//     WriteComm(0x29);
// }

// static void lcd_st7701s_480_854_true_ips_init(void)
// {
//     printk("lcd is 480x854, st7701s, true IPS, BL 12V\r\n");
//     WriteComm(0x11);
//     msleep(120);

//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x13);

//     WriteComm(0xEF);
//     WriteData(0x08);

//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x10);

//     WriteComm(0xC0);
//     WriteData(0xE9);
//     WriteData(0x03);

//     WriteComm(0xC1);
//     WriteData(0x09);
//     WriteData(0x02);

//     WriteComm(0xC2);
//     WriteData(0x01);
//     WriteData(0x08);

//     WriteComm(0xCC);
//     WriteData(0x10);

//     WriteComm(0xB0);
//     WriteData(0x00);
//     WriteData(0x0B);
//     WriteData(0x10);
//     WriteData(0x0D);
//     WriteData(0x11);
//     WriteData(0x06);
//     WriteData(0x01);
//     WriteData(0x08);
//     WriteData(0x08);
//     WriteData(0x1D);
//     WriteData(0x04);
//     WriteData(0x10);
//     WriteData(0x10);
//     WriteData(0x27);
//     WriteData(0x30);
//     WriteData(0x19);

//     WriteComm(0xB1);
//     WriteData(0x00);
//     WriteData(0x0B);
//     WriteData(0x14);
//     WriteData(0x0C);
//     WriteData(0x11);
//     WriteData(0x05);
//     WriteData(0x03);
//     WriteData(0x08);
//     WriteData(0x08);
//     WriteData(0x20);
//     WriteData(0x04);
//     WriteData(0x13);
//     WriteData(0x10);
//     WriteData(0x28);
//     WriteData(0x30);
//     WriteData(0x19);

//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x11);

//     WriteComm(0xB0);
//     WriteData(0x35);

//     WriteComm(0xB1);
//     WriteData(0x31);

//     WriteComm(0xB2);
//     WriteData(0x82);

//     WriteComm(0xB3);
//     WriteData(0x80);

//     WriteComm(0xB5);
//     WriteData(0x4E);

//     WriteComm(0xB7);
//     WriteData(0x85);

//     WriteComm(0xB8);
//     WriteData(0x20);

//     WriteComm(0xB9);
//     WriteData(0x10);

//     WriteComm(0xC1);
//     WriteData(0x78);

//     WriteComm(0xC2);
//     WriteData(0x78);

//     WriteComm(0xD0);
//     WriteData(0x88);
//     msleep(100);

//     WriteComm(0xE0);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x02);
//     WriteComm(0xE1);
//     WriteData(0x05);
//     WriteData(0xA0);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x04);
//     WriteData(0xA0);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x20);
//     WriteData(0x20);
//     WriteComm(0xE2);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0xE3);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x33);
//     WriteData(0x00);
//     WriteComm(0xE4);
//     WriteData(0x22);
//     WriteData(0x00);
//     WriteComm(0xE5);
//     WriteData(0x08);
//     WriteData(0x34);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x06);
//     WriteData(0x34);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0xE6);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x33);
//     WriteData(0x00);
//     WriteComm(0xE7);
//     WriteData(0x22);
//     WriteData(0x00);
//     WriteComm(0xE8);
//     WriteData(0x07);
//     WriteData(0x34);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x05);
//     WriteData(0x34);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0xEB);
//     WriteData(0x02);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteData(0x10);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0xEC);
//     WriteData(0x02);
//     WriteData(0x00);
//     WriteComm(0xED);
//     WriteData(0xAA);
//     WriteData(0x54);
//     WriteData(0x0B);
//     WriteData(0xBF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0xFB);
//     WriteData(0xB0);
//     WriteData(0x45);
//     WriteData(0xAA);
//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0x11);
//     msleep(120);
//     WriteComm(0x29);
//     msleep(20);
// }

// static void lcd_st7701s_360640_true_ips_init(void)
// {
//     printk("lcd is 360x640, st7701s, IPS, BL 3V\r\n");
//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x13);
//     WriteComm(0xEF);
//     WriteData(0x08);
//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x10);
//     WriteComm(0xC0);
//     WriteData(0x4F);
//     WriteData(0x00);
//     WriteComm(0xC1);
//     WriteData(0x10);
//     WriteData(0x02);
//     WriteComm(0xC2);
//     WriteData(0x01);
//     WriteData(0x02);
//     WriteComm(0xCC);
//     WriteData(0x18);
//     WriteComm(0xB0);
//     WriteData(0x40);
//     WriteData(0x0F);
//     WriteData(0x60);
//     WriteData(0x09);
//     WriteData(0x07);
//     WriteData(0x03);
//     WriteData(0x08);
//     WriteData(0x0A);
//     WriteData(0x06);
//     WriteData(0x23);
//     WriteData(0x02);
//     WriteData(0x17);
//     WriteData(0x15);
//     WriteData(0x67);
//     WriteData(0x67);
//     WriteData(0x6F);
//     WriteComm(0xB1);
//     WriteData(0x40);
//     WriteData(0x1F);
//     WriteData(0x59);
//     WriteData(0x10);
//     WriteData(0x12);
//     WriteData(0x09);
//     WriteData(0x09);
//     WriteData(0x09);
//     WriteData(0x05);
//     WriteData(0x23);
//     WriteData(0x04);
//     WriteData(0xCD);
//     WriteData(0x0B);
//     WriteData(0x67);
//     WriteData(0x6B);
//     WriteData(0x6F);
//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x11);
//     WriteComm(0xB0);
//     WriteData(0x4D);
//     WriteComm(0xB1);
//     WriteData(0x40);
//     WriteComm(0xB2);
//     WriteData(0x87);
//     WriteComm(0xB3);
//     WriteData(0x80);
//     WriteComm(0xB5);
//     WriteData(0x4E);
//     WriteComm(0xB7);
//     WriteData(0x85);
//     WriteComm(0xB8);
//     WriteData(0x20);
//     WriteComm(0xC1);
//     WriteData(0x78);
//     WriteComm(0xC2);
//     WriteData(0x78);
//     WriteComm(0xD0);
//     WriteData(0x88);
//     msleep(100);
//     WriteComm(0xE0);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x02);
//     WriteComm(0xE1);
//     WriteData(0x04);
//     WriteData(0xA0);
//     WriteData(0x06);
//     WriteData(0xA0);
//     WriteData(0x05);
//     WriteData(0xA0);
//     WriteData(0x07);
//     WriteData(0xA0);
//     WriteData(0x00);
//     WriteData(0x44);
//     WriteData(0x44);
//     WriteComm(0xE2);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x33);
//     WriteData(0x33);
//     WriteData(0x01);
//     WriteData(0xA0);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x01);
//     WriteData(0xA0);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0xE3);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x22);
//     WriteData(0x22);
//     WriteComm(0xE4);
//     WriteData(0x44);
//     WriteData(0x44);
//     WriteComm(0xE5);
//     WriteData(0x0C);
//     WriteData(0x90);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x0E);
//     WriteData(0x92);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x08);
//     WriteData(0x8B);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x0A);
//     WriteData(0x8D);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteComm(0xE6);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x22);
//     WriteData(0x22);
//     WriteComm(0xE7);
//     WriteData(0x44);
//     WriteData(0x44);
//     WriteComm(0xE8);
//     WriteData(0x0D);
//     WriteData(0x91);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x0F);
//     WriteData(0x93);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x09);
//     WriteData(0x8C);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteData(0x0B);
//     WriteData(0x8F);
//     WriteData(0xA0);
//     WriteData(0xA0);
//     WriteComm(0xEB);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0xE4);
//     WriteData(0xE4);
//     WriteData(0x44);
//     WriteData(0x88);
//     WriteData(0x00);
//     WriteComm(0xED);
//     WriteData(0xFF);
//     WriteData(0xFA);
//     WriteData(0xA1);
//     WriteData(0x0F);
//     WriteData(0x2B);
//     WriteData(0x45);
//     WriteData(0x67);
//     WriteData(0xFF);
//     WriteData(0xFF);
//     WriteData(0x76);
//     WriteData(0x54);
//     WriteData(0xB2);
//     WriteData(0xF0);
//     WriteData(0x1A);
//     WriteData(0xAF);
//     WriteData(0xFF);
//     WriteComm(0xFF);
//     WriteData(0x77);
//     WriteData(0x01);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteData(0x00);
//     WriteComm(0x36);
//     WriteData(0x00);
//     WriteComm(0x11);
//     msleep(120);
//     WriteComm(0x29);
//     msleep(20);
// }

// ///////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////
