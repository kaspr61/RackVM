option(UNION_DECODING "Use a union instead of bitmasking for decoding instruction operands." ON)
option(BENCHMARK "Whether to measure the time taken to execute the program (WIN)." OFF)

add_executable(${TARGET_VM})

set_property(TARGET ${TARGET_VM} PROPERTY C_STANDARD 99)

target_sources(${TARGET_VM}
    PRIVATE
        vm.c
        vm_memory.c     vm_memory.h
        opcodes.h
        stack_impl.h
        register_impl.h
        shared_impl.h
)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3")
endif()

if(UNION_DECODING)
    target_compile_definitions(${TARGET_VM} PRIVATE UNION_DECODING)
endif()
if(BENCHMARK)
    target_compile_definitions(${TARGET_VM} PRIVATE BENCHMARK)
endif()

add_subdirectory(decoding-exp)
