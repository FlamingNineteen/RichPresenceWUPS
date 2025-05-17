#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include <wups.h>
#include <wups/config.h>

#include <coreinit/cache.h>
#include <coreinit/title.h>
#include <coreinit/time.h>

#include <wut.h>

#include <padscore/wpad.h>
#include <padscore/kpad.h>

#include <nn/acp/client.h>
#include <nn/acp/title.h>

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME("RichPresence");
WUPS_PLUGIN_DESCRIPTION("Discord Rich Presence for the Wii U.");
WUPS_PLUGIN_VERSION("v1.0");
WUPS_PLUGIN_AUTHOR("Flaming19");
WUPS_PLUGIN_LICENSE("GNU");

WUPS_USE_WUT_DEVOPTAB();             // Use the wut devoptabs
WUPS_USE_STORAGE("rich_presence"); // Unique id for the storage api

// Global scope
const char* app;
std::string ctrls;
bool running =         false;
WPADChan channels[7] = {WPAD_CHAN_0, WPAD_CHAN_1, WPAD_CHAN_2, WPAD_CHAN_3, WPAD_CHAN_4, WPAD_CHAN_5, WPAD_CHAN_6};

void WriteToSDCard(std::string data, std::string num) {
    const char* filePath = "/vol/external01/rich_presence.txt";
    data += "^" + std::to_string(time(NULL)) + "^" + num + "\n";
    const char* datacs = data.c_str();

    FILE* file = fopen(filePath, "w");
    if (!file) {
        return;
    }

    // Overwrite file and make it blank
    fprintf(file, "%s", "");
    size_t dataSize = strlen(datacs);

    fwrite(datacs, 1, dataSize, file);
    fclose(file);
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

int ctrlNum() {
    WPADExtensionType extType;
    int c = 0;
    for (int i = 0; i < 7; i++) {
        int32_t result = WPADProbe(channels[i], &extType);
        if (result != -1) {c++;}
    }
    return c;
}

void OnControllerConnect(WPADChan chan, int32_t status) {
    ctrls = std::to_string(ctrlNum());
    WriteToSDCard(app, ctrls);
}

INITIALIZE_PLUGIN() {
    WPADInit();
    // Register the controller connection callback for all channels
    for (int i = 0; i < 7; i++) {
        WPADSetConnectCallback(channels[i], OnControllerConnect);
    }

    // Reset rich_presence.txt
    FILE* file = fopen("/vol/external01/rich_presence.txt", "w");
    fwrite("", 1, 0, file);
    fclose(file);
}

DEINITIALIZE_PLUGIN() {
    WPADShutdown();
}

ON_APPLICATION_START() {
    ctrls = std::to_string(ctrlNum());
    app = GetNameOfCurrentApplication().c_str();
    if (GetNameOfCurrentApplication() == "Health and Safety Information") {
        app = "Homebrew Application";
    }
    WriteToSDCard(app, ctrls);
}
