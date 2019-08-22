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
    )
# list(APPEND ADD_PRIVATE_INCLUDE "")
###############################################

############ Add source files #################
append_srcs_dir(ADD_SRCS "audio")
append_srcs_dir(ADD_SRCS "face_lib")
FILE(GLOB_RECURSE NETWORK_SRC
                            "network/*.c"
                        )
list(APPEND ADD_SRCS ${NETWORK_SRC})
append_srcs_dir(ADD_SRCS "src")
append_srcs_dir(ADD_SRCS "uart_recv")
# aux_source_directory(src ADD_SRCS)
# list(REMOVE_ITEM COMPONENT_SRCS "src/test2.c")
###############################################

###### Add required/dependent components ######
list(APPEND ADD_REQUIREMENTS kendryte_sdk drivers utils)
###############################################

############ Add static libs ##################
list(APPEND ADD_STATIC_LIB "face_lib/lib_face.a")
###############################################


register_component()
