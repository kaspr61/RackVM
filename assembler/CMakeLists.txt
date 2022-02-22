add_executable(${TARGET_ASM})

set_property(TARGET ${TARGET_ASM} PROPERTY CXX_STANDARD 11)

target_sources(${TARGET_ASM}
    PRIVATE
        assembler.cpp   assembler.hpp
        label.cpp       label.hpp
        encoder.cpp     encoder.hpp
        common.hpp
)

if(MINGW)
    message("Detecting MinGW. Standard libraries set to link statically.")
    target_link_libraries(${TARGET_ASM} -static-libgcc -static-libstdc++)
endif()