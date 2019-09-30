if(CONFIG_COMPONENT_DRIVERS_ENABLE)

    list(APPEND ADD_INCLUDE "sd_card/include"
                            #lcd
                            "lcd/include"
                            "lcd/lcd_st7789/include"
                            "lcd/lcd_sipeed/include"
                            #camera
                            "camera/include"
                            "camera/ov2640/include"
                            )

    if(CONFIG_SD_CARD_ENABLE)
        append_srcs_dir(ADD_SRCS "sd_card/src")
    endif()

    append_srcs_dir(ADD_SRCS "lcd/src")
    if(CONFIG_LCD_TYPE_ST7789)
        append_srcs_dir(ADD_SRCS "lcd/lcd_st7789/src")
    elseif(CONFIG_LCD_TYPE_SIPEED)
        append_srcs_dir(ADD_SRCS "lcd/lcd_sipeed/src")
    endif()

    append_srcs_dir(ADD_SRCS "camera/src")
    if(CONFIG_CAMERA_OV2640)
        append_srcs_dir(ADD_SRCS "camera/ov2640/src")
    endif()

    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

    register_component()
endif()

