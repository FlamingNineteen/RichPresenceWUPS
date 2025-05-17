## Building

For building you need:
- Discord Social SDK

It is also recommended to build in the Visual Studio Development Console.

In the root of this folder, create a new directory named `lib` and add the `discord_social_sdk` to the directory. Then run the following commands:
```
mkdir build && cd build
cmake ..
cmake --build . --config Release
```