if(CONFIG_COMPONENT_DRIVERS_ENABLE)

    list(APPEND ADD_INCLUDE "lcd/include"
                            "sd_card/include"
                            "camera/include"
                            )
    append_srcs_dir(ADD_SRCS "lcd/src")
    append_srcs_dir(ADD_SRCS "lcd/lcd_st7789/src")
    append_srcs_dir(ADD_SRCS "sd_card/src")
    append_srcs_dir(ADD_SRCS "camera/src")


    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

    register_component()
endif()

