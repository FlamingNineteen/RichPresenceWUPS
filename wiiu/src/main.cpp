#include <filesystem>
#include <fstream>
#include <malloc.h>
#include <string.h>
#include <thread>

#include <mocha/mocha.h>

#include "config.hpp"
#include "utils.hpp"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("RichPresence");
WUPS_PLUGIN_DESCRIPTION("Discord Rich Presence for the Wii U.");
WUPS_PLUGIN_VERSION(VERSION);
WUPS_PLUGIN_AUTHOR("Flaming19");
WUPS_PLUGIN_LICENSE("GPL");

WUPS_USE_WUT_DEVOPTAB();           // Use the wut devoptabs
WUPS_USE_STORAGE("rich_presence"); // Unique id for the storage api

// Create miscellanious variables
std::jthread tthread;
int elapsed;
std::string app    = "";
std::string preapp = "quantum random!!!11!";

bool INKAY_EXISTS;
std::string INKAY_CONFIG;

// Broadcast over port 5005
void Broadcast(const std::string& json) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) return;

    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    sockaddr_in dest {};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(config.port.value);

    dest.sin_addr.s_addr = inet_addr(config.ip_filter.value ? IpToString(config.ip.value).c_str() : "255.255.255.255");

    sendto(sock, json.c_str(), json.size(), 0, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    close(sock);
}

// Main background loop to broadcast current info
void GameLoop(std::stop_token stoken) {
    int ctrls;
    std::string details;
    std::string nnid;
    std::string json;

    while (!stoken.stop_requested() && config.enabled.value) {
        if (app != "") {
            // Get controller count
            switch (config.ctrl.value) {
                case CTRLCOUNT:
                    ctrls = GetCtrlNum();
                    break;
                case CTRLCOUNTNODRC:
                    ctrls = GetCtrlNum()-1;
                    break;
                default:
                    ctrls = -2;
            }

            // Get Network ID
            nnid = config.net_id.value ? GetNetworkId() : "";

            if (ReplaceSlashN(GetXmlTag("longname_en")) == "Super Smash Bros. for Wii U") {
                details = std::to_string(ReadFromMemory(0x1098B2AB)>>24) + " | " + std::to_string(ReadFromMemory(0x1098EDEB)>>24);
                // details = DecToHex(ReadFromMemory(0x1098B2AB)>>24) + " | " + DecToHex(ReadFromMemory(0x1098EDEB)>>24);
            }

            // Prepare and send json
            json = "{\"sender\":\"Wii U\",\"long\":\"" + ReplaceSlashN(GetAppTitle(ENGLISH, true)) + "\",\"app\":\"" + app + "\",\"details\":\"" + details + "\",\"time\":" + std::to_string(elapsed + (config.timeset.value * 3600)) + ",\"ctrls\":" + std::to_string(ctrls) + ",\"nnid\":\"" + nnid + "\",\"img\":\"" + (config.small_img.value ? GetNetwork(INKAY_EXISTS, INKAY_CONFIG) : "") + "\",\"dst\":" + std::to_string(config.dst.value) + ",\"compatibility\":" + std::to_string(COMPATIBLE_VERSION) + "}";
            Broadcast(json);
        }

        // Five second interval
        for (int i=0; i<5 && !stoken.stop_requested() && config.enabled.value; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    return;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();

    app = GetXmlTag("shortname_en") == "Health and Safety Information" ? "Homebrew Application" : ReplaceSlashN(GetAppTitle(config.lang.value, config.title.value));
    preapp = app;

    if (tthread.joinable()) {
        tthread.request_stop();
        tthread.join(); // Wait for thread to finish before starting a new one
    }

    if ((config.enabled.value && !(config.cod.value && app.find("Call of Duty") != std::string::npos))) {
        tthread = std::jthread(GameLoop);
    }
}

INITIALIZE_PLUGIN() {
    Mocha_InitLibrary();

    WUPSConfigAPIOptionsV1 configOptions = {.name = "Rich Presence"};
    WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback);
    WUPSStorageAPI::GetOrStoreDefault(config.enabled.id, config.enabled.value, config.enabled.def);
    WUPSStorageAPI::GetOrStoreDefault(config.net_id.id, config.net_id.value, config.net_id.def);
    WUPSStorageAPI::GetOrStoreDefault(config.timeset.id, config.timeset.value, config.timeset.def);
    WUPSStorageAPI::GetOrStoreDefault(config.ctrl.id, config.ctrl.value, config.ctrl.def);
    WUPSStorageAPI::GetOrStoreDefault(config.small_img.id, config.small_img.value, config.small_img.def);
    WUPSStorageAPI::GetOrStoreDefault(config.dst.id, config.dst.value, config.dst.def);
    WUPSStorageAPI::GetOrStoreDefault(config.title.id, config.title.value, config.title.def);
    WUPSStorageAPI::GetOrStoreDefault(config.lang.id, config.lang.value, config.lang.def);
    WUPSStorageAPI::GetOrStoreDefault(config.ip.id, config.ip.value, config.ip.def);
    WUPSStorageAPI::GetOrStoreDefault(config.ip_filter.id, config.ip_filter.value, config.ip_filter.def);
    WUPSStorageAPI::GetOrStoreDefault(config.port.id, config.port.value, config.port.def);
    WUPSStorageAPI::GetOrStoreDefault(config.cod.id, config.cod.value, config.cod.def);
    WUPSStorageAPI::SaveStorage();

    char environment_path_buffer[0x100];
    Mocha_GetEnvironmentPath(environment_path_buffer, sizeof(environment_path_buffer));
    INKAY_CONFIG = std::string(environment_path_buffer) + std::string("/plugins/config/inkay.json");
    INKAY_EXISTS = std::filesystem::exists(INKAY_CONFIG);
}

ON_APPLICATION_START() {
    app = GetXmlTag("shortname_en") == "Health and Safety Information" ? "Homebrew Application" : ReplaceSlashN(GetAppTitle(config.lang.value, config.title.value)); 

    if (app != preapp) elapsed = time(NULL); // Only update elapsed time if app changed
    preapp = app;

    if (tthread.joinable()) {
        tthread.request_stop();
        tthread.join(); // Wait for thread to finish before starting a new one
    }
    if (config.enabled.value && !(config.cod.value && app.find("Call of Duty") != std::string::npos)) tthread = std::jthread(GameLoop);
}

ON_APPLICATION_REQUESTS_EXIT() {    
    if (tthread.joinable()) {
        tthread.request_stop();
        tthread.join(); // Wait for thread to finish
    }
}

DEINITIALIZE_PLUGIN() {
    if (tthread.joinable()) {
        tthread.request_stop();
        tthread.join(); // Wait for thread to finish
    }

    Mocha_DeInitLibrary();
}
