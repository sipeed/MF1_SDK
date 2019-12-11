#include "board.h"

#include <math.h>

#include "dvp.h"
#include "fpioa.h"
#include "gpio.h"
#include "gpiohs.h"
#include "pwm.h"
#include "sleep.h"
#include "sysctl.h"
#include "uart.h"
#include "uarths.h"
#include "rtc.h"
#include "timer.h"

#include "audio.h"
#include "camera.h"
#include "lcd.h"
#include "sipeed_rtc.h"

#include "flash.h"
#include "face_lib.h"

#include "system_config.h"
#include "global_config.h"
///////////////////////////////////////////////////////////////////////////////
volatile board_cfg_t g_board_cfg;
volatile uint8_t g_rtc_tick_flag = 0;
volatile uint8_t g_key_press = 0;
volatile uint8_t g_key_long_press = 0;
uint8_t sKey_dir = 0;

///////////////////////////////////////////////////////////////////////////////
#if 0
#if (CONFIG_CAMERA_GC0328_DUAL)
uint8_t kpu_image_tmp[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3] __attribute__((aligned(128)));
#endif

#if CONFIG_LCD_VERTICAL
uint8_t display_image_ver[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2] __attribute__((aligned(64))); //显示
#endif

uint8_t kpu_image[2][CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3] __attribute__((aligned(128)));
uint8_t display_image[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2] __attribute__((aligned(64)));
#else
uint8_t kpu_image[2][CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3] __attribute__((aligned(128)));
uint8_t rgb_image[2][CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2] __attribute__((aligned(64)));
#endif
///////////////////////////////////////////////////////////////////////////////
static volatile uint8_t g_gpio_flag = 0;
static volatile uint64_t g_gpio_time = 0;

///////////////////////////////////////////////////////////////////////////////
static void my_dvp_reset(void);
static void my_dvp_init(uint8_t reg_len);
static uint32_t my_dvp_set_xclk_rate(uint32_t xclk_rate);
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
    if (g_gpio_flag)
    {
        uint64_t v_time_now = sysctl_get_time_us();

        if (v_time_now - g_gpio_time > 10 * 1000) /* press 10 ms  scan qrcode */
        {
            if (gpiohs_get_pin(CONFIG_GPIOHS_NUM_USER_KEY) == !sKey_dir)
            {
                g_key_press = 1;
                g_gpio_flag = 0;
            }
        }

        if (v_time_now - g_gpio_time > 2 * 1000 * 1000) /* long press 2s */
        {
            if (gpiohs_get_pin(CONFIG_GPIOHS_NUM_USER_KEY) == sKey_dir)
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

#ifndef CONFIG_NOT_MF1_BOARD
    //build for MF1
    fpioa_set_function(25, FUNC_CMOS_RST);
#else
    //build for Others
    fpioa_set_function(42, FUNC_CMOS_RST);
#endif

    /* change to 1.8V */
    sysctl_set_spi0_dvp_data(1);

#if CONFIG_ENABLE_IR_LED
    //IR LED
    fpioa_set_function(CONFIG_PIN_NUM_IR_LED, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_IR_LED);
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_IR_LED, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_IR_LED, 1 - CONFIG_IR_LED_OPEN_VOL);
#endif /* CONFIG_ENABLE_IR_LED */

#if CONFIG_ENABLE_FLASH_LED
    fpioa_set_function(CONFIG_PIN_NUM_FLASH_LED, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_FLASH_LED);
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_FLASH_LED, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_FLASH_LED, 1 - CONFIG_FLASH_LED_OPEN_VOL);

#define TIMER_NOR 3
#define TIMER_CHN 0
#define TIMER_PWM 1
#define TIMER_PWM_CHN 0
    /* Init timer */
    timer_init(TIMER_NOR);
    pwm_init(TIMER_PWM);
#endif /* CONFIG_ENABLE_FLASH_LED */

#if CONFIG_ENABLE_RGB_LED
    //RGB LED
    fpioa_set_function(CONFIG_PIN_NUM_RGB_LED_R, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_RGB_LED_R);
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RGB_LED_R, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_R, 1);

    fpioa_set_function(CONFIG_PIN_NUM_RGB_LED_G, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_RGB_LED_G);
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RGB_LED_G, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_G, 1);

    fpioa_set_function(CONFIG_PIN_NUM_RGB_LED_B, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_RGB_LED_B);
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_RGB_LED_B, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_B, 1);
#endif /* CONFIG_ENABLE_RGB_LED */

#if CONFIG_ENABLE_LCD_BL
    fpioa_set_function(CONFIG_PIN_NUM_LCD_BL, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_LCD_BL);
    gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_LCD_BL, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_LCD_BL, 1 - CONFIG_LCD_BL_OPEN_VOL);
#endif /* CONFIG_ENABLE_LCD_BL */

#if CONFIG_NET_ENABLE
#if CONFIG_NET_ESP8285
    //SPI WIFI
    fpioa_set_function(CONFIG_WIFI_PIN_TX, FUNC_GPIO0 + CONFIG_WIFI_GPIO_NUM_UART_TX);
    gpio_set_drive_mode(CONFIG_WIFI_GPIO_NUM_UART_TX, GPIO_DM_INPUT);

    fpioa_set_function(CONFIG_WIFI_PIN_TX, FUNC_GPIO0 + CONFIG_WIFI_GPIO_NUM_UART_RX);
    gpio_set_drive_mode(CONFIG_WIFI_GPIO_NUM_UART_RX, GPIO_DM_INPUT);

    fpioa_set_function(CONFIG_WIFI_PIN_ENABLE, FUNC_GPIOHS0 + CONFIG_WIFI_GPIOHS_NUM_ENABLE);
    gpiohs_set_drive_mode(CONFIG_WIFI_GPIOHS_NUM_ENABLE, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_WIFI_GPIOHS_NUM_ENABLE, 0); //disable WIFI

    /*
     GPIO   |   Name    |   K210    
=====================================
      12    |   MISO    |   2
      13    |   MOSI    |   3
      14    |   SCK     |   1
      15    |   SS      |   0
    */
    gpiohs_set_pin(CONFIG_WIFI_GPIOHS_NUM_ENABLE, 1); //enable WIFI

    fpioa_set_function(CONFIG_WIFI_PIN_SPI_CS, FUNC_GPIOHS0 + CONFIG_WIFI_GPIOHS_NUM_CS); //CS
    fpioa_set_function(CONFIG_WIFI_PIN_SPI_MISO, FUNC_SPI1_D1);                           //MISO
    fpioa_set_function(CONFIG_WIFI_PIN_SPI_MOSI, FUNC_SPI1_D0);                           //MOSI
    fpioa_set_function(CONFIG_WIFI_PIN_SPI_SCLK, FUNC_SPI1_SCLK);                         //CLK
#elif CONFIG_NET_W5500
    /* ETH */
    fpioa_set_function(CONFIG_PIN_NUM_ETH_W5500_CS, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_W5500_CS); //CSS
    // fpioa_set_function(CONFIG_ETH_PIN_RST, FUNC_GPIOHS0 + CONFIG_ETH_GPIOHS_NUM_RST); //RST

    fpioa_set_function(CONFIG_PIN_NUM_ETH_W5500_MISO, FUNC_SPI1_D1);   //MISO
    fpioa_set_function(CONFIG_PIN_NUM_ETH_W5500_MOSI, FUNC_SPI1_D0);   //MOSI
    fpioa_set_function(CONFIG_PIN_NUM_ETH_W5500_SCLK, FUNC_SPI1_SCLK); //CLK
#else
#error("no net if select")
#endif
#endif
}

///////////////////////////////////////////////////////////////////////////////
void set_IR_LED(int state)
{
#if CONFIG_ENABLE_IR_LED
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_IR_LED, state ? CONFIG_IR_LED_OPEN_VOL : 1 - CONFIG_IR_LED_OPEN_VOL);
    // gpiohs_set_pin(CONFIG_GPIOHS_NUM_IR_LED, CONFIG_IR_LED_OPEN_VOL);
#endif /* CONFIG_ENABLE_IR_LED */
    return;
}

void set_W_LED(int state)
{
#if CONFIG_ENABLE_FLASH_LED

    if (state)
    {
        fpioa_set_function(CONFIG_PIN_NUM_FLASH_LED, FUNC_TIMER1_TOGGLE1);
        timer_set_enable(TIMER_NOR, TIMER_CHN, 1);
        pwm_set_frequency(TIMER_PWM, TIMER_PWM_CHN, 5 * 1000, 0.9);
        pwm_set_enable(TIMER_PWM, TIMER_PWM_CHN, 1);
    }
    else
    {
        fpioa_set_function(CONFIG_PIN_NUM_FLASH_LED, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_FLASH_LED);
        gpiohs_set_drive_mode(CONFIG_GPIOHS_NUM_FLASH_LED, GPIO_DM_OUTPUT);
        gpiohs_set_pin(CONFIG_GPIOHS_NUM_FLASH_LED, 1 - CONFIG_FLASH_LED_OPEN_VOL);
    }
    // pwm_set_enable(TIMER_PWM, TIMER_PWM_CHN, state ? 0 : 1);
    // gpiohs_set_pin(CONFIG_GPIOHS_NUM_FLASH_LED, state ? CONFIG_FLASH_LED_OPEN_VOL : 1 - CONFIG_FLASH_LED_OPEN_VOL);
    // gpiohs_set_pin(CONFIG_GPIOHS_NUM_FLASH_LED, CONFIG_FLASH_LED_OPEN_VOL);
#endif /* CONFIG_ENABLE_FLASH_LED */
    return;
}

void set_lcd_bl(int stat)
{
#if CONFIG_ENABLE_LCD_BL
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_LCD_BL, stat ? CONFIG_LCD_BL_OPEN_VOL : 1 - CONFIG_LCD_BL_OPEN_VOL);
#endif /* CONFIG_ENABLE_LCD_BL */
}

///////////////////////////////////////////////////////////////////////////////
void web_set_RGB_LED(uint8_t val[3])
{
#if CONFIG_ENABLE_RGB_LED
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_R, (val[0]) ? 0 : 1);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_G, (val[1]) ? 0 : 1);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_B, (val[2]) ? 0 : 1);
#endif
    return;
}

void set_RGB_LED(int state)
{
#if CONFIG_ENABLE_RGB_LED
    state = state & 0x07;
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_R, (state & RLED) ? 0 : 1);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_G, (state & GLED) ? 0 : 1);
    gpiohs_set_pin(CONFIG_GPIOHS_NUM_RGB_LED_B, (state & BLED) ? 0 : 1);
#endif
    return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if CONFIG_ENABLE_RTC
static int rtc_tick_cb(void *ctx)
{
    printk("rtc\r\n");
    g_rtc_tick_flag = 1;
    return 0;
}
#endif /* CONFIG_ENABLE_RTC */

#if CONFIG_ENABLE_WECHAT
void update_rtc_time(mqtt_rtc_time_t *rtc_time)
{
    int year, month, day, hour, min, sec;

    if (rtc_time == NULL)
    {
        return;
    }

    year = rtc_time->year;
    month = rtc_time->mon;
    day = rtc_time->day;
    hour = rtc_time->hour;
    min = rtc_time->min;
    sec = rtc_time->sec;

    rtc_timer_set(year, month, day, hour, min, sec);

#if CONFIG_ENABLE_RTC
    struct sipeed_rtc_time time;
    time.tm_sec = (uint8_t)sec;
    time.tm_min = (uint8_t)min;
    time.tm_hour = (uint8_t)hour;
    time.tm_mday = (uint8_t)day;
    time.tm_mon = (uint8_t)month;
    time.tm_year = (uint16_t)year;
    sipeed_rtc_write_time(&time);
#endif /* CONFIG_ENABLE_RTC */

    update_time();
    return;
}

uint64_t rtc_get_time_timestamp(mqtt_rtc_time_t *rtc_time)
{
    int year, mon, day, hour, min, sec;
    uint64_t timestamp = 0;

    rtc_timer_get(&year, &mon, &day, &hour, &min, &sec);

    printk("read time: %04d-%02d-%02d %02d:%02d:%02d\r\n", year, mon, day, hour, min, sec);

    //January and February are counted as months 13 and 14 of the previous year
    if (mon <= 2)
    {
        mon += 12;
        year -= 1;
    }

    hour -= 8; /* 东八区 */
    //Convert years to days
    timestamp = (365 * year) + (year / 4) - (year / 100) + (year / 400);
    //Convert months to days
    timestamp += (30 * mon) + (3 * (mon + 1) / 5) + day;
    //Unix time starts on January 1st, 1970
    timestamp -= 719561;
    //Convert days to seconds
    timestamp *= 86400;
    //Add hours, minutes and seconds
    timestamp += (3600 * hour) + (60 * min) + sec;

    printk("timestamp: %ld\r\n", timestamp);

    if (rtc_time)
    {
        rtc_time->year = year;
        rtc_time->mon = mon;
        rtc_time->day = day;
        rtc_time->hour = hour;
        rtc_time->min = min;
        rtc_time->sec = sec;

        rtc_time->time_stamp = timestamp;
    }

    return timestamp;
}
#endif /* CONFIG_ENABLE_WECHAT */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
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

    /* Flash init */
    flash_init();

    set_IR_LED(0);

#if CONFIG_ENABLE_SPK
#if CONFIG_SPK_TYPE_ES8374
    audio_init(AUDIO_TYPE_ES8374);
#elif CONFIG_SPK_TYPE_PT8211
    audio_init(AUDIO_TYPE_PT8211);
#endif /* CONFIG_SPK_TYPE_ES8374 */
#endif /* CONFIG_ENABLE_SPK */

#if CONFIG_ENABLE_RTC
#if CONFIG_RTC_TYPE_GM1302
    sipeed_rtc_init(RTC_TYPE_GM1302);
#elif CONFIG_RTC_TYPE_BM8563
    sipeed_rtc_init(RTC_TYPE_BM8563);
#endif /* CONFIG_RTC_TYPE_GM1302 */

    /* here we use K210's RTC */
    struct sipeed_rtc_time t2;

    sipeed_rtc_read_time(&t2);
    printk("set rtc: %04d-%02d-%02d %02d:%02d:%02d %d\r\n", t2.tm_year, t2.tm_mon, t2.tm_mday,
           t2.tm_hour, t2.tm_min, t2.tm_sec, t2.tm_wday);

    rtc_init();
    rtc_timer_set(t2.tm_year, t2.tm_mon, t2.tm_mday,
                  t2.tm_hour, t2.tm_min, t2.tm_sec);
    rtc_tick_irq_register(0, RTC_INT_MINUTE, rtc_tick_cb, NULL, 2);
#endif /* CONFIG_ENABLE_RTC */

    /* DVP init */
#ifndef CONFIG_NOT_MF1_BOARD
    //build for MF1
    my_dvp_init(8);
    my_dvp_set_xclk_rate(48000000);
    // my_dvp_set_xclk_rate(36000000);
    // my_dvp_set_xclk_rate(24000000);
#else
    //build for Others
    dvp_init(8);
    dvp_set_xclk_rate(24000000);
#endif

    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(CONFIG_CAMERA_RESOLUTION_WIDTH, CONFIG_CAMERA_RESOLUTION_HEIGHT);

#if CONFIG_CAMERA_OV2640
    camera_init(CAM_OV2640);
#elif CONFIG_CAMERA_GC0328_SINGLE
    camera_init(CAM_GC0328_SINGLE);
#elif CONFIG_CAMERA_GC0328_DUAL
    camera_init(CAM_GC0328_DUAL);
#else
    printf("unknown camera type!!!\r\n");
#endif

    dvp_set_ai_addr(_IOMEM_ADDR(kpu_image[1]),
                    _IOMEM_ADDR(kpu_image[1]) + CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT,
                    _IOMEM_ADDR(kpu_image[1]) + CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2);
    dvp_set_display_addr(rgb_image[1]);

    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /* LCD init */
#if CONFIG_LCD_TYPE_ST7789
    lcd_init(LCD_ST7789);
    lcd_clear(RED);
#elif CONFIG_LCD_TYPE_SIPEED
    printf("unsupport lcd type!!!\r\n");
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
    if (v_apb1_clk > xclk_rate * 2)
        v_period = round(v_apb1_clk / (xclk_rate * 2.0)) - 1;
    else
        v_period = 0;
    if (v_period > 255)
        v_period = 255;
    dvp->cmos_cfg &= (~DVP_CMOS_CLK_DIV_MASK);
    dvp->cmos_cfg |= DVP_CMOS_CLK_DIV(v_period) | DVP_CMOS_CLK_ENABLE;
    my_dvp_reset();
    return v_apb1_clk / ((v_period + 1) * 2);
}
