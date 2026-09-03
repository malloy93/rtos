cmake_minimum_required(VERSION 3.15)
project(rtos_tests CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_MemoryPool
    test_MemoryPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../RTOS/MemoryPool.cpp
)

target_include_directories(test_MemoryPool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../RTOS
    # stubs/ udostępnia zamienniki Core/Inc/usart.h i stm32f4xx.h
    # które są ciągnięte przez Logger.hpp; nie musi być przed RTOS/
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

# Force-include stub Logger przed RTOS/Logger.hpp — dzięki temu LOG_* makra
# trafiają na stdout. #ifndef guardy w Logger.hpp nie pozwalają go nadpisać.
target_compile_options(test_MemoryPool PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/stubs/Logger.hpp
)

target_link_libraries(test_MemoryPool PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_MemoryPool)

add_executable(test_Scheduler
    test_Scheduler.cpp
)

target_include_directories(test_Scheduler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../RTOS
)

target_compile_options(test_Scheduler PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/stubs/Logger.hpp
)

target_link_libraries(test_Scheduler PRIVATE GTest::gtest_main)

gtest_discover_tests(test_Scheduler)

add_executable(test_ModernScheduler
    test_ModernScheduler.cpp
)

target_include_directories(test_ModernScheduler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../RTOS
)

target_compile_options(test_ModernScheduler PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/stubs/Logger.hpp
)

target_link_libraries(test_ModernScheduler PRIVATE GTest::gtest_main)

gtest_discover_tests(test_ModernScheduler)

add_executable(test_Core
    test_Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../RTOS/Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../RTOS/Thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../RTOS/MemoryPool.cpp
)

target_include_directories(test_Core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../RTOS
)

target_compile_options(test_Core PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/stubs/Logger.hpp
)

target_compile_definitions(test_Core PRIVATE
    RTOS_HOST_TEST
)

target_link_libraries(test_Core PRIVATE GTest::gtest_main)

gtest_discover_tests(test_Core)
