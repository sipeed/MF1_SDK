############### Add include ###################
list(APPEND ADD_INCLUDE "audio"
                        "face_lib"
                        "uart_recv"
                        "include"
                        #esp8285
                        "network/esp8285"
                        "network/esp8285/http"
                        "network/esp8285/mqtt"
                        "network/esp8285/qrcode"
                        "network/esp8285/wifi"
                        "network/esp8285/wifi/spi"
                        "network/esp8285/wifi/utility"
                        #w5500
                        "network/w5500"
                        "network/w5500/httpc"
                        "network/w5500/httpd/src"
                        "network/w5500/httpd/web_pages"
                        "network/w5500/mqtt"
                        "network/w5500/spi"
                        "network/w5500/src"
                        "network/w5500/src/utility"
    )
# list(APPEND ADD_PRIVATE_INCLUDE "")
###############################################

############ Add source files #################
append_srcs_dir(ADD_SRCS "audio")
append_srcs_dir(ADD_SRCS "face_lib")

if(CONFIG_NET_ENABLE)
    if(CONFIG_NET_ESP8285)
        message("-- Use ESP8285")
        FILE(GLOB_RECURSE NETWORK_SRC "network/esp8285/*.c")
    elseif(CONFIG_NET_W5500)
        message("-- Use W5500")
        FILE(GLOB_RECURSE NETWORK_SRC "network/w5500/*.c")
    endif()
    list(APPEND ADD_SRCS ${NETWORK_SRC})
endif()

append_srcs_dir(ADD_SRCS "src")
append_srcs_dir(ADD_SRCS "uart_recv")

# aux_source_directory(src ADD_SRCS)
# list(REMOVE_ITEM COMPONENT_SRCS "src/test2.c")
###############################################

###### Add required/dependent components ######
list(APPEND ADD_REQUIREMENTS kendryte_sdk drivers utils)
###############################################

############ Add static libs ##################

# add_custom_command( PRE_BUILD
if(CONFIG_LCD_VERTICAL AND 
    (CONFIG_CAMERA_OV2640 OR CONFIG_CAMERA_GC0328_SINGLE))
        message("-- Use lib_face_single_ver.a")
        list(APPEND ADD_STATIC_LIB "face_lib/lib/lib_face_single_ver.a")
elseif(CONFIG_LCD_VERTICAL AND CONFIG_CAMERA_GC0328_DUAL)
        message("-- Use lib_face_dual_ver.a")
        list(APPEND ADD_STATIC_LIB "face_lib/lib/lib_face_dual_ver.a")
elseif(NOT CONFIG_LCD_VERTICAL AND 
    (CONFIG_CAMERA_OV2640 OR CONFIG_CAMERA_GC0328_SINGLE))
        message("-- Use lib_face_single_hor.a")
        list(APPEND ADD_STATIC_LIB "face_lib/lib/lib_face_single_hor.a")
elseif(NOT CONFIG_LCD_VERTICAL AND CONFIG_CAMERA_GC0328_DUAL)
        message("-- Use lib_face_dual_hor.a")
        list(APPEND ADD_STATIC_LIB "face_lib/lib/lib_face_dual_hor.a")
endif()

###############################################


register_component()
