add_executable(${TARGET_VM})

set_property(TARGET ${TARGET_VM} PROPERTY C_STANDARD 99)

target_sources(${TARGET_VM}
    PRIVATE
        vm.c
)

add_subdirectory(decoding-exp)
