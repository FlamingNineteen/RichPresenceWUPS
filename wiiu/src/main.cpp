#include <algorithm> // Add this include for std::sort
#include <atomic>
#include <chrono>
#include <dirent.h>
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
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/config_api.h>

#include <coreinit/title.h>
#include <coreinit/time.h>
#include <coreinit/thread.h>

#include <arpa/inet.h>

#include <sys/stat.h>

#include <padscore/wpad.h>
#include <padscore/kpad.h>

#include <nn/acp/client.h>
#include <nn/acp/title.h>
#include <nn/act.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("RichPresence");
WUPS_PLUGIN_DESCRIPTION("Discord Rich Presence for the Wii U.");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("Flaming19");
WUPS_PLUGIN_LICENSE("GNU");

#define STACK_SIZE 0x2000
#define CONFIG_ENABLED_CONFIG_ID "enabled"
#define CONFIG_TIMESET_CONFIG_ID "timeset"
#define CONFIG_DISPLAY_CONFIG_ID "display"

/**
    All of these defines can be used in ANY file.
    It's possible to split it up into multiple files.
**/

WUPS_USE_WUT_DEVOPTAB();           // Use the wut devoptabs
WUPS_USE_STORAGE("rich_presence"); // Unique id for the storage api

enum DisplayOptions {
    NODISPLAY = 0, CTRLCOUNT = 1, CTRLCOUNTNODRC = 2, NNID = 3
};

#define CONFIG_ENABLED_DEFAULT_VALUE true
#define CONFIG_TIMESET_DEFAULT_VALUE 0
#define CONFIG_DISPLAY_DEFAULT_VALUE CTRLCOUNT

std::jthread tthread;
std::string nnid;
int elapsed;
bool configEnabled           = CONFIG_ENABLED_DEFAULT_VALUE;
int configTimeset            = CONFIG_TIMESET_DEFAULT_VALUE;
DisplayOptions configDisplay = CONFIG_DISPLAY_DEFAULT_VALUE;
std::string app              = "";
std::string preapp           = "quantum random!!!11!";
WPADChan channels[7]         = {WPAD_CHAN_0, WPAD_CHAN_1, WPAD_CHAN_2, WPAD_CHAN_3, WPAD_CHAN_4, WPAD_CHAN_5, WPAD_CHAN_6};

int ctrlNum(DisplayOptions display) {
    int c;
    switch (display) {
        case CTRLCOUNT:
            c = 0;
            break;
        case CTRLCOUNTNODRC:
            c = -1;
            break;
        default:
            return 0;
    }
    WPADExtensionType extType;
    for (int i = 0; i < 7; i++) {
        int32_t result = WPADProbe(channels[i], &extType);
        if (result != -1) {c++;}
    }
    return c;
}

std::string GetXmlTag(std::string tag) {
    std::string result;
    ACPInitialize();
    auto *metaXml = (ACPMetaXml *) memalign(0x40, sizeof(ACPMetaXml));
    if (ACPGetTitleMetaXml(OSGetTitleID(), metaXml) == ACP_RESULT_SUCCESS) {
        if (tag == "longname_en") result       = metaXml->longname_en;
        else if (tag == "shortname_en") result = metaXml->shortname_en;
        else result.clear();
    } else {
        result.clear();
    }
    ACPFinalize();
    return result;
}

std::string RemoveSlashN(std::string s) {
    while (true) {
        size_t finder = s.find("\n");
        if (finder == std::string::npos) break;
        s.replace(finder, 1, " ");
    }
    return s;
}

std::vector<std::string> ListDirs(const char* path) {
    std::vector<std::string> dirs;
    DIR* dir = opendir(path);
    if (!dir) {
        return dirs;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Build full path
        std::string fullPath = std::string(path) + "/" + entry->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            dirs.push_back(entry->d_name);
        }
    }
    closedir(dir);
    return dirs;
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

std::string GetNnid() {
    if (static_cast<int>(configDisplay) > 2) {
        char account_id[256];
        nn::act::GetAccountId(account_id);
        std::string stickyId = account_id;
        return stickyId;
    } else return "";
}

void GameLoop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        if (app != "") {
            int ctrls = ctrlNum(configDisplay);
            nnid = GetNnid();
            std::string json = "{\"sender\":\"Wii U\",\"long\":\"" + RemoveSlashN(GetXmlTag("longname_en")) + "\",\"app\":\"" + app + "\",\"time\":" + std::to_string(elapsed + (configTimeset * 3600)) + ",\"ctrls\":" + std::to_string(ctrls) + ",\"nnid\":\"" + nnid + "\",\"display\":" + std::to_string(static_cast<int>(configDisplay)) + "}";
            Broadcast(json);
        }

        // Five second interval
        for (int i=0; i<1000 && !stoken.stop_requested(); i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    return;
}

/**
 * Callback that will be called if the config has been changed
 */
void boolItemChanged(ConfigItemBoolean *item, bool newValue) {
    if (std::string_view(CONFIG_ENABLED_CONFIG_ID) == item->identifier) {
        configEnabled = newValue;
        // If the value has changed, we store it in the storage.
        WUPSStorageAPI::Store(item->identifier, newValue);
    }
}

void integerRangeItemChanged(ConfigItemIntegerRange *item, int newValue) {
    if (std::string_view(CONFIG_TIMESET_CONFIG_ID) == item->identifier) {
        configTimeset = newValue;
        // If the value has changed, we store it in the storage.
        WUPSStorageAPI::Store(item->identifier, newValue);
    }
}

void multipleValueItemChanged(ConfigItemMultipleValues *item, u_int32_t newValue) {
    // If the value has changed, we store it in the storage.
    if (std::string_view(CONFIG_DISPLAY_CONFIG_ID) == item->identifier) {
        configDisplay = (DisplayOptions) newValue;
        // If the value has changed, we store it in the storage.
        WUPSStorageAPI::Store(item->identifier, newValue);
    }
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    // Create a new WUPSConfigCategory from the root handle
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    try {
        // Setup information category
        auto setupCat = WUPSConfigCategory::Create("Setup information");
        setupCat.add(WUPSConfigItemStub::Create("This plugin works with a computer application."));
        setupCat.add(WUPSConfigItemStub::Create("That application must be running to update rich presence."));
        setupCat.add(WUPSConfigItemStub::Create("Check this plugin's repository for more information:"));
        setupCat.add(WUPSConfigItemStub::Create("https://github.com/flamingnineteen/RichPresenceWUPS"));
        root.add(std::move(setupCat));
        
        // Settings category
        auto configCat = WUPSConfigCategory::Create("Plugin settings");

        // Enabled boolean
        configCat.add(WUPSConfigItemBoolean::Create(CONFIG_ENABLED_CONFIG_ID, "Enabled",
                                                    CONFIG_ENABLED_DEFAULT_VALUE, configEnabled,
                                                    boolItemChanged));

        // Timeset integer range
        configCat.add(WUPSConfigItemIntegerRange::Create(CONFIG_TIMESET_CONFIG_ID, "Offset \"elapsed time\" timezone for correct display",
                                                         CONFIG_TIMESET_DEFAULT_VALUE, configTimeset,
                                                         -12, 12,
                                                         &integerRangeItemChanged));
        
        // Display options
        constexpr WUPSConfigItemMultipleValues::ValuePair displayOptValues[] = {
                {NODISPLAY, "Nothing"},
                {CTRLCOUNT, "Player Count"},
                {CTRLCOUNTNODRC, "Player Count (exclude Gamepad)"},
                {NNID, "Network ID"}
        };

        // Display multiselect
        configCat.add(WUPSConfigItemMultipleValues::CreateFromValue(CONFIG_DISPLAY_CONFIG_ID, "Display under game name",
                                                                    CONFIG_DISPLAY_DEFAULT_VALUE, configDisplay,
                                                                    displayOptValues,
                                                                    multipleValueItemChanged));
        
        root.add(std::move(configCat));

        // Contribute category
        auto helpCat = WUPSConfigCategory::Create("Contribute");
        helpCat.add(WUPSConfigItemStub::Create("The plugin is missing images of many Wii U games."));
        helpCat.add(WUPSConfigItemStub::Create("If you are interested in adding game images, and"));
        helpCat.add(WUPSConfigItemStub::Create("have a Github account, check out this repository:"));
        helpCat.add(WUPSConfigItemStub::Create("https://github.com/flamingnineteen/RichPresenceWUPS-DB"));
        root.add(std::move(helpCat));

        return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
    } catch (std::exception &e) {return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;}
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();
}

INITIALIZE_PLUGIN() {
    WUPSConfigAPIOptionsV1 configOptions = {.name = "Rich Presence"};
    WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_ENABLED_CONFIG_ID, configEnabled, CONFIG_ENABLED_DEFAULT_VALUE);
    WUPSStorageAPI::GetOrStoreDefault(CONFIG_TIMESET_CONFIG_ID, configTimeset, CONFIG_TIMESET_DEFAULT_VALUE);
    WUPSStorageAPI::SaveStorage();

    if (static_cast<int>(configDisplay) > 2) nnid = GetNnid();
}

ON_APPLICATION_START() {
    app = GetXmlTag("shortname_en");
    if (app == "Health and Safety Information") app = "Homebrew Application";
    if (app != preapp) elapsed = time(NULL); // Only update elapsed time if app changed
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