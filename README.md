# Wii U Rich Presence Plugin

This plugin sets Discord Rich Presence based on the application currently being played, the time the application was loaded, the amount of controllers connected and more.

This fork adds a **direct mode**: the plugin talks to Discord itself over HTTPS, so **no application has to run on your computer**. Your status shows up even when Discord is only open on your phone. The original mode, which sends UDP packets to an application on your computer, is still available.

## Direct mode (no computer needed)

Direct mode uses Discord's headless sessions, the same mechanism used by projects like [Reef](https://github.com/bentheminernz/reef). You log in once with a small script; after that the Wii U keeps the login fresh by itself.

> [!NOTE]
> Headless sessions are not part of Discord's official public documentation, so Discord could change them in the future. The login is a normal OAuth2 login that can only set your activity: it does not give access to your messages or account.

### 1. Create a Discord application

1. Go to the [Discord Developer Portal](https://discord.com/developers/applications) and create a new application (the name is shown in some places, e.g. "Wii U").
2. Copy the **Application ID** from *General Information*.
3. In *OAuth2*, turn on **Public Client** and add the redirect `http://127.0.0.1:8765/callback`.
4. Enable the **Discord Social SDK** for the application (Getting Started form in the Social SDK section).
5. Optional: under *Rich Presence > Art Assets*, upload images named `nn` and `pn` to show a small Nintendo Network / Pretendo icon.

### 2. Log in once

On any computer with Python 3 (no extra packages needed), run [`setup/discord_setup.py`](setup/discord_setup.py) with the Wii U's SD card inserted:

```bash
python setup/discord_setup.py --client-id <APPLICATION ID> --sd <SD CARD ROOT>
```

Your browser opens to log in to Discord. The script checks that presence works and writes `richpresence_discord.json` to `sd:/wiiu/environments/aroma/plugins/config/`. Use `--environment` if your environment is not called `aroma`, or leave out `--sd` to save the file next to the script and copy it yourself.

Keep this file private: anyone who has it can set your Discord activity. If the login ever expires (the plugin's *Setup* menu shows the status), run the script again.

### 3. Install on the Wii U

1. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
2. Copy `CURLWrapperModule.wms` from the [CURLWrapperModule releases](https://github.com/wiiu-env/CURLWrapperModule/releases) into `sd:/wiiu/environments/[ENVIRONMENT]/modules` (already present on most Aroma setups).
3. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
4. In the plugin's configuration menu, *Setup > Send presence* should be set to *directly to Discord* (the default).

The presence is refreshed every few minutes while you play and removed when a title closes. The elapsed time is corrected for the Wii U's clock automatically, so the timezone options are not needed in direct mode.

## Computer mode (original)

The steps below set up the original mode, where an application on your computer forwards the presence to the Discord desktop app. Set *Setup > Send presence* to *via the computer application* in the plugin's configuration menu.

## Installation

<details>
<summary><b>Windows</b></summary>
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Download both the `RichPresence.wps` plugin and the `WURP-Windows` executable from the [Releases page](https://github.com/FlamingNineteen/RichPresenceWUPS/releases).
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
    - Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
3. Keep the executable on your computer.
</details>

<details>
<summary><b>macOS</b></summary>
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Download both the `RichPresence.wps` plugin and the `WURP-macOS` binary from the [Releases page](https://github.com/FlamingNineteen/RichPresenceWUPS/releases).
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
    - Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
3. Keep the binary on your computer.
</details>

<details>
<summary><b>Linux</b></summary>
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)  

1. Download both the `RichPresence.wps` plugin and the `WURP-Linux` binary from the [Releases page](https://github.com/FlamingNineteen/RichPresenceWUPS/releases).
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
    - Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
3. Keep the binary on your computer.

**If you are using a systemd-based system and would like to have the application run on startup, follow these next steps:**

5. Download the `install.sh` script from the [repository](https://github.com/FlamingNineteen/RichPresenceWUPS/blob/main/install.sh)
6. make `install.sh` executable with this:
```bash
chmod +x ./install.sh
```
7. Place the `WURP-Linux` binary into the same directory as `install.sh`.  
8. Run the installation script (this installs the binary as a systemd user process)  
```bash
./install.sh
```
</details>

<details>
<summary><b>Python Script</b></summary>
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)  

1. Download the `RichPresence.wps` plugin from the [Releases page](https://github.com/FlamingNineteen/RichPresenceWUPS/releases).
2. Copy `RichPresence.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
    - Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
3. Download the `discord-script.py` file from the [repository](https://github.com/FlamingNineteen/RichPresenceWUPS/blob/main/discord-script.py).
4. Make sure that [`pypresence`](https://github.com/qwertyquerty/pypresence) and `requests` are installed by running the following command:
```bash
pip install requests pypresence
```
</details>

> [!NOTE]
> After installing, if Discord incorrectly reflects the amount of time you have been playing, open the plugin configuration menu and change the "Offset 'elapsed time' timezone for correct display" setting. If elapsed time displays `0:00:00`, you need to change the setting to a negative number. If the elapsed time displays hours ahead of your actual play time, you need to change the setting to a positive number.

## Usage
Start your Wii U with the environment you placed the plugin in, and run the executable, binary, or Python file with the Discord app open.

You can enter the plugin's configuration settings to change the way presence is displayed.

Additionally, you can specify command-line arguments for advanced customization:
- `-a, --app-id <APPLICATION ID>`: Set the application ID of the Discord app to connect to.
- `-p, --port <PORT>`: Set the UDP port number. Make sure the port matches the one set in the plugin's configuration settings.
- `-r, --repo <DOMAIN/PATH>`: Set the link to the image repository. Omit http(s):// in the link, or the fetch will fail.
- `-v, --version`: Print the version number upon startup.
- `-w, --windows-logs`: Create a window to show logs on Windows.

## Contribute
The plugin might be missing images from a few Wii U games. If you are interested in adding game images, and have a Github account, check out the [image repository](https://github.com/flamingnineteen/RichPresenceWUPS-DB) for this plugin.

## Building
For specifics on building either the plugin or the binary, please check their respective directories:
- Plugin: [wiiu/README.md](https://github.com/FlamingNineteen/RichPresenceWUPS/blob/main/wiiu/README.md)
- Binary: [discord/README.md](https://github.com/FlamingNineteen/RichPresenceWUPS/blob/main/discord/README.md)
