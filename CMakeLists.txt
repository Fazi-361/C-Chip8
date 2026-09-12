cmake_minimum_required(VERSION 3.25)
project(CHIP_8_Emulator C)

set(CMAKE_C_STANDARD 23)

add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_compile_options(-Wall -Wextra -Wimplicit-fallthrough)

add_executable(C-Chip8
        src/main.c
        src/chip8.c
        include/chip8.h
        src/mainutils.c
        include/mainutils.h
)

target_link_libraries(C-Chip8 PRIVATE SDL3::SDL3)

if(WIN32)
    add_custom_command(
            TARGET C-Chip8 POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
            $<TARGET_FILE:SDL3::SDL3-shared>
            $<TARGET_FILE_DIR:C-Chip8>
            VERBATIM
    )
endif()