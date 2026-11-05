#!/usr/bin/env python3
"""
One-time Discord login for the Wii U Rich Presence plugin's direct mode.

Run this once on any computer. It lets you log in to Discord in your browser,
checks that your Discord application can set presence, and writes
`richpresence_discord.json`. Put that file on the Wii U's SD card in
`wiiu/environments/[ENVIRONMENT]/plugins/config/` and the plugin keeps the
login fresh by itself from then on. Nothing has to keep running on the
computer afterwards.

Only the Python standard library is used.

    python discord_setup.py --client-id <APPLICATION ID>
    python discord_setup.py --client-id <APPLICATION ID> --sd E:\\
"""

import argparse
import base64
import hashlib
import http.server
import json
import os
import secrets
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import webbrowser

API = "https://discord.com/api/v10"
AUTHORIZE_URL = "https://discord.com/oauth2/authorize"
SCOPES = "openid sdk.social_layer_presence"
PORT = 8765
REDIRECT_URI = f"http://127.0.0.1:{PORT}/callback"
USER_AGENT = "WiiURichPresence-setup (https://github.com/sennecaelen/RichPresenceWUPS)"
TOKEN_FILE_NAME = "richpresence_discord.json"


def request(url, data=None, token=None, form=False):
    """Small JSON/form HTTP helper. Returns (status, parsed body or text)."""
    headers = {"User-Agent": USER_AGENT}
    body = None
    if data is not None:
        if form:
            body = urllib.parse.urlencode(data).encode()
            headers["Content-Type"] = "application/x-www-form-urlencoded"
        else:
            body = json.dumps(data).encode()
            headers["Content-Type"] = "application/json"
    if token:
        headers["Authorization"] = f"Bearer {token}"

    req = urllib.request.Request(url, data=body, headers=headers, method="POST" if body is not None else "GET")
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            status, raw = resp.status, resp.read()
    except urllib.error.HTTPError as e:
        status, raw = e.code, e.read()

    text = raw.decode(errors="replace")
    try:
        return status, json.loads(text) if text else {}
    except ValueError:
        return status, text


def wait_for_code(expected_state):
    """Runs a tiny local web server until Discord redirects back with a code."""
    result = {}

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            parsed = urllib.parse.urlparse(self.path)
            if parsed.path != "/callback":
                self.send_response(404)
                self.end_headers()
                return
            query = urllib.parse.parse_qs(parsed.query)
            result["code"] = query.get("code", [None])[0]
            result["state"] = query.get("state", [None])[0]
            result["error"] = query.get("error_description", query.get("error", [None]))[0]

            ok = result["code"] and result["state"] == expected_state
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            message = "Login complete, you can close this tab." if ok else "Login failed, check the terminal."
            self.wfile.write(f"<html><body style='font-family:sans-serif'><h2>{message}</h2></body></html>".encode())

        def log_message(self, *args):
            pass

    server = http.server.HTTPServer(("127.0.0.1", PORT), Handler)
    server.timeout = 1
    deadline = time.time() + 300
    while "code" not in result and "error" not in result and time.time() < deadline:
        server.handle_request()
    server.server_close()

    if result.get("error") and not result.get("code"):
        sys.exit(f"Discord returned an error: {result['error']}")
    if not result.get("code"):
        sys.exit("No login received within 5 minutes.")
    if result.get("state") != expected_state:
        sys.exit("The login response did not match this request, try again.")
    return result["code"]


def test_presence(client_id, access_token):
    """Creates and removes a short headless session to prove everything works."""
    activity = {
        "name": "Wii U Rich Presence setup",
        "application_id": client_id,
        "platform": "desktop",
        "type": 0,
        "details": "Setup test",
    }
    status, body = request(f"{API}/users/@me/headless-sessions", {"activities": [activity]}, token=access_token)
    if status != 200:
        return False, f"{status}: {body}"

    session = body.get("token") if isinstance(body, dict) else None
    if session:
        request(f"{API}/users/@me/headless-sessions/delete", {"token": session}, token=access_token)
    return True, ""


def output_path(args):
    if args.sd:
        folder = os.path.join(args.sd, "wiiu", "environments", args.environment, "plugins", "config")
        os.makedirs(folder, exist_ok=True)
        return os.path.join(folder, TOKEN_FILE_NAME)
    return os.path.abspath(args.output)


def main():
    parser = argparse.ArgumentParser(description="One-time Discord login for Wii U Rich Presence (direct mode).")
    parser.add_argument("--client-id", required=True, help="Application ID of your Discord application")
    parser.add_argument("--client-secret", help="Only needed if 'Public Client' is turned off for your application")
    parser.add_argument("--sd", help="Root of the Wii U SD card (e.g. E:\\ or /media/sd); writes the file to the right folder")
    parser.add_argument("--environment", default="aroma", help="Aroma environment name on the SD card (default: aroma)")
    parser.add_argument("--output", default=TOKEN_FILE_NAME, help="Output file when --sd is not used")
    parser.add_argument("--no-browser", action="store_true", help="Print the login link instead of opening a browser")
    args = parser.parse_args()

    client_id = args.client_id.strip()
    verifier = secrets.token_urlsafe(64)
    challenge = base64.urlsafe_b64encode(hashlib.sha256(verifier.encode()).digest()).decode().rstrip("=")
    state = secrets.token_urlsafe(16)

    url = AUTHORIZE_URL + "?" + urllib.parse.urlencode({
        "response_type": "code",
        "client_id": client_id,
        "scope": SCOPES,
        "redirect_uri": REDIRECT_URI,
        "state": state,
        "code_challenge": challenge,
        "code_challenge_method": "S256",
        "prompt": "consent",
    })

    print("Log in to Discord and allow access:")
    print(url)
    if not args.no_browser:
        webbrowser.open(url)

    code = wait_for_code(state)

    data = {
        "client_id": client_id,
        "grant_type": "authorization_code",
        "code": code,
        "redirect_uri": REDIRECT_URI,
        "code_verifier": verifier,
    }
    if args.client_secret:
        data["client_secret"] = args.client_secret

    status, tokens = request(f"{API}/oauth2/token", data, form=True)
    if status != 200 or not isinstance(tokens, dict) or "access_token" not in tokens:
        print(f"Token exchange failed ({status}): {tokens}")
        if status in (400, 401):
            print("Check that 'Public Client' is on for your application (OAuth2 page),")
            print(f"and that {REDIRECT_URI} is added as a redirect. Or pass --client-secret.")
        sys.exit(1)

    print("Logged in. Testing presence...")
    ok, error = test_presence(client_id, tokens["access_token"])
    if not ok:
        print(f"Discord refused to set presence ({error}).")
        print("Make sure the Discord Social SDK is enabled for your application")
        print("(Developer Portal > your app > Social SDK) and run this script again.")
        sys.exit(1)

    result = {
        "client_id": client_id,
        "access_token": tokens["access_token"],
        "refresh_token": tokens.get("refresh_token", ""),
        "expires_at": int(time.time()) + int(tokens.get("expires_in", 604800)),
    }
    if args.client_secret:
        result["client_secret"] = args.client_secret

    path = output_path(args)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=4)

    print(f"Saved {path}")
    if not args.sd:
        print(f"Copy it to sd:/wiiu/environments/{args.environment}/plugins/config/{TOKEN_FILE_NAME}")
    print("Keep this file private: it lets anyone set your Discord activity.")


if __name__ == "__main__":
    main()
