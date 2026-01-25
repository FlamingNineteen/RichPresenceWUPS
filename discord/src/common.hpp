#include <discord-rpc.hpp>
#include <fmt/format.h>

constexpr auto APPLICATION_ID = "1353248127469228074";
constexpr int UDP_PORT = 5005;
std::atomic<bool> idle = false;
std::atomic<bool> runIdleLoop = true;

static void discordSetup() {
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

static void updatePresence(std::string repo, std::string game, std::string full, std::string nnid, int ctrls, std::string jpg, std::string img, time_t start) {
    idle = false;
    auto& rpc = discord::RPCManager::get();
    int maxParty = (ctrls > 4) ? 8 : 4;

    rpc.getPresence()
        .setState(game)
        .setActivityType(discord::ActivityType::Game)
        .setStatusDisplayType(discord::StatusDisplayType::State)
        .setDetails((nnid == "") ? "" : "Network ID: " + nnid)
        .setStartTimestamp(start)
        .setLargeImageKey((jpg == "oh no it didn't work") ? "preview" : ("https://raw.githubusercontent.com/" + repo + "/main/icons/" + jpg))
        .setLargeImageText(full)
        .setSmallImageKey(img)
        .setSmallImageText(img == "nn" ? "Using Nintendo Network" : "Using Pretendo Network")
        .setPartyID(ctrls > -2 ? "wiiu" : "")
        .setPartySize(ctrls > -2 ? ctrls + 1 : 0)
        .setPartyMax(maxParty)
        .setPartyPrivacy(discord::PartyPrivacy::Public)
        .setInstance(false)
        .refresh();
    fmt::println("Updated Rich Presence");
}

void checkIdle() {
	bool allow = false;
    auto& rpc = discord::RPCManager::get();
	while (runIdleLoop) {
		std::this_thread::sleep_for(std::chrono::seconds(5));
		if (idle) {
            if (allow) {
                rpc.clearPresence();
                fmt::println("Cleared Rich Presence");
                while (idle && runIdleLoop) {}
            } else {
                allow = true;
            }
        }
        else {
            allow = false;
        }
		idle = true;
	}
	return;
}
