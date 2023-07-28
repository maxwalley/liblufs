#Download the test files, probably a better way of doing this which means it doesn't do it every time and it only does it when the tests are run
file(DOWNLOAD https://tech.ebu.ch/files/live/sites/tech/files/shared/testmaterial/ebu-loudness-test-setv05.zip test_files.zip SHOW_PROGRESS)
file(ARCHIVE_EXTRACT INPUT test_files.zip DESTINATION ../test_files VERBOSE)

#Add google test and set it not to build googlemock
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
add_subdirectory(googletest)

add_executable(tests 
    test_test.cpp
)

target_link_libraries(
    tests
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)