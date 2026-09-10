add_library(rtos_kernel OBJECT
    src/Core.cpp
    src/Logger.cpp
    src/MemoryPool.cpp
    src/SysTasks.cpp
    src/Thread.cpp
    src/Utils.cpp
    src/osKernelAsm.s)

target_include_directories(rtos_kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/platform/stm32f429i_disc1/include)

target_link_libraries(rtos_kernel PRIVATE rtos_build_options)
