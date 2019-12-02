if(CONFIG_COMPONENT_DRIVERS_ENABLE)

list(APPEND ADD_INCLUDE "sd_card/include"
                        #mem
                        "mem_macro"
                        #lcd
                        "lcd/include"
                        "lcd/lcd_st7789/include"
                        #camera
                        "camera/include"
                        "camera/ov2640/include"
                        )

    if(CONFIG_SD_CARD_ENABLE)
        append_srcs_dir(ADD_SRCS "sd_card/src")
    endif()

    append_srcs_dir(ADD_SRCS "lcd/src")

    if(CONFIG_LCD_TYPE_ST7789)
        append_srcs_dir(ADD_SRCS    "lcd/lcd_st7789/src")
    elseif(CONFIG_LCD_TYPE_SIPEED)
        if(CONFIG_LCD_SIPEED_V2)
            append_srcs_dir(ADD_SRCS    "lcd/lcd_sipeed_v2/src")
            list(APPEND ADD_INCLUDE     "lcd/lcd_sipeed_v2/include")
        else()
            append_srcs_dir(ADD_SRCS    "lcd/lcd_sipeed_v1/src")
            list(APPEND ADD_INCLUDE     "lcd/lcd_sipeed_v1/include")
        endif()
        
        if(CONFIG_TYPE_800_480_57_INCH)
            list(APPEND ADD_DEFINITIONS -DSIPEED_LCD_W=322
                                        -DSIPEED_LCD_H=240
                                        -DSIPEED_LCD_BANNER_W=160
                                        -DSIPEED_LCD_BANNER_H=480
                                        )
        elseif(CONFIG_TYPE_480_272_4_3_INCH)
            list(APPEND ADD_DEFINITIONS -DSIPEED_LCD_W=322
                                        -DSIPEED_LCD_H=272
                                        -DSIPEED_LCD_BANNER_W=160
                                        -DSIPEED_LCD_BANNER_H=272
                                        )
        elseif(CONFIG_TYPE_1024_600)
    
        endif()
    endif()

    append_srcs_dir(ADD_SRCS "camera/src")
    
    if(CONFIG_CAMERA_OV2640)
        append_srcs_dir(ADD_SRCS "camera/ov2640/src")
    endif()

    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

    register_component()

endif()

