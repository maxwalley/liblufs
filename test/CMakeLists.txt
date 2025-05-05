#Download the test files
set(TEST_CONTENT_URL https://tech.ebu.ch/files/live/sites/tech/files/shared/testmaterial/ebu-loudness-test-setv05.zip)
set(TEST_CONTENT_ZIP_OUTPUT "${CMAKE_BINARY_DIR}/test_content.zip")
set(TEST_CONTENT_OUTPUT "${CMAKE_BINARY_DIR}/test_content")

execute_process(
    COMMAND curl -L -A "liblufs cmake downloader (technical testing)" -o ${TEST_CONTENT_ZIP_OUTPUT} ${TEST_CONTENT_URL}
    RESULT_VARIABLE curl_result
    ERROR_VARIABLE curl_error
)

if(NOT curl_result EQUAL 0)
    message(FATAL_ERROR "Curl failed to download EBU test files: ${curl_error}")
endif()

file(ARCHIVE_EXTRACT INPUT ${TEST_CONTENT_ZIP_OUTPUT} DESTINATION ${TEST_CONTENT_OUTPUT} VERBOSE)

#Add google test and set it not to build googlemock
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(googletest)

add_executable(tests 
    test_test.cpp
)

set_target_properties(tests PROPERTIES 
    LINKER_LANGUAGE CXX
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(
    tests
    GTest::gtest_main
    lufs
)

include(GoogleTest)
gtest_discover_tests(tests)

install(TARGETS tests)

install(DIRECTORY ${TEST_CONTENT_OUTPUT}
    DESTINATION share)