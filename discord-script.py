import asyncio
import json
import requests
import socket
import time

from datetime import datetime
from pypresence import Presence
from pypresence.types import ActivityType, StatusDisplayType

APP_ID = "1353248127469228074"
REPO = "flamingnineteen/richpresencewups-db"

# Connect to Discord
client = Presence(client_id = APP_ID)

disconnected = True
while disconnected:
    try:
        client.connect()
        disconnected = False
    except:
        print("Failed to connect to Discord. Retrying...")
        time.sleep(1)
print("Connected to Discord")

# Bind Socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
ip = socket.gethostbyname(socket.gethostname())

sock.bind((ip, 5005))

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
                await asyncio.to_thread(client.clear)
                print("Cleared Rich Presence")
                while idle:
                    pass
            else:
                allow = True
        else:
            allow = False
        idle = True

asyncio.run(clearPresence())

while 1:
    print("Waiting to recieve message")
    msg = sock.recv(1024)
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

            if data["nnid"] == '':
                client.update(
                    activity_type=       ActivityType.PLAYING,
                    status_display_type= StatusDisplayType.STATE,
                    state=               data["app"],
                    start=               toepoch(data["time"]),
                    large_image=         img,
                    party_size=          [data["ctrls"] + 1 if data["ctrls"] > -2 else 0, 4 if data["ctrls"] < 4 else 8]
                )
            else:
                client.update(
                    activity_type=       ActivityType.PLAYING,
                    status_display_type= StatusDisplayType.STATE,
                    state=               data["app"],
                    details=             f"Network ID: {data["nnid"]}",
                    start=               toepoch(data["time"]),
                    large_image=         img,
                    party_size=          [data["ctrls"] + 1 if data["ctrls"] > -2 else 0, 4 if data["ctrls"] < 4 else 8]
                )
    except:
        print("Updated Rich Presence")

sock.close()
