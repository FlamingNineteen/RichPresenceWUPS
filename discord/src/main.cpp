#include <discord-rpc.hpp>
#include <thread>
#include <fmt/format.h>
// #include <resource.h>
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
static bool SendPresence = true;
std::string repo = "flamingnineteen/richpresencewups-db";
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

static void updatePresence(std::string game, std::string nnid, int ctrls, std::string jpg, time_t start) {
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

std::thread tthread(checkIdle);

time_t adjustEpochToUtc(time_t localEpoch) {
	#ifdef _WIN32
		TIME_ZONE_INFORMATION tzInfo;
		DWORD result = GetTimeZoneInformation(&tzInfo);

		// Use only the standard time bias, ignoring daylight saving time
		int bias = tzInfo.Bias;

		// Convert bias from minutes to seconds and adjust the Epoch time
		time_t utcEpoch = localEpoch + (bias * 60);
		return utcEpoch;
	#elif defined(__linux__) || defined(__APPLE__)
		long timezone_offset = timezone;
    	return localEpoch + timezone_offset;
	#endif
}

std::wstring toWstring(const std::string s) {
	std::wstring ws(s.begin(), s.end());
	return ws;
}

std::string fetchRawHtml(std::string server, std::string path) {
	#ifdef _WIN32
		std::wstring wserver = toWstring(server);
		std::wstring wpath = toWstring(path);

		HINTERNET hSession = WinHttpOpen(L"WURP", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
		if (!hSession) return "";
		
		HINTERNET hConnect = WinHttpConnect(hSession, wserver.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
		if (!hConnect) return "";
		
		HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wpath.c_str(),
												NULL, WINHTTP_NO_REFERER,
												WINHTTP_DEFAULT_ACCEPT_TYPES,
												WINHTTP_FLAG_SECURE);
		
		std::string content;
		if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
								WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
			WinHttpReceiveResponse(hRequest, NULL)) {
			
			DWORD dwSize = 0;
			do {
				DWORD dwDownloaded = 0;
				if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;

				if (dwSize > 0) {
					std::vector<char> buffer(dwSize);
					if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
						content.append(buffer.data(), dwDownloaded);
					}
				}
			} while (dwSize > 0);
		}

		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);

		return content;
	#elif defined(__linux__) || defined(__APPLE__)
		CURL* curl = curl_easy_init();
		if (!curl) return "CURL init failed";

		std::string content;
		std::string url = "https://" + server + path;

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		// curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "DiscordWiiU/1.0");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (res != CURLE_OK) {
			return std::string("CURL error: ") + curl_easy_strerror(res);
		}
		return content;
	#endif
	return "";
}

static void gameLoop() {
	std::string msg;

    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fmt::println("WSAStartup failed.");
            return;
        }

        // Create UDP socket
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET) {
			fmt::println("Socket creation failed.");
			WSACleanup();
			return;
		}

        // Bind to all interfaces on the specified port
        sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(UDP_PORT);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            fmt::println("Bind failed.");
            closesocket(sock);
            WSACleanup();
            return;
        }
    #elif defined(__linux__) || defined(__APPLE__)
        boost::asio::io_context io_context;

        // Create UDP socket
        // Bind to all interfaces on the specified port
        udp::socket socket(io_context, udp::endpoint(udp::v4(), UDP_PORT));

        char data[1024];
        udp::endpoint sender_endpoint;
    #endif

    auto& rpc = discord::RPCManager::get();

    json out;
    json images;
    std::string image;
    char buffer[1024];

    std::string fetch = fetchRawHtml("raw.githubusercontent.com", "/" + repo + "/main/titles.json");
	try {
		images = json::parse(fetch);
		fmt::println("Successfully fetched titles.json!");
	} catch (...) {
		fmt::println("Error fetching titles.json. Using default image.");
	}

    do {
        #ifdef _WIN32
            sockaddr_in sender {};
            int senderLen = sizeof(sender);
            int len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            (sockaddr*)&sender, &senderLen);
            if (len == SOCKET_ERROR) {
                fmt::println("recvfrom failed.");
                break;
            }
            buffer[len] = '\0'; // Null-terminate
            std::string msg = buffer;
        #elif defined(__linux__) || defined(__APPLE__)
            size_t length = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
            msg = std::string(data, length);
        #endif
        try {
            out = json::parse(msg);
            if (out["sender"] == "Wii U") {
                
                fmt::println("Received: {}", msg);

                idle = false;
            }

            try {
                image = images[out["long"]];
            } catch (...) {
                image = "oh no it didn't work";
            }

            updatePresence(out["app"], out["nnid"], out["ctrls"], image, adjustEpochToUtc(out["time"]));
        }
        catch (...) {}
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
