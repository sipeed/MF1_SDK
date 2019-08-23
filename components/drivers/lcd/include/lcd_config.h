#ifndef __LCD_CONFIG_H
#define __LCD_CONFIG_H

#include "global_config.h"



#if CONFIG_LCD_TYPE_ST7789
#define LCD_240240                           (1)
#define LCD_320240                           (0)
#define LCD_ROTATE                           (1)

#define LCD_W                                (240)
#define LCD_H                                (240)
#elif CONFIG_LCD_TYPE_SSD1963
#define LCD_240240                            ()
#define LCD_320240                            ()
#define LCD_ROTATE                            ()

#define LCD_W                                 ()
#define LCD_H                                 ()
#elif CONFIG_LCD_TYPE_SIPEED
#define LCD_W                                (402)
#define LCD_H                                (240)

#define DAT_W                                (240)
#define DAT_H                                (240)
#endif

#define LCD_BL_PIN                          (CONFIG_LCD_PIN_BL)
#define LCD_RST_PIN                         (CONFIG_LCD_PIN_RST)
#define LCD_DCX_PIN                         (CONFIG_LCD_PIN_DCX)
#define LCD_WRX_PIN                         (CONFIG_LCD_PIN_WRX)
#define LCD_SCK_PIN                         (CONFIG_LCD_PIN_SCK)
#define LCD_DCX_HS_NUM                      (CONFIG_LCD_GPIOHS_DCX)
#define LCD_RST_HS_NUM                      (CONFIG_LCD_GPIOHS_RST)

#endif


