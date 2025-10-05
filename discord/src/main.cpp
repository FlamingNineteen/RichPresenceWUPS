#include <discord-rpc.hpp>
#include <iostream>
#include <fmt/format.h>
#include <string>

#include "json.hpp"
using json = nlohmann::json;

#if defined(__linux__) || defined(__APPLE__)
	#include <boost/asio.hpp>
	#include <boost/beast/core.hpp>
	#include <boost/beast/http.hpp>
	#include <boost/beast/ssl.hpp>
	#include <boost/asio/connect.hpp>
	#include <boost/asio/ip/tcp.hpp>
	#include <boost/asio/ssl/error.hpp>
	#include <boost/asio/ssl/stream.hpp>
	#include <curl/curl.h>
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace ssl = net::ssl;
	using tcp = net::ip::tcp;
	using boost::asio::ip::udp;
#endif

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#pragma comment(lib, "ws2_32.lib")
	#pragma comment(lib, "winhttp.lib")
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <winhttp.h>
#elif __linux__
	#include <unistd.h>
#elif __APPLE__
	#include <mach-o/dyld.h>
#endif

constexpr auto APPLICATION_ID = "1353248127469228074";
constexpr int UDP_PORT = 5005;
static uint32_t FrustrationLevel = 0;
static int64_t StartTime;
static bool SendPresence = true;
std::string repo = "flamingnineteen/richpresence-db";
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

static void updatePresence(std::string game, std::string nnid, int ctrls, std::string image) {
    auto& rpc = discord::RPCManager::get();
    int maxParty = (ctrls > 4) ? 8 : 4;
    if (!SendPresence) {
        rpc.clearPresence();
        return;
    }

    rpc.getPresence()
        .setState(game)
        .setActivityType(discord::ActivityType::Game)
        .setStatusDisplayType(discord::StatusDisplayType::State)
        .setDetails(fmt::format("Network ID: {}", nnid))
        .setStartTimestamp(StartTime)
        .setEndTimestamp(time(nullptr) + 5 * 60)
        .setLargeImageKey("https://raw.githubusercontent.com/" + repo + "/refs/heads/main/icons/nintendoland.jpg")
        .setPartyID("party1234")
        .setPartySize(ctrls)
        .setPartyMax(maxParty)
        .setPartyPrivacy(discord::PartyPrivacy::Public)
        .setInstance(false)
        .refresh();
}

void CheckIdle() {
	bool allow = true;
    auto& rpc = discord::RPCManager::get();
	while (runIdleLoop) {
		std::this_thread::sleep_for(std::chrono::seconds(8));
		if (idle) {
            if (allow) {
                if (!SendPresence) {
                    rpc.clearPresence();
                }
            }
        }
        else {
            allow = true;
        }
		idle = true;
	}
	return;
}

std::thread tthread(CheckIdle);

static void gameLoop() {
	std::string msg;

    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed.\n";
            return;
        }

        // Create UDP socket
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET) {
			std::cerr << "Socket creation failed.\n";
			WSACleanup();
			return;
		}

        // Bind to all interfaces on the specified port
        sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(UDP_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed.\n";
            closesocket(sock);
            WSACleanup();
            return;
        }
    #elif defined(__linux__) || defined(__APPLE__)
        boost::asio::io_context io_context;

        // Create UDP socket
        // Bind to all interfaces on the specified port
        udp::socket socket(io_context, udp::endpoint(udp::v4(), PORT));

        char data[1024];
        udp::endpoint sender_endpoint;
    #endif

    auto& rpc = discord::RPCManager::get();

    StartTime = time(nullptr);

    json out;
    json images;
    char buffer[1024];

    fmt::println("You are standing in an open field west of a white house.");
    do {
        #ifdef _WIN32
            sockaddr_in sender {};
            int senderLen = sizeof(sender);
            int len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            (sockaddr*)&sender, &senderLen);
            if (len == SOCKET_ERROR) {
                std::cerr << "recvfrom failed.\n";
                break;
            }
            buffer[len] = '\0'; // Null-terminate
            std::string msg = buffer;
        #elif defined(__linux__) || defined(__APPLE__)
            size_t length = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
            msg = std::string(data, length);
        #endif

        out = json::parse(msg);
        if (out["sender"] == "Wii U") {
            std::cout << "Received: " << buffer << std::endl;

            idle = false;
        }
        updatePresence(out["app"], "Flaming19", out["ctrls"], out["long"]);
    } while (true);

    #ifdef _WIN32
		closesocket(sock);
		WSACleanup();
	#endif

    return;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        int i = 1;
        while (i < argc) {
            // More commands can be added here in the future
            fmt::println("{}", argv[i]);
            if (std::strcmp(argv[i], "repo") == 0) {
                i++;
                repo = argv[i];
                fmt::println("Using repository {}.", repo);
            }
            i++;
        }
    }
    discordSetup();
    discord::RPCManager::get().initialize();

    gameLoop();

    runIdleLoop = false;
	if (tthread.joinable()) {
		tthread.join();
	}

    discord::RPCManager::get().shutdown();
    return 0;
}