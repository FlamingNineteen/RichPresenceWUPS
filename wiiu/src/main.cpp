#include <atomic>
#include <chrono>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <thread>
#include <unistd.h>

#include <wut.h>

#include <wups.h>
#include <wups/config.h>
#include <wups/config/WUPSConfigCategory.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/config_api.h>

#include <coreinit/title.h>
#include <coreinit/time.h>
#include <coreinit/thread.h>

#include <arpa/inet.h>

#include <padscore/wpad.h>
#include <padscore/kpad.h>

#include <nn/acp/client.h>
#include <nn/acp/title.h>

// #include <nn/acp/client.h>
// #include <nn/acp/title.h>

#define STACK_SIZE 0x2000

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("RichPresence");
WUPS_PLUGIN_DESCRIPTION("Discord Rich Presence for the Wii U.");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("Flaming19");
WUPS_PLUGIN_LICENSE("GNU");

WUPS_USE_WUT_DEVOPTAB();           // Use the wut devoptabs
WUPS_USE_STORAGE("rich_presence"); // Unique id for the storage api

// Settings
struct WURPConfig {
    int timeSet;
    bool enabled;
};

// Global variables
std::jthread tthread;
int elapsed;
WURPConfig settings;
std::string app      = "";
std::string preapp   = "quantum random!!!11!";
WPADChan channels[7] = {WPAD_CHAN_0, WPAD_CHAN_1, WPAD_CHAN_2, WPAD_CHAN_3, WPAD_CHAN_4, WPAD_CHAN_5, WPAD_CHAN_6};

int ctrlNum() {
    WPADExtensionType extType;
    int c = 0;
    for (int i = 0; i < 7; i++) {
        int32_t result = WPADProbe(channels[i], &extType);
        if (result != -1) {c++;}
    }
    return c;
}

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

// Broadcast over port 5005
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
        if (app != "") {
            int ctrls = ctrlNum();
            std::string json = "{\"app\":\"" + app + "\",\"time\":" + std::to_string(elapsed) + ",\"ctrls\":" + std::to_string(ctrls) + "}";
            Broadcast(json);
        }

        // Five second interval
        for (int i=0; i<1000 && !stoken.stop_requested(); i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    return;
}

// Callbacks that will be called if the config has been changed
void boolItemChanged(ConfigItemBoolean *item, bool newValue) {
    if (std::string_view("enabled") == item->identifier) {
        settings.enabled = newValue;
        // If the value has changed, we store it in the storage.
        WUPSStorageAPI::Store(item->identifier, newValue);
    }
}

void integerRangeItemChanged(ConfigItemIntegerRange *item, int newValue) {
    if (std::string_view("timeSet") == item->identifier) {
        settings.timeSet = newValue;
        // If the value has changed, we store it in the storage.
        WUPSStorageAPI::Store(item->identifier, newValue);
    }
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    try {
        auto infoCat = WUPSConfigCategory::Create("Setup Information");
        infoCat.add(WUPSConfigItemStub::Create("This plugin works with a pc application."));
        infoCat.add(WUPSConfigItemStub::Create("That application must be running to update rich presence."));
        infoCat.add(WUPSConfigItemStub::Create("Check this plugin's repository for more information."));
        root.add(std::move(infoCat));

        root.add(WUPSConfigItemBoolean::Create("enabled", "Enabled",
                                               settings.enabled, true,
                                               boolItemChanged));
        
        root.add(WUPSConfigItemIntegerRange::Create("timeSet", "Item for selecting an integer between 0 and 50",
                                                    0, settings.timeSet,
                                                    -12, 12,
                                                    &integerRangeItemChanged));

        return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
    } catch (std::exception &e) {
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }
}

INITIALIZE_PLUGIN() {
    settings.timeSet    = 0;
    settings.enabled = true;
    WUPSStorageAPI::Get("time", settings.timeSet);
    WUPSStorageAPI::Get("enabled", settings.enabled);
}

ON_APPLICATION_START() {
    app = GetNameOfCurrentApplication();
    if (GetNameOfCurrentApplication() == "Health and Safety Information") {app = "Homebrew Application";}
    if (app != preapp) {elapsed = time(NULL);}
    preapp = app;
    if (tthread.joinable()) {
        tthread.request_stop();
        tthread.join(); // Wait for thread to finish before starting a new one
    }
    tthread = std::jthread(GameLoop);
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
}