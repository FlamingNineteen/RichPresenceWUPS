# Wii U Rich Presence Plugin

This plugin uses UDP to communicate with an application on your computer to set Discord Rich Presence for the user. The activity is set based on the application currently being played, the time the application was loaded, and the amount of controllers connected.

> [!NOTE]
> After installing, if Discord incorrectly reflects the amount of time you have been playing, open the plugin configuration menu and change the "Offset 'elapsed time' timezone for correct display" setting. If elapsed time displays `0:00:00`, you need to change the setting to a negative number. If the elapsed time displays hours ahead of your actual play time, you need to change the setting to a positive number.

## Installation

<details>
<summary><b>Windows</b></summary>
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Download both the `RichPresence.wps` plugin and the `WURP-Windows` executable from the [Releases page](https://github.com/FlamingNineteen/RichPresenceWUPS/releases).
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
3. Keep the executable on your computer.
4. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
</details>

<details>
<summary><b>macOS</b></summary>
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Download both the `RichPresence.wps` plugin and the `WURP-macOS` binary from the [Releases page](https://github.com/FlamingNineteen/RichPresenceWUPS/releases).
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
3. Keep the executable on your computer.
4. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
</details>

<details>
<summary><b>Linux</b></summary>
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)  

1. Download both the `RichPresence.wps` plugin and the `WURP-Linux` binary from the [Releases page](https://github.com/FlamingNineteen/RichPresenceWUPS/releases).
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
3. Keep the executable on your computer.
4. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.

**If you are using a systemd-based system and would like to have the application run on startup, follow these next steps:**

5. Clone this repository using git.  
```bash
git clone https://github.com/cmyksoda/RichPresenceWUPS.git
```
6. Place the `WURP-Linux` binary into the same directory as this repository.  
7. Run the installation script (this installs the binary as a systemd user process)  
```bash
./install.sh
```
</details>

<details>
<summary><b>Python Script</b></summary>
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)  

1. Download the `RichPresence.wps` plugin from the [Releases page](https://github.com/FlamingNineteen/RichPresenceWUPS/releases).
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
3. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
4. Download the `discord-script.py` file from the [repository](https://github.com/FlamingNineteen/RichPresenceWUPS/blob/main/discord-script.py).
5. Make sure that [`pypresence`](https://github.com/qwertyquerty/pypresence) and `requests` are installed by running the following command:
```bash
pip install requests pypresence
```
</details>

## Usage
Start your Wii U with the environment you placed the plugin in, and run the executable, binary, or Python file with the Discord app open.

Additionally, you can specify command-line arguments for advanced customization:
- Append `repo username/repository` to change the image repository. Replace `username/repository` with the desired GitHub repository.
- Append `port num` to change the UDP port number, where `num` is the port number you wish to use.

## Contribute
The plugin might be missing images from a few Wii U games. If you are interested in adding game images, and have a Github account, check out the [image repository](https://github.com/flamingnineteen/RichPresenceWUPS-DB) for this plugin.

## Building
For specifics on building either the plugin or the binary, please check their respective directories.
