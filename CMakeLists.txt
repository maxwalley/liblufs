cmake_minimum_required(VERSION 3.29)
project(liblufs LANGUAGES CXX)

include(ExternalProject)

ExternalProject_Add(samplerate 
    GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/libsamplerate -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DLIBSAMPLERATE_EXAMPLES=OFF
    EXCLUDE_FROM_ALL
)

option(BUILD_TESTS "Build tests" ON)

add_library(lufs SHARED
    include/FixedTermLoudnessMeter.h
    include/IntegratedLoudnessMeter.h
    include/TruePeakMeter.h
    src/FixedTermLoudnessMeter.cpp
    src/IntegratedLoudnessMeter.cpp
    src/ChannelProcessor.cpp
    src/TruePeakMeter.cpp
)

target_include_directories(lufs PRIVATE 
    include
    ${CMAKE_BINARY_DIR}/libsamplerate/include
)

if(WIN32)
    target_link_libraries(lufs PRIVATE
    	${CMAKE_BINARY_DIR}/libsamplerate/lib/samplerate.lib
    )
elseif(UNIX)
    target_link_libraries(lufs PRIVATE
    	${CMAKE_BINARY_DIR}/libsamplerate/lib/libsamplerate.a
    )
endif()

set_target_properties(lufs PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

install(TARGETS lufs
    PUBLIC_HEADER DESTINATION include)