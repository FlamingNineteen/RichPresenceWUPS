#define APP_ID 1353248127469228074
#define UDP_PORT 5005
#define DISCORDPP_IMPLEMENTATION
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <string>
#include <csignal>
#include <cstdlib>
#include <limits>
#include <fstream>
#include <future>
#include <ctime>

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

#include "discordpp.h"
#include "json.hpp"
using json = nlohmann::json;

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

// Replace with your Discord Application ID
const uint64_t APPLICATION_ID = APP_ID;

// Create a flag to stop the application
std::atomic<bool> running = true;

// Clears rich presence if idle
std::atomic<bool> idle = false;
std::atomic<bool> runIdleLoop = false;

// Signal handler to stop the application
void signalHandler(int signum) {
	running.store(false);
}

// Callback function to write data to a string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t totalSize = size * nmemb;
	userp->append((char*)contents, totalSize);
	return totalSize;
}

size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t totalSize = size * nmemb;
	userp->append((char*)contents, totalSize);
	return totalSize;
}

std::string GetAppDir() {
	#ifdef _WIN32
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::string fullPath(buffer);
		return fullPath.substr(0, fullPath.find_last_of("\\/"));
	#elif __linux__
		char buffer[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
		if (count == -1) {
			perror("readlink");
			return "";
		}
		std::string path = std::string(buffer, count);
		size_t pos = path.find_last_of('/');
		return (pos != std::string::npos) ? path.substr(0, pos) : "";
	#elif __APPLE__
		char buffer[PATH_MAX];
		uint32_t size = sizeof(buffer);
		if (_NSGetExecutablePath(buffer, &size) != 0) {
			std::cerr << "Buffer too small; needed size: " << size << std::endl;
		return "";
		}
		std::string path = std::string(buffer);
		size_t pos = path.find_last_of('/');
		return (pos != std::string::npos) ? path.substr(0, pos) : "";
	#endif
	return "";
}

void save(json fjs, std::string dir) {
	std::ofstream ofs(dir + "/save.json");
	ofs << fjs.dump(4);
	ofs.close();
}

void debug(int d, int s = 0) {
	std::cout << "\nStep " << std::to_string(d);
	std::this_thread::sleep_for(std::chrono::seconds(s));
}

std::wstring to_wstring(const std::string s) {
	std::wstring ws(s.begin(), s.end());
	return ws;
}

bool WaitForDiscordReady(std::shared_ptr<discordpp::Client> client, int timeoutSeconds = 10) {
	std::promise<bool> readyPromise;
	auto readyFuture = readyPromise.get_future();
	auto callback = [&readyPromise](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) {
		if (status == discordpp::Client::Status::Ready) {
			readyPromise.set_value(true);
		} else if (error != discordpp::Client::Error::None) {
			readyPromise.set_value(false);
		}
	};
	client->SetStatusChangedCallback(callback);

	// Wait for Ready or Error
	auto start = std::chrono::steady_clock::now();
	while (readyFuture.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
		discordpp::RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		if (std::chrono::steady_clock::now() - start > std::chrono::seconds(timeoutSeconds)) {
			return false; // Timeout
		}
	}
	return readyFuture.get();
}

bool SyncUpdateToken(std::shared_ptr<discordpp::Client> client, discordpp::AuthorizationTokenType tokenType, const std::string& accessToken) {
	std::promise<bool> promise;
	auto future = promise.get_future();
	client->UpdateToken(tokenType, accessToken, [client, &promise](discordpp::ClientResult result) {
		promise.set_value(result.Successful());
	});
	// Run event loop until done
	while (future.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
		discordpp::RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return future.get();
}

struct RefreshResult {
	bool success;
	std::string accessToken;
	std::string refreshToken;
	discordpp::AuthorizationTokenType tokenType;
	int32_t expiresIn;
	std::string scope;
};

RefreshResult SyncRefreshToken(std::shared_ptr<discordpp::Client> client, const std::string& refreshToken) {
	std::promise<RefreshResult> promise;
	auto future = promise.get_future();
	client->RefreshToken(APPLICATION_ID, refreshToken,
		[client, &promise](discordpp::ClientResult result, std::string accessToken, std::string refreshToken, discordpp::AuthorizationTokenType tokenType, int32_t expiresIn, std::string scope) {
			promise.set_value({result.Successful(), accessToken, refreshToken, tokenType, expiresIn, scope});
		}
	);
	// Run event loop until done
	while (future.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
		discordpp::RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return future.get();
}

void CheckIdle(std::shared_ptr<discordpp::Client> client) {
	bool allow = true;
	while (runIdleLoop) {
		std::this_thread::sleep_for(std::chrono::seconds(8));
		if (idle) {if (allow) {client->ClearRichPresence();}}
		else {allow = true;};
		idle = true;
	}
	return;
}

bool Authorize(std::shared_ptr<discordpp::Client> client, json &fjs) {

	// Generate OAuth2 code verifier for authentication
	std::promise<bool> authPromise;
	auto authFuture = authPromise.get_future();
	auto codeVerifier = client->CreateAuthorizationCodeVerifier();

	// Set up authentication arguments
	discordpp::AuthorizationArgs args{};
	args.SetClientId(APPLICATION_ID);
	args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
	args.SetCodeChallenge(codeVerifier.Challenge());

	// Begin authentication process
	client->Authorize(args, [client, codeVerifier, &authPromise, &fjs](auto result, auto code, auto redirectUri) {
		if (!result.Successful()) {
			std::cerr << "Authentication Error: " << result.Error() << std::endl;
			authPromise.set_value(false);
		} else {
			std::cout << "Authorization successful! Getting access token...\n";
			client->GetToken(APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
				[client, &authPromise, &fjs](discordpp::ClientResult result,
				std::string accessToken,
				std::string refreshToken,
				discordpp::AuthorizationTokenType tokenType,
				int32_t expiresIn,
				std::string scope) {
					if(result.Successful()) {
						std::cout << "Access token received! Establishing connection...\n";
						fjs["cache"]["accessToken"]  = accessToken;
						fjs["cache"]["refreshToken"] = refreshToken;
						fjs["cache"]["expiresIn"]    = expiresIn;
						fjs["cache"]["scope"]        = scope;
						client->UpdateToken(tokenType, accessToken, [client, &authPromise](discordpp::ClientResult result) {
							if(result.Successful()) {
								std::cout << "Token updated, connecting to Discord...\n";
								client->Connect();
								authPromise.set_value(true);
							} else {
								authPromise.set_value(false);
							}
						});
					} else {
						authPromise.set_value(false);
					}
				}
			);
		}
	});

	// Wait for the async process to finish
	while (authFuture.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
		discordpp::RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return authFuture.get();
}

time_t AdjustEpochToUTC(time_t localEpoch) {
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

std::string FetchRawHtml(std::string server, std::string path) {
	#ifdef _WIN32
		std::wstring wserver = to_wstring(server);
		std::wstring wpath = to_wstring(path);

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
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
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

int main() {
	json fjs;
	std::string dir  = GetAppDir();
	std::ifstream fif(dir + "/save.json");
	bool auth        = true;
	bool logging     = false;
	std::string repo = "flamingnineteen/RichPresenceWUPS-DB";

	// Get JSON Info
	std::cout << "Retrieving data...\n";
	if (fif.good()) {
		try {
			fjs = json::parse(fif);
			auth = fjs["auth"];
			logging = fjs["logging"];
			if (fjs["repo"] != "") repo = fjs["repo"];
			std::cout << "Using repository " << repo << std::endl;

		} catch (const std::exception& e) {
			std::cerr << "Failed to parse save.json: " << e.what() << "\n" << std::endl;
			fjs = json::parse(R"({"auth":true,"logging":false,"repo":"","cache":{"accessToken":"","refreshToken":"","expiresIn":0,"scope":""}})");
			save(fjs, dir);
		}
	} else {
		fjs = json::parse(R"({"auth":true,"logging":false,"repo":"","cache":{"accessToken":"","refreshToken":"","expiresIn":0,"scope":""}})");
		save(fjs, dir);
	}

	std::signal(SIGINT, signalHandler);
	std::cout << "Initializing Discord SDK...\n";

	// Create our Discord Client
	auto client = std::make_shared<discordpp::Client>();

	// Set up logging callback
	if (logging) {
		client->AddLogCallback([](auto msg, auto severity) {
			std::cout << "[" << EnumToString(severity) << "] " << msg << std::endl;
		}, discordpp::LoggingSeverity::Info);
	}

	// Set up status callback to monitor client connection
	client->SetStatusChangedCallback([client](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) {
		if (status == discordpp::Client::Status::Ready) {
			std::cout << "Client is ready.\n";
		} else if (error != discordpp::Client::Error::None) {
			std::cerr << "Connection Error: " << discordpp::Client::ErrorToString(error) << " - Details: " << errorDetail << std::endl;
		} else {
			std::cout << "Status changed: " << discordpp::Client::StatusToString(status) << std::endl;
		}
	});
	
	// Authorize
	do {
		if (auth) {
			auth = !Authorize(client, fjs);
		} else {
			std::this_thread::sleep_for(std::chrono::seconds(5));
			std::string accessToken                     = fjs["cache"]["accessToken"];
			std::string refreshToken                    = fjs["cache"]["refreshToken"];
			discordpp::AuthorizationTokenType tokenType = discordpp::AuthorizationTokenType::Bearer;
			int32_t expiresIn                           = fjs["cache"]["expiresIn"];
			std::string scope                           = fjs["cache"]["scope"];
			int step                                    = 0;

			if (SyncUpdateToken(client, tokenType, accessToken)) {
				client->Connect();
				if (WaitForDiscordReady(client)) {
					std::cout << "Successfully authenticated with cached token.\n";
					auth = false;
				} else {
					std::cout << "Failed to authenticate with cached token. Refreshing...\n";
					auto refreshResult = SyncRefreshToken(client, refreshToken);
					if (refreshResult.success) {
						std::cout << "Refreshed token.\n";
						fjs["cache"]["accessToken"] = refreshResult.accessToken;
						accessToken = refreshResult.accessToken;
						if (SyncUpdateToken(client, tokenType, accessToken)) {
							client->Connect();
							if (WaitForDiscordReady(client)) {
								std::cout << "Successfully authenticated with refreshed token.\n";
								auth = false;
							} else {
								std::cout << "Refreshed token is invalid.\n";
								auth = true;
							}
						}
					} else {
						std::cout << "Failed to refresh token.\n";
					}
				}
			}
			if (auth) {
				auth = !Authorize(client, fjs);
			}
		}

		if (auth) {
			std::string s;
			std::cout << "Failed to authenticate. Press Enter to try again.\n";
			std::getline(std::cin, s);
		}

		fjs["auth"] = auth;
		save(fjs, dir);
	} while (auth);

	json out;
	json images;
	char buffer[1024];
	const int PORT         = UDP_PORT;
	std::string players[8] = {"Singleplayer","Local 2-Player","Local 3-Player","Local 4-Player","Local 5-Player","Local 6-Player","Local 7-Player","Local 8-Player"};

	std::cout << "Fetching titles.json..." << std::endl;
	std::string fetch = FetchRawHtml("raw.githubusercontent.com", "/" + repo + "/main/titles.json");
	try {
		images = json::parse(fetch);
		std::cout << "Successfully fetched titles.json!" << std::endl;
	} catch (...) {
		std::cout << "Error fetching titles.json. Using default image." << std::endl;
	}

	// Keep application running to allow SDK to receive events and callbacks
	#ifdef _WIN32
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			std::cerr << "WSAStartup failed.\n";
			return 1;
		}

		// Create UDP socket
		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET) {
			std::cerr << "Socket creation failed.\n";
			WSACleanup();
			return 1;
		}

		// Bind to all interfaces on the specified port
		sockaddr_in addr {};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(PORT);
		addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
			std::cerr << "Bind failed.\n";
			closesocket(sock);
			WSACleanup();
			return 1;
		}
	#elif defined(__linux__) || defined(__APPLE__)
		boost::asio::io_context io_context;

		// Create UDP socket
		// Bind to all interfaces on the specified port
        udp::socket socket(io_context, udp::endpoint(udp::v4(), PORT));

        char data[1024];
        udp::endpoint sender_endpoint;
	#endif

	runIdleLoop = true;
	std::thread tthread(CheckIdle, client);
	std::string msg;

	std::cout << "Listening for Wii U broadcasts on port " << PORT << "...\n";

	while (true) {
		discordpp::RunCallbacks();

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
		
		try {
			out = json::parse(msg);
			if (out["sender"] == "Wii U") {
				std::cout << "Received: " << buffer << std::endl;

				idle = false;

				// Configure rich presence details
				discordpp::Activity activity;
				activity.SetType(discordpp::ActivityTypes::Playing);
				activity.SetDetails(out["app"]);
				int ctrl = out["ctrls"];
				activity.SetState(players[ctrl]);
				discordpp::ActivityAssets assets;
				try {
					std::string temp = images[out["long"]];
					assets.SetLargeImage("https://raw.githubusercontent.com/" + repo + "/refs/heads/main/icons/" + temp);
				} catch (...) {
					assets.SetLargeImage("preview");
				}
				activity.SetAssets(assets);
				discordpp::ActivityTimestamps timestamps;
				timestamps.SetStart(AdjustEpochToUTC(out["time"]));
				activity.SetTimestamps(timestamps);

				// Update rich presence
				client->UpdateRichPresence(activity, [](discordpp::ClientResult result) {
					if (result.Successful()) std::cout << "Rich Presence updated successfully!\n";
					else std::cout << "Rich Presence update failed\n";
				});
			}
		}
		catch (...) {std::cout << "Json parse error" << std::endl;}
	}

	#ifdef _WIN32
		closesocket(sock);
		WSACleanup();
	#endif
	if (tthread.joinable()) {
		tthread.join();
	}
	return 0;
}
