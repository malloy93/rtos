cmake_minimum_required(VERSION 3.15.3)

include(${CMAKE_CURRENT_SOURCE_DIR}/arm-none-eabi-gcc.cmake)
project(rrtos_f429_core)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MCU_FAMILY STM32F4xx)
set(MCU_MODEL STM32F429xx)
set(CPU_PARAMETERS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard)

set(MCU_LINKER_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f429i_disc1/linker/STM32F429ZITX_FLASH.ld)

enable_language(C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

message(STATUS "MCU_FAMILY: ${MCU_FAMILY}")

set(CMSIS_HAL_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f429i_disc1/include
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f429i_disc1/include/board
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/${MCU_FAMILY}_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/${MCU_FAMILY}_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/${MCU_FAMILY}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include)

include(cmake/RtosBuildOptions.cmake)

add_subdirectory(kernel)
add_subdirectory(platform/stm32f429i_disc1)
add_subdirectory(apps/demo)

add_executable(rrtos_f429_core
    $<TARGET_OBJECTS:rtos_kernel>
    $<TARGET_OBJECTS:stm32f429_platform>
    $<TARGET_OBJECTS:demo_app>)

target_link_libraries(rrtos_f429_core PRIVATE rtos_build_options)

target_link_options(rrtos_f429_core PRIVATE
    -T${MCU_LINKER_SCRIPT}
    ${CPU_PARAMETERS}
    -Wl,-Map=rrtos_f429_core.map
    --specs=nosys.specs
    -Wl,--start-group
    -lc
    -lm
    -lstdc++
    -lsupc++
    -Wl,--end-group
    -Wl,--print-memory-usage)

add_custom_command(TARGET rrtos_f429_core POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:rrtos_f429_core>
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:rrtos_f429_core> rrtos_f429_core.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:rrtos_f429_core> rrtos_f429_core.bin)
