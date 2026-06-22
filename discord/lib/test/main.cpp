#include <discord-rpc.hpp>
#include <iostream>
#include <fmt/format.h>

constexpr auto APPLICATION_ID = "345229890980937739";
static uint32_t FrustrationLevel = 0;
static int64_t StartTime;
static bool SendPresence = true;

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
        })
        .onJoinGame([](std::string_view joinSecret) {
            fmt::println("Discord: join game - {}", joinSecret);
        })
        .onSpectateGame([](std::string_view spectateSecret) {
            fmt::println("Discord: spectate game - {}", spectateSecret);
        })
        .onJoinRequest([](discord::User const& user) {
            fmt::println("Discord: join request from {}#{} - {}", user.username, user.discriminator, user.id);
        });
}

static void updatePresence() {
    auto& rpc = discord::RPCManager::get();
    if (!SendPresence) {
        rpc.clearPresence();
        return;
    }

    rpc.getPresence()
        .setState("West of House")
        .setActivityType(discord::ActivityType::Game)
        .setStatusDisplayType(discord::StatusDisplayType::State)
        .setDetails(fmt::format("Frustration Level: {}", FrustrationLevel))
        .setStartTimestamp(StartTime)
        .setEndTimestamp(time(nullptr) + 5 * 60)
        .setLargeImageKey("canary-large")
        .setSmallImageKey("ptb-small")
        .setPartyID("party1234")
        .setPartySize(1)
        .setPartyMax(6)
        .setPartyPrivacy(discord::PartyPrivacy::Public)
        .setButton1("Click me!", "https://google.com/")
        .setButton2("Dont click me!", "https://www.youtube.com/watch?v=dQw4w9WgXcQ")
        // Buttons conflict with join/spectate secrets, you can't have both
        // .setMatchSecret("xyzzy")
        // .setJoinSecret("join")
        // .setSpectateSecret("look")
        .setInstance(false)
        .refresh();
}

static void gameLoop() {
    auto& rpc = discord::RPCManager::get();

    StartTime = time(nullptr);

    fmt::println("You are standing in an open field west of a white house.");
    do {
        std::string buffer;
        std::getline(std::cin, buffer);

        if (buffer == "h") {
            fmt::println("Commands:");
            fmt::println("  h - display this help message");
            fmt::println("  y - reinit Discord RPC");
            fmt::println("  s - shutdown Discord RPC");
            fmt::println("  c - toggle presence");
            fmt::println("  q - exit the program");
            continue;
        }

        if (buffer == "y") {
            fmt::println("Reinitializing Discord RPC...");
            discord::RPCManager::get().initialize();
            continue;
        }

        if (buffer == "s") {
            fmt::println("Shutting down Discord RPC...");
            discord::RPCManager::get().shutdown();
            continue;
        }

        if (buffer == "c") {
            SendPresence = !SendPresence;
            fmt::println("Presence: {}", SendPresence ? "enabled" : "disabled");
            updatePresence();
            continue;
        }

        if (buffer == "q") {
            fmt::println("Quitting...");
            break;
        }

        if (time(nullptr) & 1) {
            fmt::println("I don't understand that.");
        } else {
            auto space = buffer.find(' ');
            if (space != std::string::npos) {
                buffer = buffer.substr(0, space);
            }
            fmt::println("I don't know the word '{}'.", buffer);
        }

        ++FrustrationLevel;
        updatePresence();
    } while (true);
}

int main() {
    discordSetup();
    discord::RPCManager::get().initialize();

    gameLoop();

    discord::RPCManager::get().shutdown();
    return 0;
}