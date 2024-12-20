Cmake_minimum_required(VERSION 3.15.3)
include(./arm-none-eabi-gcc.cmake)
project(rrtos_f429_core)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)


set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/RTOS)

set(MCU_FAMILY STM32F4xx)
set(MCU_MODEL STM32F429xx)
set(CPU_PARAMETERS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard)

set(STARTUP_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/Core/Startup/startup_stm32f429zitx.s)
set(MCU_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F429ZITX_FLASH.ld)
# ##############################################################################
set(EXECUTABLE ${CMAKE_PROJECT_NAME})
enable_language(C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Headers
set(CMSIS_HAL_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/${MCU_FAMILY}_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/${MCU_FAMILY}_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/${MCU_FAMILY}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include)
set(PROJECT_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/main
    ${CMAKE_CURRENT_SOURCE_DIR}/RTOS
    )

# Sources
file(GLOB_RECURSE CMSIS_HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RTOS/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/${MCU_FAMILY}_HAL_Driver/Src/*.c)
file(GLOB_RECURSE PROJECT_SOURCES FOLLOW_SYMLINKS
    ${PROJECT_DIR}/*.cpp
    ${PROJECT_DIR}/*.c)

set(ASSEMBLY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/RTOS/osKernelAsm.s
)
# Executable files
add_executable(${EXECUTABLE}
    ${CMSIS_HAL_SOURCES} 
    ${PROJECT_SOURCES}
    ${ASSEMBLY_SOURCES}
    ${STARTUP_SCRIPT})

# Embedded macros(defines)
target_compile_definitions(${EXECUTABLE} PRIVATE
    ${MCU_MODEL}
    USE_HAL_DRIVER)

# Add header directories (AFTER add_executable !!)
target_include_directories(${EXECUTABLE} PRIVATE
    ${CMSIS_HAL_INCLUDES}
    ${PROJECT_INCLUDE_DIRECTORIES})

# Compiler and linker options
target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    $<$<COMPILE_LANGUAGE:CXX>:
        -Wno-missing-field-initializers
        -Wno-volatile
        -Wno-old-style-cast
        -Wno-useless-cast
        -Wsuggest-override>
    $<$<CONFIG:Debug>:
        -Og    # Optymalizacja dla debugowania
        -g3    # Generowanie szczegółowych symboli debugowania
        -ggdb  # Generowanie dodatkowych informacji do GDB
        -fno-omit-frame-pointer  # Zachowanie wskaźnika ramki dla debugowania
        -fno-inline  # Wyłączenie inliningu, aby ułatwić debugowanie
    >)

target_link_options(${EXECUTABLE} PRIVATE
    -T${MCU_LINKER_SCRIPT}
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -Wl,--start-group
    -lc
    -lm
    -lstdc++
    -lsupc++
    -Wl,--end-group
    -Wl,--print-memory-usage)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}>
    ${EXECUTABLE}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}>
    ${EXECUTABLE}.bin)