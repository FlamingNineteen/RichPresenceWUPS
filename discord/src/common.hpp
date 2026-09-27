#include <discord-rpc.hpp>
#include <fmt/format.h>
#include <chrono>
#include <mutex>

#include "version.h"

#include "json.hpp"
using json = nlohmann::json;

std::atomic<bool> runIdleLoop = true;
constexpr auto presenceTimeout = std::chrono::seconds(60);
std::mutex presenceMutex;
bool presenceActive = false;
std::chrono::steady_clock::time_point lastPresenceUpdate;
uint8_t updateMsg = 2;

// Setup the Rich Presence manager and its events
void discordSetup(std::string app_id) {
    discord::RPCManager::get()
        .setClientID(app_id)
        .onReady([](discord::User const& user) {
            fmt::println("Discord: connected to user {}#{}", user.username, user.discriminator);
            // fmt::println("Discord: connected to user {}#{} - {}", user.username, user.discriminator, user.id);
            std::lock_guard lock(presenceMutex);
            auto& rpc = discord::RPCManager::get();
            // Refreshing an empty presence creates a blank activity card after reconnecting.
            if (presenceActive && std::chrono::steady_clock::now() - lastPresenceUpdate < presenceTimeout) {
                rpc.refresh();
            } else {
                presenceActive = false;
                rpc.clearPresence();
                fmt::println("Cleared Rich Presence: no recent Wii U update on Discord connection");
            }
        })
        .onDisconnected([](int errcode, std::string_view message) {
            fmt::println("Discord: disconnected with error code {} - {}", errcode, message);
        })
        .onErrored([](int errcode, std::string_view message) {
            fmt::println("Discord: error with code {} - {}", errcode, message);
        });
}

// Sets the Rich Presence
void updatePresence(std::string repo, std::string game, std::string full, std::string nnid, int ctrls, std::string jpg, std::string img, time_t start) {
    std::lock_guard lock(presenceMutex);
    auto& rpc = discord::RPCManager::get();

    rpc.getPresence()
        .setName(game)
        .setActivityType(discord::ActivityType::Game)
        .setStatusDisplayType(discord::StatusDisplayType::Name)
        .setState(nnid != "" ? "NID: " + nnid : "")
        .setDetails("Playing on the Wii U")
        .setStartTimestamp(start)
        .setLargeImageKey((jpg == "oh no it didn't work") ? "preview" : ("http://" + repo + "/icons/" + jpg))
        .setLargeImageText(full)
        .setSmallImageKey(img == "backwards" ? "" : img)
        .setSmallImageText(img == "nn" ? "Using Nintendo Network" : "Using Pretendo Network")
        .setPartyID(ctrls > -2 ? "wiiu" : "")
        .setPartySize(ctrls > -2 ? ctrls + 1 : 0)
        .setPartyMax((ctrls + 1 > 4) ? 8 : 4)
        .setPartyPrivacy(discord::PartyPrivacy::Public)
        .setInstance(false)
        .refresh();

    lastPresenceUpdate = std::chrono::steady_clock::now();
    presenceActive = true;
    
    fmt::println("Updated Rich Presence");
}

// Asynchronous function to stop Rich Presence if nothing is recieved
void checkIdle() {
    auto& rpc = discord::RPCManager::get();
	while (runIdleLoop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard lock(presenceMutex);
        if (presenceActive && std::chrono::steady_clock::now() - lastPresenceUpdate >= presenceTimeout) {
            rpc.clearPresence();
            presenceActive = false;
            fmt::println("Cleared Rich Presence: no valid Wii U update for {} seconds", presenceTimeout.count());
        }
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
            updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, out["img"], adjustEpochToUtc(out["time"], out["dst"] == 1));
        }
        else if (out.contains("img")) { // Update 2.0
            updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, out["img"], adjustEpochToUtc(out["time"], false));
        }
        else { // Update 1.9
            updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, "backwards", adjustEpochToUtc(out["time"], false));
        }

        // Check for updates
        if (out.contains("compatibility") && updateMsg > 1) {
            if ((out["compatibility"] > VERSION)) {
                double v = out["compatibility"];
                fmt::println("A new update is available: v{}", v);
                updateMsg = 1;
            }
            else {
                updateMsg = 0;
            }
        }
    }
    catch (...) {
        return -1;
    }

    return 0;
}
