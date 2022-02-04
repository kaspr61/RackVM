set(CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED true)

add_executable(${TARGET_ASM})

target_sources(${TARGET_ASM}
    PRIVATE
        assembler.cpp   assembler.hpp
        label.cpp       label.hpp
        common.hpp
)

if(MINGW)
    message("Detecting MinGW. Standard libraries set to link statically.")
    target_link_libraries(${TARGET_ASM} -static-libgcc -static-libstdc++)
endif()