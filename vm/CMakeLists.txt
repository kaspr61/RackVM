set(C_STANDARD 99)
set(C_STANDARD_REQUIRED true)

add_executable(${TARGET_VM})

target_sources(${TARGET_VM}
    PRIVATE
        vm.c
)
