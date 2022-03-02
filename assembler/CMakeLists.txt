add_executable(${TARGET_ASM})

set_property(TARGET ${TARGET_ASM} PROPERTY CXX_STANDARD 14)

target_sources(${TARGET_ASM}
    PRIVATE
        assembler.cpp   assembler.hpp
        label.cpp       label.hpp
        encoder.cpp     encoder.hpp
        common.hpp
)
