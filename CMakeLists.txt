cmake_minimum_required(VERSION 3.18)
project(liblufs LANGUAGES CXX)

option(BUILD_TESTS "Build tests" ON)

add_library(lufs SHARED
    include/Channel.h
    include/Filter.h
    include/LoudnessMeter.h
    src/LoudnessMeter.cpp
)

target_include_directories(lufs PUBLIC include)

set_target_properties(lufs PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    PUBLIC_HEADER "include/LoudnessMeter.h;include/Channel.h;include/Filter.h"
)

install(TARGETS lufs
    PUBLIC_HEADER DESTINATION include)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()