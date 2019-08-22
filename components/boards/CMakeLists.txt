

if(CONFIG_COMPONENT_BOARDS_ENABLE)
    list(APPEND ADD_INCLUDE "include")
    aux_source_directory("src" BOARDS_SRCS)
    list(APPEND ADD_SRCS ${BOARDS_SRCS})
    list(APPEND ADD_REQUIREMENTS kendryte_sdk)

    register_component()
endif()


