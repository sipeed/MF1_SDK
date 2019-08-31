#include "lcd_st7789.h"

#include "sleep.h"

#include "nt35310.h"

/* clang-format off */
#define NO_OPERATION            0x00
#define SOFTWARE_RESET          0x01
#define READ_ID                 0x04
#define READ_STATUS             0x09
#define READ_POWER_MODE         0x0A
#define READ_MADCTL             0x0B
#define READ_PIXEL_FORMAT       0x0C
#define READ_IMAGE_FORMAT       0x0D
#define READ_SIGNAL_MODE        0x0E
#define READ_SELT_DIAG_RESULT   0x0F
#define SLEEP_ON                0x10
#define SLEEP_OFF               0x11
#define PARTIAL_DISPALY_ON      0x12
#define NORMAL_DISPALY_ON       0x13
#define INVERSION_DISPALY_OFF   0x20
#define INVERSION_DISPALY_ON    0x21
#define GAMMA_SET               0x26
#define DISPALY_OFF             0x28
#define DISPALY_ON              0x29
#define HORIZONTAL_ADDRESS_SET  0x2A
#define VERTICAL_ADDRESS_SET    0x2B
#define MEMORY_WRITE            0x2C
#define COLOR_SET               0x2D
#define MEMORY_READ             0x2E
#define PARTIAL_AREA            0x30
#define VERTICAL_SCROL_DEFINE   0x33
#define TEAR_EFFECT_LINE_OFF    0x34
#define TEAR_EFFECT_LINE_ON     0x35
#define MEMORY_ACCESS_CTL       0x36
#define VERTICAL_SCROL_S_ADD    0x37
#define IDLE_MODE_OFF           0x38
#define IDLE_MODE_ON            0x39
#define PIXEL_FORMAT_SET        0x3A
#define WRITE_MEMORY_CONTINUE   0x3C
#define READ_MEMORY_CONTINUE    0x3E
#define SET_TEAR_SCANLINE       0x44
#define GET_SCANLINE            0x45
#define WRITE_BRIGHTNESS        0x51
#define READ_BRIGHTNESS         0x52
#define WRITE_CTRL_DISPALY      0x53
#define READ_CTRL_DISPALY       0x54
#define WRITE_BRIGHTNESS_CTL    0x55
#define READ_BRIGHTNESS_CTL     0x56
#define WRITE_MIN_BRIGHTNESS    0x5E
#define READ_MIN_BRIGHTNESS     0x5F
#define READ_ID1                0xDA
#define READ_ID2                0xDB
#define READ_ID3                0xDC
#define RGB_IF_SIGNAL_CTL       0xB0
#define NORMAL_FRAME_CTL        0xB1
#define IDLE_FRAME_CTL          0xB2
#define PARTIAL_FRAME_CTL       0xB3
#define INVERSION_CTL           0xB4
#define BLANK_PORCH_CTL         0xB5
#define DISPALY_FUNCTION_CTL    0xB6
#define ENTRY_MODE_SET          0xB7
#define BACKLIGHT_CTL1          0xB8
#define BACKLIGHT_CTL2          0xB9
#define BACKLIGHT_CTL3          0xBA
#define BACKLIGHT_CTL4          0xBB
#define BACKLIGHT_CTL5          0xBC
#define BACKLIGHT_CTL7          0xBE
#define BACKLIGHT_CTL8          0xBF
#define POWER_CTL1              0xC0
#define POWER_CTL2              0xC1
#define VCOM_CTL1               0xC5
#define VCOM_CTL2               0xC7
#define NV_MEMORY_WRITE         0xD0
#define NV_MEMORY_PROTECT_KEY   0xD1
#define NV_MEMORY_STATUS_READ   0xD2
#define READ_ID4                0xD3
#define POSITIVE_GAMMA_CORRECT  0xE0
#define NEGATIVE_GAMMA_CORRECT  0xE1
#define DIGITAL_GAMMA_CTL1      0xE2
#define DIGITAL_GAMMA_CTL2      0xE3
#define INTERFACE_CTL           0xF6
/* clang-format on */

static int lcd_st7789_config(lcd_t *lcd)
{
    uint8_t data = 0;

    tft_hard_init();
    /*soft reset*/
    tft_write_command(SOFTWARE_RESET);
    usleep(6 * 1000);
    /*exit sleep*/
    tft_write_command(SLEEP_OFF);
    usleep(6 * 1000);
    /*pixel format*/
    tft_write_command(PIXEL_FORMAT_SET);
    data = 0x05;
    tft_write_byte(&data, 1);
#if (CONFIG_LCD_TYPE_ST7789 && CONFIG_LCD_WIDTH == 240)
    tft_write_command(INVERSION_DISPALY_ON);
#endif
    tft_write_command(WRITE_BRIGHTNESS_CTL); //CACB
    data = 0xb3;
    tft_write_byte(&data, 1);
    tft_write_command(WRITE_MIN_BRIGHTNESS);
    data = 0x80;
    tft_write_byte(&data, 1);
    tft_write_command(DISPALY_ON);
    return 0;
}

static int lcd_st7789_clear(lcd_t *lcd, uint16_t rgb565_color)
{
    uint32_t data = ((uint32_t)rgb565_color << 16) | (uint32_t)rgb565_color;

    lcd->lcd_set_area(lcd, 0, 0, lcd->width, lcd->height);
    tft_fill_data(&data, lcd->width * lcd->height / 2);
    return 0;
}

static int lcd_st7789_set_direction(lcd_t *lcd, lcd_dir_t dir)
{
    uint16_t tmp = 0;

    lcd->dir = dir;
    if (dir & DIR_XY_MASK)
    {
        if (lcd->width < lcd->height)
        {
            tmp = lcd->width;
            lcd->width = lcd->height;
            lcd->height = tmp;
        }
    }
    else
    {
        if (lcd->width > lcd->height)
        {
            tmp = lcd->width;
            lcd->width = lcd->height;
            lcd->height = tmp;
        }
    }
    tft_write_command(MEMORY_ACCESS_CTL);
    tft_write_byte((uint8_t *)&dir, 1);
    return 0;
}

static int lcd_st7789_set_area(lcd_t *lcd,
                               uint16_t x, uint16_t y,
                               uint16_t w, uint16_t h)
{
    uint8_t data[4] = {0};

    data[0] = (uint8_t)(x >> 8);
    data[1] = (uint8_t)(x);
    data[2] = (uint8_t)((x + w - 1) >> 8);
    data[3] = (uint8_t)((x + w - 1));
    tft_write_command(HORIZONTAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    // if(lcd->dir & 0x80)
    // {
    //     y += 80;
    // }

    data[0] = (uint8_t)(y >> 8);
    data[1] = (uint8_t)(y);
    data[2] = (uint8_t)((y + h - 1) >> 8);
    data[3] = (uint8_t)((y + h - 1));
    tft_write_command(VERTICAL_ADDRESS_SET);
    tft_write_byte(data, 4);

    tft_write_command(MEMORY_WRITE);
    return 0;
}

static int lcd_st7789_draw_picture(lcd_t *lcd,
                                   uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h,
                                   uint32_t *imamge)
{
    lcd->lcd_set_area(lcd, x, y, w, h);
    tft_write_word(imamge, w * h / 2, 0);
    return 0;
}

int lcd_st7789_init(lcd_t *lcd)
{
    lcd->dir = 0x0;

#if LCD_240240
    lcd->width = 240;
    lcd->height = 240;
#else
    lcd->width = 320;
    lcd->height = 240;
#endif

    lcd->lcd_config = lcd_st7789_config;
    lcd->lcd_clear = lcd_st7789_clear;
    lcd->lcd_set_direction = lcd_st7789_set_direction;
    lcd->lcd_set_area = lcd_st7789_set_area;
    lcd->lcd_draw_picture = lcd_st7789_draw_picture;
    return 0;
}
