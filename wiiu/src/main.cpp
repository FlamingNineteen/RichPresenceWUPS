#include <atomic>
#include <chrono>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <thread>
#include <unistd.h>

#include <wups.h>
#include <wups/config.h>

#include <wut.h>

#include <coreinit/cache.h>
#include <coreinit/title.h>
#include <coreinit/time.h>
#include <coreinit/thread.h>
#include <coreinit/memdefaultheap.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

// #include <padscore/wpad.h>
// #include <padscore/kpad.h>

#include <nn/acp/client.h>
#include <nn/acp/title.h>

#define STACK_SIZE 0x2000

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("RichPresence");
WUPS_PLUGIN_DESCRIPTION("Discord Rich Presence for the Wii U.");
WUPS_PLUGIN_VERSION("v1.5");
WUPS_PLUGIN_AUTHOR("Flaming19");
WUPS_PLUGIN_LICENSE("GNU");

WUPS_USE_WUT_DEVOPTAB();           // Use the wut devoptabs
WUPS_USE_STORAGE("rich_presence"); // Unique id for the storage api

// Settings
struct WURP_CONFIG {
    int time;
    bool enabled;
    bool notify;
};

// Global variables
std::string app = "";
std::jthread tthread;

std::string GetNameOfCurrentApplication() {
    std::string result;
    ACPInitialize();
    auto *metaXml = (ACPMetaXml *) memalign(0x40, sizeof(ACPMetaXml));
    if (ACPGetTitleMetaXml(OSGetTitleID(), metaXml) == ACP_RESULT_SUCCESS) {
        result = metaXml->shortname_en;
    } else {
        result.clear();
    }
    ACPFinalize();
    return result;
}

// Broadcast over TCP 5000
void Broadcast(const std::string& json) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) return;

    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    sockaddr_in dest {};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(5005);
    dest.sin_addr.s_addr = inet_addr("255.255.255.255");

    sendto(sock, json.c_str(), json.size(), 0, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    close(sock);
}

void GameLoop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        Broadcast("LOOP"); // Debug
        if (app != "") {
            std::string json = "\"app\":" + app + ",\"time\":" + std::to_string(time(NULL));
            Broadcast(json);
        }
        // Five second interval
        for (int i=0; i<1000 && !stoken.stop_requested(); i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    Broadcast("QUIT"); // Debug
}

INITIALIZE_PLUGIN() {
    Broadcast("INIT PLUGIN"); // Debug
    WURP_CONFIG settings;
    WUPSStorageAPI::Get("time", settings.time);
    WUPSStorageAPI::Get("enabled", settings.enabled);
}

ON_APPLICATION_START() {
    Broadcast("APP START"); // Debug
    app = GetNameOfCurrentApplication();
    if (GetNameOfCurrentApplication() == "Health and Safety Information") {
        app = "Homebrew Application";
    }
    Broadcast("APP SET " + app); // Debug
    if (tthread.joinable()) tthread.request_stop();
    tthread = std::jthread(GameLoop);
}

ON_APPLICATION_REQUESTS_EXIT() {
    Broadcast("APP REQ EXIT"); // Debug
    if (tthread.joinable()) tthread.request_stop();
}

DEINITIALIZE_PLUGIN() {
    Broadcast("DEINIT PLUGIN"); // Debug
    if (tthread.joinable()) tthread.request_stop();
}