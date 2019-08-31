
    
list(APPEND ADD_INCLUDE "base64/include"
                        "cJSON/include"
                        "jpeg_decode/include"
                        "jpeg_encode/include"
                        "image_op/include"
                        "list/src"
                    )

append_srcs_dir(ADD_SRCS    "base64/src")
append_srcs_dir(ADD_SRCS    "cJSON/src")
append_srcs_dir(ADD_SRCS    "jpeg_decode/src")
append_srcs_dir(ADD_SRCS    "jpeg_encode/src")
append_srcs_dir(ADD_SRCS    "image_op/src")
append_srcs_dir(ADD_SRCS    "list/src")

register_component()

