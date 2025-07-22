cmake_minimum_required(VERSION 3.16)

project(firmware)

add_executable(${CMAKE_PROJECT_NAME}
	"src/main.c"
    "src/system/if_inverter.c"
    "src/system/if_encoder.c"
    "src/control/pid.c"
    "src/control/current_loop.c"
    "src/drivers/drv_inverter.c"
    "src/drivers/drv_encoder_as5600.c"
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
	"src"
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
