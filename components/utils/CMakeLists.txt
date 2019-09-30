
    
list(APPEND ADD_INCLUDE "base64/include"
                        "cJSON/include"
                        "image_op/include"
                        "list/src"
                        #option
                        "jpeg_decode/include"
                        "jpeg_encode/include"
                        "cQueue/src"
                    )

append_srcs_dir(ADD_SRCS    "base64/src")
append_srcs_dir(ADD_SRCS    "cJSON/src")
append_srcs_dir(ADD_SRCS    "image_op/src")
append_srcs_dir(ADD_SRCS    "list/src")

if(CONFIG_ENABLE_JPEG_DECODE)
    append_srcs_dir(ADD_SRCS    "jpeg_decode/src")
endif()

if(CONFIG_ENABLE_JPEG_ENCODE)
    append_srcs_dir(ADD_SRCS    "jpeg_encode/src")
endif()

if(CONFIG_ENABLE_CQUEUE)
    append_srcs_dir(ADD_SRCS    "cQueue/src")
endif()

register_component()

