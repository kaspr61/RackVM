option(UNION_DECODING "Use a union instead of bitmasking for decoding instruction operands." ON)

add_executable(${TARGET_VM})

set_property(TARGET ${TARGET_VM} PROPERTY C_STANDARD 99)

target_sources(${TARGET_VM}
    PRIVATE
        vm.c
)

if(UNION_DECODING)
    target_compile_definitions(${TARGET_VM} PRIVATE UNION_DECODING)
endif()

add_subdirectory(decoding-exp)
