#include <string>
#include <vector>

#include "common.hpp"

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>

#include "json.hpp"
using json = nlohmann::json;

time_t adjustEpochToUtc(time_t localEpoch) {
    TIME_ZONE_INFORMATION tzInfo;
    DWORD result = GetTimeZoneInformation(&tzInfo);

    // Use only the standard time bias, ignoring daylight saving time
    int bias = tzInfo.Bias;

    // Convert bias from minutes to seconds and adjust the Epoch time
    time_t utcEpoch = localEpoch + (bias * 60);
    return utcEpoch;
}

std::wstring toWstring(const std::string s) {
	std::wstring ws(s.begin(), s.end());
	return ws;
}

std::string fetchRawHtml(std::string server, std::string path) {
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
}

json getImageKeys(std::string repo) {
    json images;

    std::string fetch = fetchRawHtml("raw.githubusercontent.com", "/" + repo + "/main/titles.json");
	try {
		images = json::parse(fetch);
		fmt::println("Successfully fetched titles.json!");
	} catch (...) {
		fmt::println("Error fetching titles.json. Using default image.");
	}

    return images;
}

bool bind(SOCKET &sock) {
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
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fmt::println("Bind failed.");
        closesocket(sock);
        WSACleanup();
        return false;
    }

    return true;
}

static void gameLoop(std::string repo) {
	std::string msg;

    SOCKET sock;
    while(!bind(sock)) {
        fmt::println("Retrying...");
    };
    fmt::println("Successfully binded to UDP port 5005");

    auto& rpc = discord::RPCManager::get();

    json out;
    json images = getImageKeys(repo);
    std::string image;
    char buffer[1024];

    do {
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

            updatePresence(repo, out["app"], out["long"], out["nnid"], out["ctrls"], image, out["img"], adjustEpochToUtc(out["time"]));
        }
        catch (...) {}
    } while (true);

    closesocket(sock);
    WSACleanup();

    return;
}
