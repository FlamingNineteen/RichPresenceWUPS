#!/bin/bash

# exit immediately if any command fails
set -e

# check if the binary actually exists in the current directory first
if [ ! -f "WiiURichPresence" ]; then
    echo "Error: WiiURichPresence binary not found in the current directory."
    echo "Please run this script from the folder where WiiURichPresence is located."
    exit 1
fi

# moves WiiURichPresence to local binaries folder and makes it executable
mkdir -p ~/.local/bin
mv WiiURichPresence ~/.local/bin/
chmod +x ~/.local/bin/WiiURichPresence

# set up systemd service
mkdir -p ~/.config/systemd/user
cat << 'EOF' > ~/.config/systemd/user/wurp.service
[Unit]
Description=Wii U Rich Presence Service
After=network.target

[Service]
Type=simple
ExecStart=%h/.local/bin/WiiURichPresence
Restart=on-failure

[Install]
WantedBy=default.target
EOF

# enable service
systemctl --user daemon-reload
systemctl --user enable --now wurp.service

# print installation message
echo "WUPS Service Installed and Started Successfully!"
