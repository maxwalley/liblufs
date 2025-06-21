# liblufs - Live Laugh LUF
A C++ library for measuring loudness according to EBU R128

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

Testing requires downloading the test dataset from https://tech.ebu.ch/publications/ebu_loudness_test_set and placing in the test directory of the project. The EBU has this file behind a cloudflare layer that prevents automated downloading.

The test project must then be run from the same location as the test content folder.