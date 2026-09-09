#include <discord-rpc.hpp>
#include <fmt/format.h>

#include "version.h"

#include "json.hpp"
using json = nlohmann::json;

constexpr auto APPLICATION_ID = "1353248127469228074";
unsigned short UDP_PORT = 5005;
std::atomic<bool> idle = false;
std::atomic<bool> runIdleLoop = true;
bool updateMsg = false;

// Setup the Rich Presence manager and its events
void discordSetup() {
    discord::RPCManager::get()
        .setClientID(APPLICATION_ID)
        .onReady([](discord::User const& user) {
            fmt::println("Discord: connected to user {}#{} - {}", user.username, user.discriminator, user.id);
        })
        .onDisconnected([](int errcode, std::string_view message) {
            fmt::println("Discord: disconnected with error code {} - {}", errcode, message);
        })
        .onErrored([](int errcode, std::string_view message) {
            fmt::println("Discord: error with code {} - {}", errcode, message);
        });
}

// Sets the Rich Presence
void updatePresence(std::string repo, std::string game, std::string full, std::string nnid, int ctrls, std::string jpg, std::string img, time_t start, std::string details = "") {
    idle = false;
    auto& rpc = discord::RPCManager::get();

    rpc.getPresence()
        .setName(game)
        .setActivityType(discord::ActivityType::Game)
        .setStatusDisplayType(discord::StatusDisplayType::Name)
        .setState(nnid != "" ? "NID: " + nnid : "")
        .setDetails(details != "" ? details : "Playing on the Wii U")
        .setStartTimestamp(start)
        .setLargeImageKey((jpg == "oh no it didn't work") ? "preview" : ("https://raw.githubusercontent.com/" + repo + "/main/icons/" + jpg))
        .setLargeImageText(full)
        .setSmallImageKey(img == "backwards" ? "" : img)
        .setSmallImageText((nnid == "" || details == "") ? (img == "nn" ? "Using Nintendo Network" : "Using Pretendo Network") : ("ID: " + nnid))
        .setPartyID(ctrls > -2 ? "wiiu" : "")
        .setPartySize(ctrls > -2 ? ctrls + 1 : 0)
        .setPartyMax((ctrls + 1 > 4) ? 8 : 4)
        .setPartyPrivacy(discord::PartyPrivacy::Public)
        .setInstance(false)
        .refresh();
    
    fmt::println("Updated Rich Presence");
}

// Asynchronous function to stop Rich Presence if nothing is recieved
void checkIdle() {
	bool allow = false;
    bool already = false;
    auto& rpc = discord::RPCManager::get();
	while (runIdleLoop) {
		std::this_thread::sleep_for(std::chrono::seconds(5));
		if (idle) {
            if (allow && !already) {
                rpc.clearPresence();
                fmt::println("Cleared Rich Presence");
                already = true;
            } else {
                allow = true;
            }
        }
        else {
            allow = false;
            already = false;
        }
		idle = true;
	}
	return;
}

short parseJsonAndUpdate(std::string msg, json images, std::string repo, time_t (*adjustEpochToUtc)(time_t, bool)) {
    std::string image;

    try {
        json out = json::parse(msg);

        // Check if the sender is the Wii U
        if (out["sender"] == "Wii U") {
            fmt::println("Received: {}", msg);
            idle = false;
        }
        else {
            return 0;
        }

        // Try to get the icon from the database
        try {
            image = images[out["long"]];
        } catch (...) {
            image = "oh no it didn't work";
        }
        
        // Update presence, but also make sure it's backwards compatible
        if (out.contains("dst")) { // Update 2.1
            updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, out["img"], adjustEpochToUtc(out["time"], out["dst"] == 1), out.contains("details") ? out["details"] : "");
        }
        else if (out.contains("img")) { // Update 2.0
            updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, out["img"], adjustEpochToUtc(out["time"], false));
        }
        else { // Update 1.9
            updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, "backwards", adjustEpochToUtc(out["time"], false));
        }

        // Check for updates
        if (out.contains("compatibility")) {
            if ((out["compatibility"] > VERSION) && !updateMsg) {
                double v = out["compatibility"];
                fmt::println("A new update is available: v{}", v);
                updateMsg = true;
            }
        }
    }
    catch (...) {
        return -1;
    }

    return 0;
}
