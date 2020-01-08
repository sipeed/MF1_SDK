
    
list(APPEND ADD_INCLUDE "base64/include"
                        "cJSON/include"
                        "image_op/include"
                        "list/src"
                        #option
                        "jpeg_decode/include"
                        "jpeg_encode/include"
                        "cQueue/src"
                        "qrcode/include"
                        "qrcode/quirc/include"
                        "qrcode/zbar/include"
                        #yuv_tab
                        "yuv_tab/include"
                    )

append_srcs_dir(ADD_SRCS    "base64/src")
append_srcs_dir(ADD_SRCS    "cJSON/src")
append_srcs_dir(ADD_SRCS    "image_op/src")
append_srcs_dir(ADD_SRCS    "list/src")
append_srcs_dir(ADD_SRCS    "yuv_tab/src")

if(CONFIG_ENABLE_JPEG_DECODE)
    append_srcs_dir(ADD_SRCS    "jpeg_decode/src")
endif()

if(CONFIG_ENABLE_JPEG_ENCODE)
    append_srcs_dir(ADD_SRCS    "jpeg_encode/src")
endif()

if(CONFIG_ENABLE_CQUEUE)
    append_srcs_dir(ADD_SRCS    "cQueue/src")
endif()

if(CONFIG_ENABLE_QRCODE)
    append_srcs_dir(ADD_SRCS    "qrcode/src")
    if(CONFIG_QRCODE_TYPE_QUIRC)
        append_srcs_dir(ADD_SRCS    "qrcode/quirc/src")
    elseif(CONFIG_QRCODE_TYPE_ZBAR)
        append_srcs_dir(ADD_SRCS    "qrcode/zbar/src")
    endif()
endif()

register_component()

