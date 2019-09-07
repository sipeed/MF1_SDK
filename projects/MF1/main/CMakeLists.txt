############### Add include ###################
list(APPEND ADD_INCLUDE "audio"
                        "face_lib"
                        "network"
                        "network/http"
                        "network/mqtt"
                        "network/qrcode"
                        "network/wifi"
                        "network/wifi/spi"
                        "network/wifi/utility"
                        "uart_recv"
                        "include"
                        "camera/include"
                        "camera/ov2640/include"
    )
# list(APPEND ADD_PRIVATE_INCLUDE "")
###############################################

############ Add source files #################
append_srcs_dir(ADD_SRCS "audio")
append_srcs_dir(ADD_SRCS "face_lib")

if(CONFIG_WIFI_ENABLE)
FILE(GLOB_RECURSE NETWORK_SRC
                            "network/*.c"
                        )
list(APPEND ADD_SRCS ${NETWORK_SRC})
endif()

append_srcs_dir(ADD_SRCS "src")
append_srcs_dir(ADD_SRCS "uart_recv")

append_srcs_dir(ADD_SRCS "camera/src")
if(CONFIG_CAMERA_OV2640)
    append_srcs_dir(ADD_SRCS "camera/ov2640/src")
endif()

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
