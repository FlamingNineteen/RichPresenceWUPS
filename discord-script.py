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
REPO = "flamingnineteen/richpresencewups-db" if len(sys.argv) < 2 else sys.argv[2]

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
        sock.bind(('', 5005))
        binded = True
        print("Binded to UDP port 5005")
    except:
        print("Failed to bind to UDP port 5005. Is another program using the port? Retrying...")
        time.sleep(2)

# Recieve Image URLs
titles = {}
try:
    req = requests.get(f"https://raw.githubusercontent.com/{REPO}/main/titles.json")
    titles = json.loads(req.text)
    print("Successfully fetched titles.json!")
except:
    print("Error fetching titles.json. Using default image.")

def parse(msg):
    p = msg.split('\'')[1]
    try:
        i = json.loads(p)
    except:
        i = {}
    return i

def toepoch(e):
    dt = datetime.now().astimezone()
    return e - int(dt.utcoffset().total_seconds())

idle = True

async def clearPresence():
    global idle, client
    allow = False
    while 1:
        await asyncio.sleep(5)
        if idle:
            if allow:
                await asyncio.to_thread(client.clear) # TODO: RICH PRESENCE DOES NOT CLEAR
                print("Cleared Rich Presence")
                while idle:
                    await asyncio.sleep(0.1)
            else:
                allow = True
        else:
            allow = False
        idle = True

async def main():
    global idle
    while 1:
        print("Waiting to recieve message")
        msg = await asyncio.to_thread(sock.recv, 1024)
        data = parse(str(msg))
        print(f"Recieved: {data}")
        idle = False

        try:
            if (data["sender"] == "Wii U"):
                img = ""
                try:
                    img = f"https://raw.githubusercontent.com/{REPO}/main/icons/{titles[data["long"]]}"
                except:
                    img = "preview"

                await asyncio.to_thread(client.update,
                    activity_type=       ActivityType.PLAYING,
                    status_display_type= StatusDisplayType.STATE,
                    state=               data["app"],
                    details=             None if data["nnid"] == '' else f"Network ID: {data["nnid"]}",
                    start=               toepoch(data["time"]),
                    large_image=         img,
                    large_text=          data["long"],
                    party_size=          [data["ctrls"] + 1 if data["ctrls"] > -2 else 0, 4 if data["ctrls"] < 4 else 8],
                    small_image=         None if data["img"] == "" else data["img"],
                    small_text=          f"Using {"Nintendo" if data["img"] == "nn" else "Pretendo"} Network"
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
