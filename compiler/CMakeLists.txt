find_package(BISON)
if (BISON_FOUND)
    bison_target(RackParser parser.yy 
        ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
        COMPILE_FLAGS "-d"
    )
endif ()

find_package(FLEX)
if (FLEX_FOUND)
    flex_target(RackLexer lexer.l
        ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
        COMPILE_FLAGS "-c++"
    )
endif ()

if (FLEX_FOUND AND BISON_FOUND)
    add_flex_bison_dependency(RackLexer RackParser)
endif ()

add_executable(${TARGET_COMPILER})

set_property(TARGET ${TARGET_COMPILER} PROPERTY CXX_STANDARD 14)

target_include_directories(${TARGET_COMPILER} PRIVATE 
    include
    ${CMAKE_CURRENT_BINARY_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR} # Needed for bison/flex files to include non-generated source files.
)

target_sources(${TARGET_COMPILER}
    PRIVATE
        compiler.cpp        compiler.hpp
        code_generator.cpp  code_generator.hpp
        lexer.hpp
        types.hpp
        include/cli_args.hpp
        ${BISON_RackParser_OUTPUTS}
        ${FLEX_RackLexer_OUTPUTS}
)
