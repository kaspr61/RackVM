cmake_minimum_required(VERSION 3.13.0)

project(Assembler)

set(TARGET_ASM asm)
set(TARGET_VM  vm)

add_subdirectory(assembler)
add_subdirectory(vm)
