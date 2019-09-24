if(CONFIG_COMPONENT_DRIVERS_ENABLE)

list(APPEND ADD_INCLUDE "sd_card/include"
                        )

    if(CONFIG_SD_CARD_ENABLE)
        append_srcs_dir(ADD_SRCS "sd_card/src")

        list(APPEND ADD_REQUIREMENTS kendryte_sdk)

        register_component()
    endif()
endif()

