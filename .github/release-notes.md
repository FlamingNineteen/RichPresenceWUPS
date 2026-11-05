## Direct mode: no computer needed

This fork adds a **direct mode**: the plugin talks to Discord itself over HTTPS, so no application has to run on your computer. Your status shows up even when Discord is only open on your phone. The original computer mode is still available.

### What's new
- Presence is sent straight from the Wii U using Discord's headless sessions
- One-time login with `discord_setup.py` (Python 3, no extra packages)
- Nintendo Network, Pretendo and **Protarium** icons; NNID / PNID shown under the game
- Controller count, elapsed time and game icons like the original
- Status line in the plugin's *Setup* menu explains any problem
- Choose *directly to Discord* or *via the computer application* in *Setup > Send presence*

### Install
1. Create a Discord application, turn on **Public Client**, add the redirect `http://127.0.0.1:8765/callback` and enable the **Social SDK** (see the README).
2. Run `python discord_setup.py --client-id <APPLICATION ID> --sd <SD CARD ROOT>` once.
3. Copy `RichPresence.wps` to `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.
4. Make sure `CURLWrapperModule.wms` is in `sd:/wiiu/environments/[ENVIRONMENT]/modules` ([download](https://github.com/wiiu-env/CURLWrapperModule/releases)).

> Headless sessions are not part of Discord's official public documentation, so Discord could change them in the future.

The computer applications (`WURP-*`) are only needed for computer mode.
