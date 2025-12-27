# Wii U Rich Presence Plugin

This plugin uses UDP port 5005 to communicate with an application on your computer to set Discord Rich Presence for the user. The activity is set based on the application currently being played, the time the application was loaded, and the amount of controllers connected.

> [!NOTE]
> Elapsed time may not show up correctly because of your Wii U's time. To offset the elapsed time that shows up in Discord by a certain amount of hours, open the plugin configuration menu and change the setting. If elapsed time displays `0:00:00`, you need to change the setting to a negative number. If elapsed time displays hours ahead of your actual play time, you need to change the setting to a positive number.

## Installation
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Download both `RichPresence.wps` and the executable associated with your operating system.
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
3. Keep the executable on your computer.
4. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.

## Usage (Executable)
Start your Wii U with the environment you placed the plugin in, and run the executable file with the Discord app open.

> [!IMPORTANT]
> On Linux, if you get the error `Segmentation fault (core dumped)`, use `sudo`.

## Usage (Python)
Make sure that [`pypresence`](https://github.com/qwertyquerty/pypresence) and `requests` are installed

```
pip install requests pypresence
```

Start your Wii U with the environment you placed the plugin in, and run the executable file with the Discord app open.

## Contribute
The plugin is missing images of many Wii U games. If you are interested in adding game images, and have a Github account, check out the [image repository](https://github.com/flamingnineteen/RichPresenceWUPS-DB) for this plugin.

## Building
For specifics on building either the plugin or the executable, please check the respective directories.


