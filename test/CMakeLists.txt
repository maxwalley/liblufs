include(FetchContent)

#Download the test files
FetchContent_Declare(
  ebuLoudnessTestSet
  URL      https://github.com/maxwalley/EBULoudnessTestSetV5/releases/download/EBU_Loudness_Test_Set_v5/ebu-loudness-test-setv05.zip
  URL_HASH SHA256=9cc500b4df83f7c21855c74dce795ef5209a752bf884253ae57d0ce512efb062
)

FetchContent_MakeAvailable(ebuLoudnessTestSet)

#Add google test and set it not to build googlemock
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(googletest)

add_executable(tests 
    EBU3341_Test_Set.cpp
)

set_target_properties(tests PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_include_directories(tests PRIVATE 
    "dr_libs" 
    "../include"
    ${CMAKE_BINARY_DIR}/libsamplerate/include
)

target_link_libraries(
    tests
    GTest::gtest_main
    lufs
)

include(GoogleTest)
gtest_discover_tests(tests)

install(TARGETS tests)

install(DIRECTORY ${ebuloudnesstestset_SOURCE_DIR}
    DESTINATION share)