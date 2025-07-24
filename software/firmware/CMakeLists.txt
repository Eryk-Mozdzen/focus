cmake_minimum_required(VERSION 3.16)

project(test)

add_executable(${CMAKE_PROJECT_NAME}
    "../focus/current_loop.c"
    "../focus/encoder.c"
    "../focus/inverter.c"
    "../focus/memory_controller.c"
    "../focus/memory_reg.c"
    "../focus/memory.c"
    "../focus/pid.c"

    "src/main.c"
    "src/drv_inverter.c"
    "src/drv_as5600.c"
    "src/drv_ram.c"
    "src/drv_flash.c"
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
	"src"
    "../focus/include"
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -O3
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    m
)
