# Wii U Rich Presence Plugin

This plugin uses UDP to communicate with an application on your computer to set Discord Rich Presence for the user. The activity is set based on the application currently being played, the time the application was loaded, and the amount of controllers connected.

> [!NOTE]
> To offset the elapsed time that shows up in Discord by a certain amount of hours, open the plugin configuration menu and change the "Offset 'elapsed time' timezone for correct display" setting. If elapsed time displays `0:00:00`, you need to change the setting to a negative number. If elapsed time displays hours ahead of your actual play time, you need to change the setting to a positive number.

## Installation (Windows, MacOS)
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Download both `RichPresence.wps` and the executable associated with your operating system.
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
3. Keep the executable on your computer.
4. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.

## Installation (Linux)
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)  
**If not using a systemd-based system, refer to the Windows/MacOS instructions instead.**

1. Clone this repository using git.  
```bash
git clone https://github.com/cmyksoda/RichPresenceWUPS.git
```
3. Download both `RichPresence.wps` and the `WURP-Linux` binary from the releases page.
4. Place the `WURP-Linux` binary into the same directory as this repository.  
5. Run the installation script (this installs WUPS as a systemd user process)  
```bash
./install.sh
```
6. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
7. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.

## Usage
Start your Wii U with the environment you placed the plugin in, and run the executable file with the Discord app open.

If you are using `discord-script.py`, make sure that [`pypresence`](https://github.com/qwertyquerty/pypresence) and `requests` are installed

```
pip install requests pypresence
```

Additionally, you can specify command-line arguments for the executables.
- Append `repo username/repository` to change the image repository. Replace `username/repository` with the desired GitHub repository.
- Append `port num` to change the UDP port number, where `num` is the port number you wish to use.

## Contribute
The plugin is missing images of many Wii U games. If you are interested in adding game images, and have a Github account, check out the [image repository](https://github.com/flamingnineteen/RichPresenceWUPS-DB) for this plugin.

## Building
For specifics on building either the plugin or the executable, please check the respective directories.




