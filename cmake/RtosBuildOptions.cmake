add_library(rtos_build_options INTERFACE)

target_compile_definitions(rtos_build_options INTERFACE
    ${MCU_MODEL}
    USE_HAL_DRIVER)

target_include_directories(rtos_build_options INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMSIS_HAL_INCLUDES})

target_compile_options(rtos_build_options INTERFACE
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
        -Og
        -g3
        -ggdb
        -fno-omit-frame-pointer
        -fno-inline>)
