# Wii U Rich Presence Plugin

This plugin uses FTP to communicate with an application on your computer to set Discord Rich Presence for the user. The activity is set based on the application currently being played, the time the application was loaded, and the amount of controllers connected.

## Installation
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Download both `wurpWiiU.zip` and `wurpDiscord.zip` and extract both of them.
2. From `wurpWiiU`, copy the file `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
3. Requires the [ftpiiu plugin](https://github.com/wiiu-env/ftpiiu_plugin) in `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
4. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
5. Keep the `wurpDiscord` folder on your computer.

## Usage
Make sure ftpiiu is installed and enabled. Run `WiiURichPresence` in the `wurpDiscord` folder with the Discord app open or Discord open on your browser. Authorize the app in Discord. The first time you use the app, it will ask for your Wii U's IP address. You can find the IP in the plugin configuration menu under ftpiiu on your Wii U.

Elapsed time may not show up correctly because of your Wii U's time. To offset the elapsed time that shows up in Discord by a certain amount of hours, close the program, open the `save.json` in the `wurpDiscord` folder with a text editor, and change the `time` key to a number between -12 and 12.

## Building
For specifics on building either the plugin or the executable, please check the respective directories.
