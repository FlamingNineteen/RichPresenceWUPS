#include <filesystem>
#include <fstream>
#include <malloc.h>
#include <string.h>
#include <thread>

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

#define STACK_SIZE 0x2000

/**
    All of these defines can be used in ANY file.
    It's possible to split it up into multiple files.
**/

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
    dest.sin_port = htons(configPort);
    dest.sin_addr.s_addr = inet_addr("255.255.255.255");

    sendto(sock, json.c_str(), json.size(), 0, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    close(sock);
}

// Main background loop to broadcast current info
void GameLoop(std::stop_token stoken) {
    int ctrls;
    std::string nnid;
    std::string json;

    while (!stoken.stop_requested() && configEnabled) {
        if (app != "") {
            // Get controller count
            switch (configCtrl) {
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
            nnid = configNetId ? GetNetworkId() : "";

            // Prepare and send json
            json = "{\"sender\":\"Wii U\",\"long\":\"" + ReplaceSlashN(GetAppTitle(ENGLISH, true)) + "\",\"app\":\"" + app + "\",\"time\":" + std::to_string(elapsed + (configTimeset * 3600)) + ",\"ctrls\":" + std::to_string(ctrls) + ",\"nnid\":\"" + nnid + "\",\"img\":\"" + (configSmallImg ? GetNetwork(INKAY_EXISTS, INKAY_CONFIG) : "") + "\",\"dst\":" + std::to_string(configDst) + "}";
            Broadcast(json);
        }

        // Five second interval
        for (int i=0; i<1000 && !stoken.stop_requested() && configEnabled; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    return;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();

    app = GetXmlTag("shortname_en") == "Health and Safety Information" ? "Homebrew Application" : ReplaceSlashN(GetAppTitle(configLang, configTitle));
    preapp = app;

    if (tthread.joinable()) {
        tthread.request_stop();
        tthread.join(); // Wait for thread to finish before starting a new one
    }

    if ((configEnabled && !(configCod && app.find("Call of Duty") != std::string::npos))) {
        tthread = std::jthread(GameLoop);
    }
}

INITIALIZE_PLUGIN() {
    Mocha_InitLibrary();

    WUPSConfigAPIOptionsV1 configOptions = {.name = "Rich Presence"};
    WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_ENABLED_CONFIG_ID, configEnabled, CONFIG_ENABLED_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_NET_ID_CONFIG_ID, configNetId, CONFIG_NET_ID_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_TIMESET_CONFIG_ID, configTimeset, CONFIG_TIMESET_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_CTRL_CONFIG_ID, configCtrl, CONFIG_CTRL_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_SMALL_IMG_CONFIG_ID, configSmallImg, CONFIG_SMALL_IMG_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_DST_CONFIG_ID, configDst, CONFIG_DST_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_TITLE_CONFIG_ID, configTitle, CONFIG_TITLE_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_LANG_CONFIG_ID, configLang, CONFIG_LANG_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_PORT_CONFIG_ID, configPort, CONFIG_PORT_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_COD_CONFIG_ID, configCod, CONFIG_COD_DEFAULT_VALUE);
    WUPSStorageAPI::SaveStorage();

    char environment_path_buffer[0x100];
    Mocha_GetEnvironmentPath(environment_path_buffer, sizeof(environment_path_buffer));
    INKAY_CONFIG = std::string(environment_path_buffer) + std::string("/plugins/config/inkay.json");
    INKAY_EXISTS = std::filesystem::exists(INKAY_CONFIG);
}

ON_APPLICATION_START() {
    app = GetXmlTag("shortname_en") == "Health and Safety Information" ? "Homebrew Application" : ReplaceSlashN(GetAppTitle(configLang, configTitle)); 

    if (app != preapp) elapsed = time(NULL); // Only update elapsed time if app changed
    preapp = app;

    if (tthread.joinable()) {
        tthread.request_stop();
        tthread.join(); // Wait for thread to finish before starting a new one
    }
    if (configEnabled && !(configCod && app.find("Call of Duty") != std::string::npos)) tthread = std::jthread(GameLoop);
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
