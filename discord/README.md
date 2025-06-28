## Building

For building you need:
- [discord_social_sdk](https://discord.com/developers/docs/discord-social-sdk/getting-started/using-c++#step-4-download-the-discord-sdk-for-c++)
- [json](https://github.com/nlohmann/json)
- [boost](https://www.boost.org/releases/latest/) (Linux and macOS only)

In the root of this directory, create a new directory `lib` and copy `discord_social_sdk` into it. In the root of this directory, create a new directory `include` and copy `nholmann` into it. If you are building for Linux or macOS, also add the Boost library's `include` and `lib` folders. Then run the following commands:
```
mkdir build && cd build
cmake ..
cmake --build . --config Release
```