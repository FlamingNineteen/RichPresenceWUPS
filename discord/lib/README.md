# Discord RPC
A simple Discord Rich Presence library for C++ made from scratch.

> **Note**: Project is still in development. Please report any issues you find.

**Checklist**:
- [x] Handshake
- [x] Update presence with custom data
- [x] More features (buttons, activity type, etc.)
- [x] Linux support
- [x] MacOS support
- [x] Windows support
- [x] Out-of-the-box Wine support *(Only supports Wine Staging 10.2+ because of AF_UNIX socket support)*
- [ ] Events (join, spectate, etc.)
- [ ] Auto register (for steam games)
- [ ] Social SDK features
- [ ] Documentation
- [ ] Examples

---

## What's new?

In spite of the deprecation of [the original library](https://github.com/discord/discord-rpc),
many people still want to use it. This library is a complete rewrite of the original library
that uses a modern C++ style and has more features.

### Features
- **Modern C++**: This library is written in C++23 and follows builder patterns as much as possible.
- **More features**: Implements new features from Discord RPC documentation that is not present in the original library, such as buttons, activity type, and more.
- **Project integration**: Say goodbye to copying library files or dealing with RapidJSON errors. This library can be easily integrated into your project without any hassle.
- **Cross-platform**: This library is designed to work on all supported platforms, including Linux, macOS, and Windows.
- **Wine support**: Library provides internal layer to support Wine, no extra configuration needed.

---

### Prerequisites
- **CMake**: This project uses CMake to build the library. Make sure you have CMake installed on your system.
- **C++23**: Required for [Glaze](https://github.com/stephenberry/glaze), a fast JSON library used in this project.
- **Discord Developer Portal**: You need to create an application on the Discord Developer Portal to use this library. You can create an application [here](https://discord.com/developers/applications).

### Credits
- [Discord](https://github.com/discord/discord-rpc): For creating the original library.
- [Glaze](https://github.com/stephenberry/glaze): JSON library
- [fmtlib](https://github.com/fmtlib/fmt): Formatting library

### License
This project is licensed under the MIT License. See the [LICENSE.md](LICENSE.md) file for details.