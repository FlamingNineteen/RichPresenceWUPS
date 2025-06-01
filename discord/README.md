## Building

For building you need:
- [discord_social_sdk](https://discord.com/developers/docs/discord-social-sdk/getting-started/using-c++#step-4-download-the-discord-sdk-for-c++)
- [json](https://github.com/nlohmann/json)

It is also recommended to build in the Visual Studio Development Console.

In the root of this directory, create a new directory `lib` and copy `discord_social_sdk` into it. In the root of this directory, create a new directory `include` and copy `nholmann` into it. Then run the following commands:
```
mkdir build && cd build
cmake ..
cmake --build . --config Release
```