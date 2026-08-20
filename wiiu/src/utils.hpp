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
    if (metaXml) {
        if (ACPGetTitleMetaXml(OSGetTitleID(), metaXml) == ACP_RESULT_SUCCESS) {
            if (tag ==  "longname_en") {
                result = metaXml->longname_en;
            }
            else if (tag ==  "shortname_en") {
                result = metaXml->shortname_en;
            }
            else if (tag ==  "longname_ja") {
                result = metaXml->longname_ja;
            }
            else if (tag ==  "shortname_ja") {
                result = metaXml->shortname_ja;
            }
            else if (tag ==  "longname_fr") {
                result = metaXml->longname_fr;
            }
            else if (tag ==  "shortname_fr") {
                result = metaXml->shortname_fr;
            }
            else if (tag ==  "longname_de") {
                result = metaXml->longname_de;
            }
            else if (tag ==  "shortname_de") {
                result = metaXml->shortname_de;
            }
            else if (tag ==  "longname_it") {
                result = metaXml->longname_it;
            }
            else if (tag ==  "shortname_it") {
                result = metaXml->shortname_it;
            }
            else if (tag ==  "longname_es") {
                result = metaXml->longname_es;
            }
            else if (tag ==  "shortname_es") {
                result = metaXml->shortname_es;
            }
            else if (tag ==  "longname_zhs") {
                result = metaXml->longname_zhs;
            }
            else if (tag ==  "shortname_zhs") {
                result = metaXml->shortname_zhs;
            }
            else if (tag ==  "longname_ko") {
                result = metaXml->longname_ko;
            }
            else if (tag ==  "shortname_ko") {
                result = metaXml->shortname_ko;
            }
            else if (tag ==  "longname_nl") {
                result = metaXml->longname_nl;
            }
            else if (tag ==  "shortname_nl") {
                result = metaXml->shortname_nl;
            }
            else if (tag ==  "longname_pt") {
                result = metaXml->longname_pt;
            }
            else if (tag ==  "shortname_ru") {
                result = metaXml->shortname_ru;
            }
            else if (tag ==  "longname_zht") {
                result = metaXml->longname_zht;
            }
            else if (tag ==  "shortname_zht") {
                result = metaXml->shortname_zht;
            }
            else {
                result.clear();
            }
        }
        else {
            result.clear();
        }
        free(metaXml);
    }
    ACPFinalize();
    return result;
}

/**
 * Attempts to get a title from the application's `meta.xml`.
 * If the chosen title is blank it will try other languages.
 * @param full Whether or not to get the full title.
 * @param lang The language from the LangOptions enum.
 * @return The title string
 */
std::string GetAppTitle(LangOptions lang = ENGLISH, bool full = false) {
    std::string len = full ? "longname_" : "shortname_";
    std::string result = GetXmlTag(len + LANG_STRINGS[static_cast<int>(lang)]);

    if (result != "") {
        return result;
    }
    else {
        // Try every language in order as they appear in `meta.xml`
        for (std::string i : LANG_STRINGS) {
            result = GetXmlTag(len + i);

            if (result != "") {
                return result;
            }
        }
    }

    return "";
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
 *         `"pn"` for Pretendo,
 *         `""` upon error.
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
