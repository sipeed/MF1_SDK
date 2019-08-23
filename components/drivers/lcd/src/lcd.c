#include "lcd.h"


#if CONFIG_LCD_TYPE_SSD1963
uint8_t display_image_rgb888[CONFIG_CAMERA_RESOLUTION_WIDTH * CONFIG_CAMERA_RESOLUTION_HEIGHT * 3] __attribute__((aligned(64)));
#endif

#if CONFIG_LCD_TYPE_SIPEED
uint8_t lcd_image[LCD_W * LCD_H * 2] __attribute__((aligned(64)));
#endif

#if CONFIG_LCD_TYPE_ST7789
#if LCD_240240
uint8_t lcd_image[LCD_W * LCD_H * 2] __attribute__((aligned(64)));
#else
const uint8_t *lcd_image = display_image;
#endif
#endif

