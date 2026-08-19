#include <padscore/wpad.h>

#include <nn/acp/client.h>
#include <nn/acp/title.h>
#include <nn/act.h>

#include <coreinit/title.h>
#include <coreinit/time.h>
#include <coreinit/thread.h>

#include <arpa/inet.h>

#include <mocha/mocha.h>

#include "consts.hpp"

/**
 * Gets the current number of connected controllers,
 * excluding anything not directly connected to a
 * WPAD channel.
 * @return The current number of connected controllers.
 */
int GetCtrlNum() {
    int c = 0;
    WPADExtensionType extType;
    for (int i = 0; i < 7; i++) {
        int32_t result = WPADProbe(WPAD_CHANS[i], &extType);
        if (result != -1) {c++;}
    }
    return c;
}

/**
 * Gets a tag from the application's `meta.xml`.
 * @param tag The tag to get.
 * @return The value of the tag as a string.
 */
std::string GetXmlTag(std::string tag) {
    std::string result;
    ACPInitialize();
    auto *metaXml = (ACPMetaXml *) memalign(0x40, sizeof(ACPMetaXml));
    if (metaXml)
    {
        if (ACPGetTitleMetaXml(OSGetTitleID(), metaXml) == ACP_RESULT_SUCCESS)
        {
            if (tag == "longname_en") result = metaXml->longname_en;
            else if (tag == "shortname_en") result = metaXml->shortname_en;
            else result.clear();
        }
        else
        {
            result.clear();
        }
        free(metaXml);
    }
    ACPFinalize();
    return result;
}

/** 
 * Replaces any instances of `"\\n"` from a string with
 * `" "`, effectively putting everything onto one line.
 * @param s The string to replace.
 * @return The replaced string.
 */
std::string ReplaceSlashN(std::string s) {
    while (true) {
        size_t finder = s.find("\n");
        if (finder == std::string::npos) break;
        s.replace(finder, 1, " ");
    }
    return s;
}

/**
 * Gets the network id of the current account.
 * @return The network id of the current account, as a string.
 */
std::string GetNetworkId() {
    char account_id[256];
    nn::act::GetAccountId(account_id);
    std::string stickyId = account_id;
    return stickyId;
}

/**
 * Gets the currently used network.
 * Network is decided by the Inkay config file.
 * @return `"nn"` for Nintendo,
 * @return `"pn"` for Pretendo,
 * @return `""` upon error.
 */
std::string GetNetwork(bool inkayExists, std::string inkayConfig) {
    if (inkayExists) {
        std::ifstream acc(inkayConfig);
        if (!acc.is_open()) {
            return "";
        }

        size_t pos;
        std::string line;
        while (std::getline(acc, line)) {
            pos = line.find("connect_to_network");
            if (pos != std::string::npos) {
                pos = line.find("true");
                if (pos != std::string::npos) {
                    return "pn";
                } else {
                    return "nn";
                }
            }
        }
    }
    return "nn";
}
