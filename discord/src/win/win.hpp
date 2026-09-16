#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>

#include "../common.hpp"

// Change the recieved time elapsed to epoch
time_t adjustEpochToUtc(time_t localEpoch, bool dst = false) {
    TIME_ZONE_INFORMATION tzInfo;
    DWORD result = GetTimeZoneInformation(&tzInfo);

    // Get either standard time bias or daylight savings time bias
    int bias = tzInfo.Bias - dst * 60;

    // Convert bias from minutes to seconds and adjust the Epoch time
    time_t utcEpoch = localEpoch + (bias * 60);
    return utcEpoch;
}

// Converts a string to a wide string
std::wstring toWstring(const std::string s) {
	std::wstring ws(s.begin(), s.end());
	return ws;
}

// Fetches data with html
std::string fetchRawHtml(std::string server, std::string path) {
    std::wstring wserver = toWstring(server);
    std::wstring wpath = toWstring(path);

    HINTERNET hSession = WinHttpOpen(L"WiiURichPresence/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
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
}

// Fetch the image keys from the repository
json getImageKeys(std::string repo) {
    json images;

    std::string fetch = fetchRawHtml(repo.substr(0, repo.find("/")), repo.substr(repo.find("/")) + "/titles.json");
	try {
		images = json::parse(fetch);
		fmt::println("Successfully fetched titles.json!");
	} catch (...) {
		fmt::println("Error fetching titles.json. Using default image.");
	}

    return images;
}

// Bind to a UDP socket
bool bind(SOCKET &sock, uint16_t port = 5005) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    // Create UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fmt::println("Socket creation failed.");
        WSACleanup();
        return false;
    }

    // Bind to all interfaces on the specified port
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fmt::println("Failed to bind to UDP port. Is another program using it? Retrying...");
        closesocket(sock);
        WSACleanup();
        return false;
    }

    return true;
}

// Main loop
void gameLoop(std::string repo, uint16_t port) {
    // Bind the socket
	std::string msg;
    SOCKET sock;
    while(!bind(sock, port)) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    };
    fmt::println("Successfully binded to port");

    auto& rpc = discord::RPCManager::get();

    json out;
    json images = getImageKeys(repo);
    std::string image;
    char buffer[1024];

    do {
        // Wait for a message
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

        // Attempt to set Rich Presence
        if (parseJsonAndUpdate(msg, images, repo, adjustEpochToUtc) < 0) {
            fmt::println("Failed to update Rich Presence");
        }
    } while (true);

    closesocket(sock);
    WSACleanup();

    return;
}


void SetConsole() {
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
}
