list(APPEND ADD_INCLUDE "mem_macro"
                        #audio
                        "audio/include"
                        "audio/es8374/include"
                        "audio/pt8211/include"
                        #camera
                        "camera/include"
                        "camera/ov2640/include"
                        #lcd
                        "lcd/include"
                        "lcd/lcd_st7789/include"
                        "lcd/lcd_sipeed/include"
                        #rtc
                        "rtc/include"
                        "rtc/gm1302/include"
                        "rtc/bm8563/include"
                        #sd card
                        "sd_card/include"
                        )

if(CONFIG_ENABLE_SPK)
    append_srcs_dir(ADD_SRCS "audio/src")
    
    if(CONFIG_SPK_TYPE_ES8374)
        append_srcs_dir(ADD_SRCS "audio/es8374/src")
    elseif(CONFIG_SPK_TYPE_PT8211)
        append_srcs_dir(ADD_SRCS "audio/pt8211/src")
    endif()
endif()

if(CONFIG_ENABLE_CAMERA)
    append_srcs_dir(ADD_SRCS "camera/src")

    if(CONFIG_CAMERA_OV2640)
        append_srcs_dir(ADD_SRCS "camera/ov2640/src")
    endif()
endif()

if(CONFIG_ENABLE_LCD)
    append_srcs_dir(ADD_SRCS "lcd/src")

    if(CONFIG_LCD_TYPE_ST7789)
        append_srcs_dir(ADD_SRCS "lcd/lcd_st7789/src")
    elseif(CONFIG_LCD_TYPE_SIPEED)
        # append_srcs_dir(ADD_SRCS "lcd/lcd_sipeed/src")
     message(FATAL_ERROR "not support this LCD Driver")
    endif()
endif()

if(CONFIG_ENABLE_RTC)
    append_srcs_dir(ADD_SRCS "rtc/src")

    if(CONFIG_RTC_TYPE_GM1302)
        append_srcs_dir(ADD_SRCS "rtc/gm1302/src")
    elseif(CONFIG_RTC_TYPE_BM8563)
        append_srcs_dir(ADD_SRCS "rtc/bm8563/src")
    endif()
endif()

if(CONFIG_ENABLE_SD_CARD)
    append_srcs_dir(ADD_SRCS "sd_card/src")
endif()

list(APPEND ADD_REQUIREMENTS kendryte_sdk)

register_component()
