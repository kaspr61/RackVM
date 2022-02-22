add_executable(${TARGET_DECODING})

set_property(TARGET ${TARGET_DECODING} PROPERTY C_STANDARD 99)

target_sources(${TARGET_DECODING}
    PRIVATE
        decoding_experiment.c
)

if (CMAKE_BUILD_TYPE STREQUAL Release)
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()
