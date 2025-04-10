# Wii U Rich Presence Plugin

This plugin uses FTP to communicate with an application on your computer to set Discord Rich Presence for the user. The activity is set based on the application currently being played, the time the application was loaded, and the amount of controllers connected.

## Installation
(`[ENVIRONMENT]` is a placeholder for the actual environment namev.)

1. Copy the file `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
2. Requires the [ftpiiu plugin](https://github.com/wiiu-env/ftpiiu_plugin) in `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
3. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
5. Keep the `WURPdiscord` folder on your computer.

## Usage
Make sure ftpiiu is installed and enabled. Run `WiiURichPresence` in the `WURPdiscord` folder with Discord app open or Discord open on your browser. Authorize the app. The first time you use the app, it will ask for your Wii U's IP address. You can find the IP in the plugin configuration menu under ftpiiu on your Wii U.

## Building

For building you need:
- [wut](https://github.com/decaf-emu/wut)
- [wups](https://github.com/wiiu-env/WiiUPluginSystem)