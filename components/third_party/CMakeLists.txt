

if(CONFIG_OOFATFS_ENABLE)
    list(APPEND ADD_INCLUDE "oofatfs/include")
    
    append_srcs_dir(ADD_SRCS    "oofatfs/src")
    append_srcs_dir(ADD_SRCS    "oofatfs/option")

    list(APPEND ADD_REQUIREMENTS kendryte_sdk)
    
    register_component()
endif()



