cmake_minimum_required(VERSION 3.25)
project(CHIP_8_Emulator C)

set(CMAKE_C_STANDARD 23)

add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_executable(CHIP_8_Emulator
        src/main.c
        src/chip8.c
        include/chip8.h
)

target_link_libraries(CHIP_8_Emulator PRIVATE SDL3::SDL3)

if(WIN32)
    add_custom_command(
            TARGET CHIP_8_Emulator POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:CHIP_8_Emulator>
            VERBATIM
    )
endif()