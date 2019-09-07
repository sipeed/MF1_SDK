if(CONFIG_COMPONENT_DRIVERS_ENABLE)

    list(APPEND ADD_INCLUDE "lcd/include"
                            "sd_card/include"
                            "lcd/lcd_st7789/include"
                            "lcd/lcd_sipeed/include"
                            )
    append_srcs_dir(ADD_SRCS "lcd/src")
    if(CONFIG_LCD_TYPE_ST7789)
        append_srcs_dir(ADD_SRCS "lcd/lcd_st7789/src")
    elseif(CONFIG_LCD_TYPE_SIPEED)
        append_srcs_dir(ADD_SRCS "lcd/lcd_sipeed/src")
    endif()

    if(CONFIG_SD_CARD_ENABLE)
        append_srcs_dir(ADD_SRCS "sd_card/src")
    endif()

    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

    register_component()
endif()

