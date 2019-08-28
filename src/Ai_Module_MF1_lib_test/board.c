#include "board.h"

#include <math.h>

#include "dvp.h"
#include "fpioa.h"
#include "gpio.h"
#include "gpiohs.h"
#include "pwm.h"
#include "rtc.h"
#include "sleep.h"
#include "sysctl.h"
#include "uart.h"
#include "uarths.h"

#if CONFIG_LCD_TYPE_ST7789
#include "lcd_st7789.h"
#elif CONFIG_LCD_TYPE_SIPEED
#include "lcd_sipeed.h"
#endif

///////////////////////////////////////////////////////////////////////////////
volatile uint8_t g_key_press = 0;
volatile uint8_t g_key_long_press = 0;
uint8_t sKey_dir = 0;

volatile board_cfg_t g_board_cfg;

///////////////////////////////////////////////////////////////////////////////
uint8_t kpu_image_tmp[IMG_W * IMG_H * 3] __attribute__((aligned(128)));
uint8_t kpu_image[2][IMG_W * IMG_H * 3] __attribute__((aligned(128)));
uint8_t display_image[IMG_W * IMG_H * 2] __attribute__((aligned(64)));

#if CONFIG_LCD_TYPE_SSD1963
uint8_t display_image_rgb888[IMG_W * IMG_H * 3] __attribute__((aligned(64)));
#endif

#if CONFIG_LCD_TYPE_SIPEED
uint8_t lcd_image[LCD_W * LCD_H * 2] __attribute__((aligned(64)));
#endif

///////////////////////////////////////////////////////////////////////////////
static volatile uint8_t g_gpio_flag = 0;
static volatile uint64_t g_gpio_time = 0;

///////////////////////////////////////////////////////////////////////////////
static void my_dvp_reset(void);
static void my_dvp_init(uint8_t reg_len);
static uint32_t my_dvp_set_xclk_rate(uint32_t xclk_rate);
static void myuarths_init(uint32_t uart_freq);
///////////////////////////////////////////////////////////////////////////////
int irq_gpiohs(void *ctx)
{
    g_gpio_flag = 1;
    g_gpio_time = sysctl_get_time_us();
    return 0;
}

void update_key_state(void)
{
    g_key_press = 0;
    if(g_gpio_flag)
    {
        uint64_t v_time_now = sysctl_get_time_us();

        if(v_time_now - g_gpio_time > 10 * 1000) /* press 10 ms  scan qrcode */
        {
            if(gpiohs_get_pin(KEY_HS_NUM) == !sKey_dir)
            {
                printk("key press!\n");
                g_key_press = 1;
                g_gpio_flag = 0;
            }
        }

        if(v_time_now - g_gpio_time > 2 * 1000 * 1000) /* long press 2s */
        {
            if(gpiohs_get_pin(KEY_HS_NUM) == sKey_dir)
            {
                g_key_long_press = 1;
                g_gpio_flag = 0;
            }
        }
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
static void io_set_power(void)
{
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
}

#define TIMER_PWM 1
#define TIMER_PWM_CHN 0

static void io_mux_init(void)
{
    /* DVP */
    fpioa_set_function(47, FUNC_CMOS_PCLK);
    fpioa_set_function(46, FUNC_CMOS_XCLK);
    fpioa_set_function(45, FUNC_CMOS_HREF);
    fpioa_set_function(44, FUNC_CMOS_PWDN);
    fpioa_set_function(43, FUNC_CMOS_VSYNC);

    /* LCD */
    fpioa_set_function(LCD_RST_PIN, FUNC_GPIOHS0 + LCD_RST_HS_NUM);
    fpioa_set_function(LCD_DCX_PIN, FUNC_GPIOHS0 + LCD_DCX_HS_NUM);
    fpioa_set_function(LCD_WRX_PIN, FUNC_SPI0_SS3);
    fpioa_set_function(LCD_SCK_PIN, FUNC_SPI0_SCLK);

#if CONFIG_LCD_TYPE_SIPEED
    fpioa_set_io_driving(LCD_SCK_PIN, FPIOA_DRIVING_7);
#endif

    /* change to 1.8V */
    sysctl_set_spi0_dvp_data(1);

    //LCD BL
    fpioa_set_function(LCD_BL_PIN, FUNC_TIMER1_TOGGLE1);
    pwm_init(TIMER_PWM);
    pwm_set_frequency(TIMER_PWM, TIMER_PWM_CHN, 1000, 0.95); //neg, 1 dark
    pwm_set_enable(TIMER_PWM, TIMER_PWM_CHN, 1);

    //IR LED
    fpioa_set_function(IR_LED_PIN, FUNC_GPIOHS0 + IR_LED_HS_NUM);
    gpiohs_set_drive_mode(IR_LED_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(IR_LED_HS_NUM, 1);

    //RGB LED
    fpioa_set_function(RGB_LED_R_PIN, FUNC_GPIOHS0 + RGB_LED_R_HS_NUM);
    gpiohs_set_drive_mode(RGB_LED_R_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(RGB_LED_R_HS_NUM, 1);

    fpioa_set_function(RGB_LED_G_PIN, FUNC_GPIOHS0 + RGB_LED_G_HS_NUM);
    gpiohs_set_drive_mode(RGB_LED_G_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(RGB_LED_G_HS_NUM, 1);

    fpioa_set_function(RGB_LED_B_PIN, FUNC_GPIOHS0 + RGB_LED_B_HS_NUM);
    gpiohs_set_drive_mode(RGB_LED_B_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(RGB_LED_B_HS_NUM, 1);

    //SPI WIFI
    fpioa_set_function(WIFI_TX_PIN, FUNC_GPIO0 + WIFI_TX_IO_NUM);
    gpio_set_drive_mode(WIFI_TX_IO_NUM, GPIO_DM_INPUT);

    fpioa_set_function(WIFI_RX_PIN, FUNC_GPIO0 + WIFI_RX_IO_NUM);
    gpio_set_drive_mode(WIFI_RX_IO_NUM, GPIO_DM_INPUT);

    fpioa_set_function(WIFI_EN_PIN, FUNC_GPIOHS0 + WIFI_EN_HS_NUM);
    gpiohs_set_drive_mode(WIFI_EN_HS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(WIFI_EN_HS_NUM, 0); //disable WIFI

#if CONFIG_ENABLE_WIFI
    /*
     GPIO   |   Name    |   K210    
=====================================
      12    |   MISO    |   2
      13    |   MOSI    |   3
      14    |   SCK     |   1
      15    |   SS      |   0
    */
    gpiohs_set_pin(WIFI_EN_HS_NUM, 1); //enable WIFI

    fpioa_set_function(WIFI_SPI_CSXX_PIN, FUNC_GPIOHS0 + WIFI_SPI_SS_HS_NUM); //CS
    fpioa_set_function(WIFI_SPI_MISO_PIN, FUNC_SPI1_D1);                      //MISO
    fpioa_set_function(WIFI_SPI_MOSI_PIN, FUNC_SPI1_D0);                      //MOSI
    fpioa_set_function(WIFI_SPI_SCLK_PIN, FUNC_SPI1_SCLK);                    //CLK
#endif
}
///////////////////////////////////////////////////////////////////////////////
void set_IR_LED(int state)
{
    gpiohs_set_pin(IR_LED_HS_NUM, state);
    return;
}
///////////////////////////////////////////////////////////////////////////////

uint32_t rgb_state = 0;
void set_RGB_LED(int state)
{
    rgb_state = state & 0x07;
    gpiohs_set_pin(RGB_LED_R_HS_NUM, (rgb_state & RLED) ? 0 : 1);
    gpiohs_set_pin(RGB_LED_G_HS_NUM, (rgb_state & GLED) ? 0 : 1);
    gpiohs_set_pin(RGB_LED_B_HS_NUM, (rgb_state & BLED) ? 0 : 1);
    return;
}

void change_RGB_LED(int led, int state)
{
    if(led == RLED)
        gpiohs_set_pin(RGB_LED_R_HS_NUM, (state & RLED) ? 0 : 1);
    if(led == GLED)
        gpiohs_set_pin(RGB_LED_G_HS_NUM, (state & GLED) ? 0 : 1);
    if(led == BLED)
        gpiohs_set_pin(RGB_LED_B_HS_NUM, (state & BLED) ? 0 : 1);
    if(state)
        rgb_state = rgb_state | led;
    else
        rgb_state = rgb_state & (~led);
    return;
}
///////////////////////////////////////////////////////////////////////////////

void get_date_time(void)
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    rtc_timer_get(&year, &month, &day, &hour, &minute, &second);
    printk("%4d-%02d-%02d %02d:%02d:%02d\n", year, month, day, hour, minute, second);
}
///////////////////////////////////////////////////////////////////////////////

void board_init(void)
{
    /* Set CPU and dvp clk */
    sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_OUTPUT_FREQ);
    sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_OUTPUT_FREQ);
    sysctl_clock_enable(SYSCTL_CLOCK_AI);
    io_set_power();
    plic_init();
    io_mux_init();

    set_IR_LED(0);

    /* DVP init */
    my_dvp_init(8);
    my_dvp_set_xclk_rate(48000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(IMG_W, IMG_H);
    gc0328_init();

    dvp_set_ai_addr((uint32_t)kpu_image, (uint32_t)(kpu_image + IMG_W * IMG_H), (uint32_t)(kpu_image + IMG_W * IMG_H * 2));
    dvp_set_display_addr((uint32_t)display_image);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /* Flash init */
    flash_init();

    /* RTC init */
    rtc_init();
    rtc_timer_set(2019, 5, 1, 12, 00, 00);

    /* LCD init */
#if CONFIG_LCD_TYPE_ST7789
    lcd_init();
    lcd_clear(BLUE);
#elif CONFIG_LCD_TYPE_SSD1963
    /* CONFIG_LCD_TYPE_SSD1963 */
    lcd_init(&lcd_480x272_4_3inch);
    lcd_clear(lcd_color(0x00, 0x00, 0x00)); /* slow */
#elif CONFIG_LCD_TYPE_SIPEED
    lcd_init(); //delay 500ms...
#endif

    /* DVP interrupt config */
    plic_set_priority(IRQN_DVP_INTERRUPT, 1);
    plic_irq_register(IRQN_DVP_INTERRUPT, dvp_irq, NULL);
    plic_irq_enable(IRQN_DVP_INTERRUPT);

    /* enable global interrupt */
    sysctl_enable_irq();

    /* system start */
    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
    
    return;
}
///////////////////////////////////////////////////////////////////////////////

static void my_dvp_reset(void)
{
    /* First power down */
    dvp->cmos_cfg |= DVP_CMOS_POWER_DOWN;
    msleep(5);
    dvp->cmos_cfg &= ~DVP_CMOS_POWER_DOWN;
    msleep(5);

    /* Second reset */
    dvp->cmos_cfg &= ~DVP_CMOS_RESET;
    msleep(5);
    dvp->cmos_cfg |= DVP_CMOS_RESET;
    msleep(5);
}

static void my_dvp_init(uint8_t reg_len)
{
    sysctl_clock_enable(SYSCTL_CLOCK_DVP);
    sysctl_reset(SYSCTL_RESET_DVP);
    dvp->cmos_cfg &= (~DVP_CMOS_CLK_DIV_MASK);
    dvp->cmos_cfg |= DVP_CMOS_CLK_DIV(3) | DVP_CMOS_CLK_ENABLE;
    my_dvp_reset();
}

static uint32_t my_dvp_set_xclk_rate(uint32_t xclk_rate)
{
    uint32_t v_apb1_clk = sysctl_clock_get_freq(SYSCTL_CLOCK_APB1);
    uint32_t v_period;
    if(v_apb1_clk > xclk_rate * 2)
        v_period = round(v_apb1_clk / (xclk_rate * 2.0)) - 1;
    else
        v_period = 0;
    if(v_period > 255)
        v_period = 255;
    dvp->cmos_cfg &= (~DVP_CMOS_CLK_DIV_MASK);
    dvp->cmos_cfg |= DVP_CMOS_CLK_DIV(v_period) | DVP_CMOS_CLK_ENABLE;
    my_dvp_reset();
    return v_apb1_clk / ((v_period + 1) * 2);
}

static void myuarths_init(uint32_t uart_freq)
{
    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
    uint16_t div = freq / uart_freq - 1;

    /* Set UART registers */
    uarths->div.div = div;
    uarths->txctrl.txen = 1;
    uarths->rxctrl.rxen = 1;
    uarths->txctrl.txcnt = 0;
    uarths->rxctrl.rxcnt = 0;
    uarths->ip.txwm = 1;
    uarths->ip.rxwm = 1;
    uarths->ie.txwm = 0;
    uarths->ie.rxwm = 1;
}
