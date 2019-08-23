if(CONFIG_COMPONENT_DRIVERS_ENABLE)

    list(APPEND ADD_INCLUDE "lcd/include"
                            "sd_card/include"
                            "camera/include"
                            )
    append_srcs_dir(ADD_SRCS "lcd/src")
    if(CONFIG_LCD_TYPE_ST7789)
        append_srcs_dir(ADD_SRCS "lcd/lcd_st7789/src")
    elseif(CONFIG_LCD_TYPE_SSD1963)
        append_srcs_dir(ADD_SRCS "lcd/lcd_ssd1963/src")
    elseif(CONFIG_LCD_TYPE_SIPEED)
        append_srcs_dir(ADD_SRCS "lcd/lcd_sipeed/src")
    endif()
    append_srcs_dir(ADD_SRCS "sd_card/src")
    append_srcs_dir(ADD_SRCS "camera/src")


    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

    register_component()
endif()

