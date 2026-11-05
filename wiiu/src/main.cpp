#include <filesystem>
#include <fstream>
#include <malloc.h>
#include <string.h>
#include <thread>
#include <vector>

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
WUPS_PLUGIN_AUTHOR("Flaming19, direct mode by sennecaelen");
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

std::string ENVIRONMENT_PATH;
int protariumState = -1; // -1 not checked yet, 0 no, 1 yes

/**
 * Whether the installed Inkay is Protarium's build. Like Prot-endo-Switcher,
 * this looks for "protarium" inside the Inkay plugin and module files.
 * Checked once, the first time it's needed.
 */
bool FileContains(const std::string &path, const std::string &needle) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    std::string carry;
    std::vector<char> buffer(64 * 1024);
    while (f) {
        f.read(buffer.data(), buffer.size());
        std::string chunk = carry + std::string(buffer.data(), f.gcount());
        for (char &c : chunk) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (chunk.find(needle) != std::string::npos) return true;
        carry = chunk.size() > needle.size() ? chunk.substr(chunk.size() - needle.size()) : chunk;
    }
    return false;
}

bool IsProtarium() {
    if (protariumState != -1) return protariumState == 1;
    protariumState = 0;
    for (const char *folder : {"/plugins", "/modules"}) {
        std::error_code ec;
        for (const auto &entry : std::filesystem::directory_iterator(ENVIRONMENT_PATH + folder, ec)) {
            std::string name = entry.path().filename().string();
            for (char &c : name) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (name.find("inkay") != std::string::npos && FileContains(entry.path().string(), "protarium")) {
                protariumState = 1;
                return true;
            }
        }
    }
    return false;
}

// "nn", "pn", "pr" (Protarium) or ""
std::string CurrentNetwork() {
    std::string network = CurrentNetwork();
    if (network == "pn" && IsProtarium()) network = "pr";
    return network;
}

// Direct mode bookkeeping
std::string lastSentKey;
long long lastSentAt = 0;
bool lastSendOk      = false;

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
void GameLoopUnsafe(std::stop_token stoken);

// Reads a number from meta.xml; 0 when it can't be read (std::stoi would throw).
int XmlInt(const std::string &tag) {
    std::string value = GetXmlTag(tag);
    char *end         = nullptr;
    long n            = strtol(value.c_str(), &end, 10);
    return end == value.c_str() ? 0 : static_cast<int>(n);
}

// An exception leaving a thread aborts the whole console, so catch everything.
void GameLoop(std::stop_token stoken) {
    try {
        GameLoopUnsafe(stoken);
    } catch (const std::exception &e) {
        DiscordDirect::lastError = std::string("Internal error: ") + e.what();
    } catch (...) {
        DiscordDirect::lastError = "Internal error";
    }
}

void GameLoopUnsafe(std::stop_token stoken) {
    int ctrls;
    std::string details, nnid, network, json;

    while (!stoken.stop_requested() && config.enabled.value) {
        if (app != "") {
            // Get controller count
            switch (config.ctrl.value) {
                case NOCTRLCOUNT:
                    ctrls = -2;
                    break;
                case CTRLCOUNT:
                    ctrls = GetCtrlNum();
                    break;
                case CTRLCOUNTNODRC:
                    ctrls = GetCtrlNum()-1;
                    break;
                case CTRLCOUNTMETA:
                    ctrls = XmlInt("drc_use") > 1 ? 0 : GetCtrlNum() + XmlInt("drc_use") - 1;
                    break;
                default:
                    ctrls = -2;
            }

            // Get Network ID
            switch (config.net_id.value) {
                case NONETDISPLAY:
                    nnid = "";
                    break;
                case NETDISPLAYMETA:
                    nnid = XmlInt("online_account_use") ? GetNetworkId() : "";
                    break;
                case NETDISPLAY:
                    nnid = GetNetworkId();
                    break;
                default:
                    nnid = "";
            }

            // Get currently used network
            switch (config.small_img.value) {
                case NONETDISPLAY:
                    network = "";
                    break;
                case NETDISPLAYMETA:
                    network = XmlInt("online_account_use") ? CurrentNetwork() : "";
                    break;
                case NETDISPLAY:
                    network = GetNetwork(INKAY_EXISTS, INKAY_CONFIG);
                    break;
                default:
                    network = "";
            }

            // if (ReplaceSlashN(GetXmlTag("longname_en")) == "Super Smash Bros. for Wii U") {
            //     details = std::to_string(ReadFromMemory(0x1098B2AB)>>24) + " | " + std::to_string(ReadFromMemory(0x1098EDEB)>>24);
            // }
            
            if (config.mode.value == MODE_DIRECT) {
                DiscordDirect::Presence p;
                p.name     = app;
                p.longName = ReplaceSlashN(GetAppTitle(ENGLISH, true));
                p.nnid     = nnid;
                p.network  = network;
                if (!nnid.empty()) p.realNetwork = CurrentNetwork();
                p.ctrls    = ctrls;
                p.start    = elapsed;

                // Send when something changed, otherwise keep the session alive
                // (Discord drops it after 20 minutes). Retry sooner after a failure.
                long long now  = time(NULL);
                long long wait = lastSendOk ? 240 : 30;
                std::string key = p.Key();
                if (key != lastSentKey || now - lastSentAt >= wait) {
                    lastSendOk  = DiscordDirect::Update(p);
                    lastSentKey = key;
                    lastSentAt  = now;
                }
            }
            else {
            // Prepare and send json
            json = "{\"sender\":\"Wii U\",\"long\":\"" + ReplaceSlashN(GetAppTitle(ENGLISH, true)) + "\",\"app\":\"" + app + "\",\"details\":\"" + details + "\",\"time\":" + std::to_string(elapsed + (config.timeset.value * 3600)) + ",\"ctrls\":" + std::to_string(ctrls) + ",\"nnid\":\"" + nnid + "\",\"img\":\"" + (network == "pr" ? std::string("pn") : network) + "\",\"dst\":" + std::to_string(config.dst.value) + ",\"compatibility\":" + std::to_string(COMPATIBLE_VERSION) + "}";
            Broadcast(json);
            }
        }

        // Five second interval
        for (int i=0; i<5 && !stoken.stop_requested() && config.enabled.value; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    return;
}

// Stops the presence thread, cancelling a request that is still running.
void StopPresenceThread() {
    if (tthread.joinable()) {
        DiscordDirect::abortRequests = true;
        tthread.request_stop();
        tthread.join();
        DiscordDirect::abortRequests = false;
    }
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();

    app = GetXmlTag("shortname_en") == "Health and Safety Information" ? "Homebrew Application" : ReplaceSlashN(GetAppTitle(config.lang.value, config.title.value));
    preapp = app;

    StopPresenceThread();

    // Drop the presence when direct mode is off, then pick up a new token file
    if (!config.enabled.value || config.mode.value != MODE_DIRECT) DiscordDirect::Clear();
    DiscordDirect::tokensLoaded = false;
    DiscordDirect::needsSetup   = false;
    lastSentKey.clear();

    if ((config.enabled.value && !(config.cod.value && app.find("Call of Duty") != std::string::npos))) {
        tthread = std::jthread(GameLoop);
    }
}

INITIALIZE_PLUGIN() {
    Mocha_InitLibrary();

    WUPSConfigAPIOptionsV1 configOptions = {.name = "Rich Presence"};
    WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback);
    WUPSStorageAPI::GetOrStoreDefault(config.enabled.id, config.enabled.value, config.enabled.def);
    WUPSStorageAPI::GetOrStoreDefault(config.mode.id, config.mode.value, config.mode.def);
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
    ENVIRONMENT_PATH = std::string(environment_path_buffer);
    INKAY_CONFIG = std::string(environment_path_buffer) + std::string("/plugins/config/inkay.json");
    INKAY_EXISTS = std::filesystem::exists(INKAY_CONFIG);

    DiscordDirect::Init(std::string(environment_path_buffer));
}

ON_APPLICATION_START() {
    app = GetXmlTag("shortname_en") == "Health and Safety Information" ? "Homebrew Application" : ReplaceSlashN(GetAppTitle(config.lang.value, config.title.value)); 

    if (app != preapp) elapsed = time(NULL); // Only update elapsed time if app changed
    preapp = app;

    StopPresenceThread();
    lastSentKey.clear();
    if (config.enabled.value && !(config.cod.value && app.find("Call of Duty") != std::string::npos)) tthread = std::jthread(GameLoop);
}

ON_APPLICATION_REQUESTS_EXIT() {    
    // The session is kept and updated by the next title: deleting and
    // recreating it on every app switch gets rate limited by Discord.
    // After the console turns off, Discord removes it within 20 minutes.
    StopPresenceThread();
}

DEINITIALIZE_PLUGIN() {
    StopPresenceThread();
    DiscordDirect::Clear();
    DiscordDirect::Deinit();

    Mocha_DeInitLibrary();
}
