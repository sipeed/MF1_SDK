
    
list(APPEND ADD_INCLUDE "base64/include"
                        "cJSON/include"
                        "jpeg_decode/include"
                        "jpeg_encode/include"
                    )


append_srcs_dir(ADD_SRCS    "base64/src")
append_srcs_dir(ADD_SRCS    "cJSON/src")
append_srcs_dir(ADD_SRCS    "jpeg_decode/src")
append_srcs_dir(ADD_SRCS    "jpeg_encode/src")


register_component()

