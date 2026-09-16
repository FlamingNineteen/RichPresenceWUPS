## Building

For building you need:
- [curl](https://curl.se) (UNIX devices only)

When cloning the repository, make sure to also fetch submodules:
```bash
git submodule update --init
```

Then run the following commands in this directory:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Compiling for Windows on Linux using MinGW
After cloning, run the following commands in this directory:
```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw64.cmake
cmake --build .
```
