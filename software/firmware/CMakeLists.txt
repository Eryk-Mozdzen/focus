cmake_minimum_required(VERSION 3.16)

project(test)

add_executable(${CMAKE_PROJECT_NAME}
    "../focus/encoder.c"
    "../focus/inverter.c"
    "../focus/memory.c"
    "../focus/pid.c"

    "src/main.c"
    "src/driver_allocator.c"
    "src/driver_inverter.c"
    "src/driver_as5600.c"
    "src/driver_ram.c"
    "src/driver_flash.c"
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
	"src"
    "../focus"
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -O3
    -std=c99
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    m
)
