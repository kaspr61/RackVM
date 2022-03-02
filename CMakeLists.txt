cmake_minimum_required(VERSION 3.13.0)

project(Assembler)

set(TARGET_ASM asm)
set(TARGET_COMPILER compiler)
set(TARGET_VM  vm)
set(TARGET_DECODING decoding-exp)

add_subdirectory(assembler)
add_subdirectory(vm)
add_subdirectory(compiler)

if(MINGW)
    message("Detecting MinGW. C++ standard libraries set to link statically.")
    target_link_libraries(${TARGET_ASM} -static-libgcc -static-libstdc++)
    target_link_libraries(${TARGET_COMPILER} -static-libgcc -static-libstdc++)
endif()
