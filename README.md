# liblufs - Live Laugh LUF
A C++ library for measuring loudness according to ITU 1770-5 and EBU R128

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

Permission has kindly been given by the EBU for this project to host the test files used in EBU TECH 3341 and EBU TECH 3342, please find the original files here: https://tech.ebu.ch/publications/ebu_loudness_test_set 

The test project must then be run from the same location as the test content folder.