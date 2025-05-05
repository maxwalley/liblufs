# liblufs
A C++ library for measuring loudness according to EBU R128

Build Instructions:
```
mkdir build
cd build 
cmake ..
make
make install
```

Tests are automatically built, to turn them off add the option ```-DBUILD_TESTS=OFF``` to the cmake line