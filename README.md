[![Build and test on multiple platforms](https://github.com/maxwalley/liblufs/actions/workflows/build_and_test_multi_plat.yml/badge.svg)](https://github.com/maxwalley/liblufs/actions/workflows/build_and_test_multi_plat.yml)

# liblufs - Live Laugh LUF
A C++ library for realtime safe measurement of loudness according to ITU 1770-5 and EBU R128

Build Instructions:
```
mkdir build
cd build 
cmake ..
make
make install
```

## Testing
Tests are automatically built, to turn them off add the option ```-DBUILD_TESTS=OFF``` to the cmake line

Permission has kindly been given by the EBU for this project to host the test files referenced in EBU Tech 3341 and EBU Tech 3342. Please find the original files here: https://tech.ebu.ch/publications/ebu_loudness_test_set 

To build and run the tests use the following commands:
```
mkdir build
cd build 
cmake ..
make
ctest
```

## Licensing
This project is licensed under the GPL v3.0 license. 

Contact max.walley1@btinternet.com for information regarding commercial licensing.
