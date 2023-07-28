cmake_minimum_required(VERSION 3.18)
project(liblufs LANGUAGES CXX)

option(BUILD_TESTS "Build tests" ON)

add_library(lufs SHARED
    include/Channel.h
)

target_include_directories(lufs PUBLIC include)

set_target_properties(lufs PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()