list(APPEND ADD_INCLUDE "sd_card/include"
                        #mem
                        "mem_macro"
                        #lcd
                        "lcd/include"
                        "lcd/lcd_st7789/include"
                        "lcd/lcd_sipeed/include"
                        #camera
                        "camera/include"
                        "camera/ov2640/include"
                        #rtc
                        "rtc/include"
                        #gm1302
                        "rtc/gm1302/include"
                        #bm8563
                        "rtc/bm8563/include"
                        )

if(CONFIG_ENABLE_SD_CARD)
    append_srcs_dir(ADD_SRCS "sd_card/src")
endif()

append_srcs_dir(ADD_SRCS "lcd/src")

if(CONFIG_LCD_TYPE_ST7789)
    append_srcs_dir(ADD_SRCS "lcd/lcd_st7789/src")
elseif(CONFIG_LCD_TYPE_SIPEED)
    # append_srcs_dir(ADD_SRCS "lcd/lcd_sipeed/src")
    message(FATAL_ERROR "not support this LCD Driver")
endif()

append_srcs_dir(ADD_SRCS "camera/src")

if(CONFIG_CAMERA_OV2640)
    append_srcs_dir(ADD_SRCS "camera/ov2640/src")
endif()

if(CONFIG_ENABLE_RTC)
    append_srcs_dir(ADD_SRCS "rtc/src")

    if(CONFIG_RTC_TYPE_GM1302)
        append_srcs_dir(ADD_SRCS "rtc/gm1302/src")
    elseif(CONFIG_RTC_TYPE_BM8563)
        append_srcs_dir(ADD_SRCS "rtc/bm8563/src")
    endif()
endif()

list(APPEND ADD_REQUIREMENTS kendryte_sdk)

register_component()
