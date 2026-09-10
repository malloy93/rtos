add_library(demo_app OBJECT main.cpp)

target_include_directories(demo_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/kernel/include
    ${PROJECT_SOURCE_DIR}/platform/stm32f429i_disc1/include)

target_link_libraries(demo_app PRIVATE rtos_build_options)
