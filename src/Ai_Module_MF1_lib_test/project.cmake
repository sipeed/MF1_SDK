message("")
message("project.mk")

target_link_libraries(${PROJECT_NAME} 
        -Wl,--start-group
        gcc m c
        -Wl,--whole-archive
        ${SDK_ROOT}/src/${PROJ}/face_lib/lib_face.a
        -Wl,--no-whole-archive
        -Wl,--end-group
        )

message("")