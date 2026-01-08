cmake_minimum_required(VERSION 4.1)
project(beta_com C)

option(BUILD_TESTS "Construire les tests unitaires" OFF)

add_library(beta_com library.c)
target_include_directories(beta_com PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_TESTS)
    add_executable(unit_tests
            tests/test_main.c
            tests/test_cobs.c
    )
    target_link_libraries(unit_tests beta_com)
endif()