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

#include "camera.h"
#include "lcd.h"

#include "face_lib.h"

#include "system_config.h"
#include "global_config.h"
///////////////////////////////////////////////////////////////////////////////
volatile board_cfg_t g_board_cfg;

volatile uint8_t g_key_press = 0;
volatile uint8_t g_key_long_press = 0;
uint8_t sKey_dir = 0;

///////////////////////////////////////////////////////////////////////////////
uint8_t kpu_image[2][CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3] __attribute__((aligned(128)));
uint8_t cam_image[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2] __attribute__((aligned(64)));

//如果是双摄,需要多一个缓存
#if (CONFIG_CAMERA_GC0328_DUAL)
uint8_t kpu_image_tmp[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3] __attribute__((aligned(128)));
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
            if (gpiohs_get_pin(CONFIG_FUNCTION_KEY_GPIOHS_NUM) == !sKey_dir)
            {
                g_key_press = 1;
                g_gpio_flag = 0;
            }
        }

        if (v_time_now - g_gpio_time > 2 * 1000 * 1000) /* long press 2s */
        {
            if (gpiohs_get_pin(CONFIG_FUNCTION_KEY_GPIOHS_NUM) == sKey_dir)
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

    /* LCD */
    fpioa_set_function(CONFIG_LCD_PIN_RST, FUNC_GPIOHS0 + CONFIG_LCD_GPIOHS_RST);
    fpioa_set_function(CONFIG_LCD_PIN_DCX, FUNC_GPIOHS0 + CONFIG_LCD_GPIOHS_DCX);
    fpioa_set_function(CONFIG_LCD_PIN_WRX, FUNC_SPI0_SS3);
    fpioa_set_function(CONFIG_LCD_PIN_SCK, FUNC_SPI0_SCLK);

#if CONFIG_LCD_TYPE_SIPEED
    fpioa_set_io_driving(CONFIG_LCD_PIN_SCK, FPIOA_DRIVING_7);
#endif

    /* change to 1.8V */
    sysctl_set_spi0_dvp_data(1);

    //LCD BL
    fpioa_set_function(CONFIG_LCD_PIN_BL, FUNC_TIMER1_TOGGLE1);
    pwm_init(TIMER_PWM);
    pwm_set_frequency(TIMER_PWM, TIMER_PWM_CHN, 1000, 0.95); //neg, 1 dark
    pwm_set_enable(TIMER_PWM, TIMER_PWM_CHN, 1);

    //IR LED
    fpioa_set_function(CONFIG_INFRARED_LED_PIN, FUNC_GPIOHS0 + CONFIG_INFRARED_GPIOHS_NUM);
    gpiohs_set_drive_mode(CONFIG_INFRARED_GPIOHS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_INFRARED_GPIOHS_NUM, 1);

    //RGB LED
    fpioa_set_function(CONFIG_LED_R_PIN, FUNC_GPIOHS0 + CONFIG_LED_R_GPIOHS_NUM);
    gpiohs_set_drive_mode(CONFIG_LED_R_GPIOHS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LED_R_GPIOHS_NUM, 1);

    fpioa_set_function(CONFIG_LED_G_PIN, FUNC_GPIOHS0 + CONFIG_LED_G_GPIOHS_NUM);
    gpiohs_set_drive_mode(CONFIG_LED_G_GPIOHS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LED_G_GPIOHS_NUM, 1);

    fpioa_set_function(CONFIG_LED_B_PIN, FUNC_GPIOHS0 + CONFIG_LED_B_GPIOHS_NUM);
    gpiohs_set_drive_mode(CONFIG_LED_B_GPIOHS_NUM, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_LED_B_GPIOHS_NUM, 1);

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
    fpioa_set_function(CONFIG_ETH_PIN_CS, FUNC_GPIOHS0 + CONFIG_ETH_GPIOHS_NUM_CS); //CSS
    // fpioa_set_function(CONFIG_ETH_PIN_RST, FUNC_GPIOHS0 + CONFIG_ETH_GPIOHS_NUM_RST); //RST

    fpioa_set_function(CONFIG_ETH_PIN_MISO, FUNC_SPI1_D1);   //MISO
    fpioa_set_function(CONFIG_ETH_PIN_MOSI, FUNC_SPI1_D0);   //MOSI
    fpioa_set_function(CONFIG_ETH_PIN_SCLK, FUNC_SPI1_SCLK); //CLK
#else
#error("no net if select")
#endif
#endif
}

///////////////////////////////////////////////////////////////////////////////
void set_IR_LED(int state)
{
    gpiohs_set_pin(CONFIG_INFRARED_GPIOHS_NUM, state);
    return;
}

void set_W_LED(int state)
{
    // gpiohs_set_pin(CONFIG_INFRARED_GPIOHS_NUM, state);
    return;
}

///////////////////////////////////////////////////////////////////////////////
void web_set_RGB_LED(uint8_t val[3])
{
    gpiohs_set_pin(CONFIG_LED_R_GPIOHS_NUM, (val[0]) ? 0 : 1);
    gpiohs_set_pin(CONFIG_LED_G_GPIOHS_NUM, (val[1]) ? 0 : 1);
    gpiohs_set_pin(CONFIG_LED_B_GPIOHS_NUM, (val[2]) ? 0 : 1);
    return;
}

void set_RGB_LED(int state)
{
    state = state & 0x07;
    gpiohs_set_pin(CONFIG_LED_R_GPIOHS_NUM, (state & RLED) ? 0 : 1);
    gpiohs_set_pin(CONFIG_LED_G_GPIOHS_NUM, (state & GLED) ? 0 : 1);
    gpiohs_set_pin(CONFIG_LED_B_GPIOHS_NUM, (state & BLED) ? 0 : 1);
    return;
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

    /* Flash init */
    flash_init();

    /* DVP init */
#ifndef CONFIG_NOT_MF1_BOARD
    //build for MF1
    my_dvp_init(8);
    my_dvp_set_xclk_rate(48000000);
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

    dvp_set_ai_addr((uint32_t)_IOMEM_ADDR(kpu_image),
                    (uint32_t)(_IOMEM_ADDR(kpu_image) + CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT),
                    (uint32_t)(_IOMEM_ADDR(kpu_image) + CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 2));

    dvp_set_display_addr((uint32_t)_IOMEM_ADDR(cam_image));
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /* DVP interrupt config */
    plic_set_priority(IRQN_DVP_INTERRUPT, 1);
    plic_irq_register(IRQN_DVP_INTERRUPT, dvp_irq, NULL);
    plic_irq_enable(IRQN_DVP_INTERRUPT);

#if CONFIG_CAMERA_OV2640
    camera_init(CAM_OV2640);
#elif CONFIG_CAMERA_GC0328_SINGLE
    camera_init(CAM_GC0328_SINGLE);
#elif CONFIG_CAMERA_GC0328_DUAL
    camera_init(CAM_GC0328_DUAL);
#else
    printf("unknown camera type!!!\r\n");
#endif

    /* LCD init */
#if CONFIG_LCD_TYPE_ST7789
    lcd_init(LCD_ST7789);
    lcd_clear(RED);
#elif CONFIG_LCD_TYPE_SIPEED
    extern uint8_t *lcd_image;
    extern uint8_t *lcd_banner_image;
    extern uint8_t lcd_sipeed_config_disp_buf(uint8_t * lcd_disp_buf, uint8_t * lcd_disp_banner_buf);

    w25qxx_read_data(IMG_BAR_800480_ADDR, lcd_banner_image, SIPEED_LCD_BANNER_W * SIPEED_LCD_BANNER_H * 2);

    lcd_sipeed_config_disp_buf(lcd_image, lcd_banner_image);

    lcd_init(LCD_SIPEED);
    // (((r << 8) & 0xF800) | ((g << 3) & 0x7E0) | (b >> 3))
    lcd_clear((((1 << 8) & 0xF800) | ((107 << 3) & 0x7E0) | (168 >> 3)));
#endif /*CONFIG_LCD_TYPE_ST7789*/

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
