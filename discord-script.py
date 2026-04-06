#!/usr/bin/env python3
import asyncio
import json
import requests
import socket
import sys
import time

from datetime import datetime
from pypresence import Presence
from pypresence.types import ActivityType, StatusDisplayType

APP_ID = "1353248127469228074"
REPO = "flamingnineteen/richpresencewups-db"
PORT = 5005
VERSION = 2.1

# Check for command line arguments
i = 2
while i < len(sys.argv):
    if (sys.argv[i - 1] == "repo"):
        REPO = sys.argv[i]
        print(f"Using repository {REPO}.")
    if (sys.argv[i - 1] == "port"):
        PORT = int(sys.argv[i])
        print(f"Using port {PORT}.")
    i+=2

# Check for updates
update = requests.get(f'https://github.com/FlamingNineteen/RichPresenceWUPS/releases/tag/v{VERSION + 0.1}')
if (update.status_code >= 200 and update.status_code < 300):
    print('A new update is available')

# Connect to Discord
client = Presence(client_id = APP_ID)
disconnected = True
while disconnected:
    try:
        client.connect()
        disconnected = False
    except:
        print("Failed to connect to Discord. Retrying...")
        time.sleep(2)
print("Connected to Discord")

# Bind Socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
binded = False
while not binded:
    try:
        sock.bind(('', PORT))
        binded = True
        print(f"Binded to UDP port {PORT}")
    except:
        print(f"Failed to bind to UDP port {PORT}. Is another program using the port? Retrying...")
        time.sleep(2)

# Recieve Image URLs
titles = {}
try:
    req = requests.get(f"https://raw.githubusercontent.com/{REPO}/main/titles.json")
    titles = json.loads(req.text)
    print("Successfully fetched titles.json!")
except:
    print("Error fetching titles.json. Using default image.")

# Parses the recieved json into a json object
def parse(msg):
    try:
        i = json.loads(msg)
    except:
        i = {}
    return i

# Change the recieved time elapsed to epoch
def toepoch(e, dst = False):
    dt = (datetime.now() if dst else datetime.utcnow()).astimezone()
    return e - int(dt.utcoffset().total_seconds())

# Asynchronous function to stop Rich Presence if nothing is recieved
idle = True

async def clearPresence():
    global idle, client
    allow = False
    while 1:
        await asyncio.sleep(5)
        if idle:
            if allow:
                await asyncio.to_thread(client.clear)
                print("Cleared Rich Presence")
                while idle:
                    await asyncio.sleep(0.1)
            else:
                allow = True
        else:
            allow = False
        idle = True

# Main loop
async def main():
    global idle
    while 1:
        # Wait for a message
        print("Waiting to recieve message")
        msg = await asyncio.to_thread(sock.recv, 1024)
        data = parse(msg.decode())
        print(f"Recieved: {data}")
        idle = False

        # Attempt to set Rich Presence
        try:
            if (data["sender"] == "Wii U"):
                image = ""
                try:
                    image = f"https://raw.githubusercontent.com/{REPO}/main/icons/{titles[data["long"]]}"
                except:
                    image = "preview"
                
                img = data['img'] if 'img' in data else ''
                dst = (data['dst'] == 1) if 'dst' in data else False

                await asyncio.to_thread(client.update,
                    activity_type=       ActivityType.PLAYING,
                    status_display_type= StatusDisplayType.STATE,
                    state=               data["app"],
                    details=             None if data["nnid"] == '' else f"Network ID: {data["nnid"]}",
                    start=               toepoch(data["time"], dst),
                    large_image=         image,
                    large_text=          data["long"],
                    party_size=          [data["ctrls"] + 1 if data["ctrls"] > -2 else 0, 4 if data["ctrls"] < 4 else 8],
                    small_image=         None if img == "" else img,
                    small_text=          f"Using {"Nintendo" if img == "nn" else "Pretendo"} Network"
                )

                print("Updated Rich Presence")
        except:
            print("Failed to update Rich Presence")

async def run_all():
    await asyncio.gather(
        clearPresence(),
        main()
    )

asyncio.run(run_all())

sock.close()
