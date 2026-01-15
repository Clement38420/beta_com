cmake_minimum_required(VERSION 3.22)

project(beta_com C)

option(BUILD_TESTS "Build unit tests" OFF)

add_library(beta_com beta_com.c)
set_property(TARGET beta_com PROPERTY C_STANDARD 99)

target_include_directories(beta_com PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_TESTS)
    add_executable(unit_tests
            tests/test_main.c
            tests/test_cobs.c
            tests/test_crc16.c
            tests/test_msg.c
    )
    target_link_libraries(unit_tests beta_com)
endif()