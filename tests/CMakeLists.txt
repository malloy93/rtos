cmake_minimum_required(VERSION 3.15)
project(rtos_tests CXX)

enable_testing()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

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

set(RTOS_TEST_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/include)
set(RTOS_TEST_LOGGER_STUB ${CMAKE_CURRENT_SOURCE_DIR}/stubs/Logger.hpp)

add_executable(test_MemoryPool
    unit/test_MemoryPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/src/MemoryPool.cpp)
target_include_directories(test_MemoryPool PRIVATE ${RTOS_TEST_INCLUDE_DIRECTORIES})
target_compile_options(test_MemoryPool PRIVATE -include ${RTOS_TEST_LOGGER_STUB})
target_link_libraries(test_MemoryPool PRIVATE GTest::gtest_main)

add_executable(test_Scheduler unit/test_Scheduler.cpp)
target_include_directories(test_Scheduler PRIVATE ${RTOS_TEST_INCLUDE_DIRECTORIES})
target_compile_options(test_Scheduler PRIVATE -include ${RTOS_TEST_LOGGER_STUB})
target_link_libraries(test_Scheduler PRIVATE GTest::gtest_main)

add_executable(test_ModernScheduler unit/test_ModernScheduler.cpp)
target_include_directories(test_ModernScheduler PRIVATE ${RTOS_TEST_INCLUDE_DIRECTORIES})
target_compile_options(test_ModernScheduler PRIVATE -include ${RTOS_TEST_LOGGER_STUB})
target_link_libraries(test_ModernScheduler PRIVATE GTest::gtest_main)

add_executable(test_Core
    unit/test_Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/src/Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/src/Thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/src/MemoryPool.cpp)
target_include_directories(test_Core PRIVATE ${RTOS_TEST_INCLUDE_DIRECTORIES})
target_compile_options(test_Core PRIVATE -include ${RTOS_TEST_LOGGER_STUB})
target_compile_definitions(test_Core PRIVATE RTOS_HOST_TEST)
target_link_libraries(test_Core PRIVATE GTest::gtest_main)

add_executable(test_UartProtocol
    unit/test_UartProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/src/Logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/src/Telemetry/UartProtocol.cpp)
target_include_directories(test_UartProtocol PRIVATE ${RTOS_TEST_INCLUDE_DIRECTORIES})
target_link_libraries(test_UartProtocol PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_MemoryPool)
gtest_discover_tests(test_Scheduler)
gtest_discover_tests(test_ModernScheduler)
gtest_discover_tests(test_Core)
gtest_discover_tests(test_UartProtocol)
add_test(
    NAME uart_receiver
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_uart_receiver.py)
