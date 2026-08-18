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

bool INKAY_EXISTS;
std::string INKAY_CONFIG;

// Returns the number of connected controllers
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
            return -2;
    }
    WPADExtensionType extType;
    for (int i = 0; i < 7; i++) {
        int32_t result = WPADProbe(channels[i], &extType);
        if (result != -1) {c++;}
    }
    return c;
}

// Gets a tag from the application's xml
std::string getXmlTag(std::string tag) {
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

// Removes any instances of \n from a string
std::string removeSlashN(std::string s) {
    while (true) {
        size_t finder = s.find("\n");
        if (finder == std::string::npos) break;
        s.replace(finder, 1, " ");
    }
    return s;
}

// Gets the network id of the current account.
std::string getNnid() {
    if (configNetId) {
        char account_id[256];
        nn::act::GetAccountId(account_id);
        std::string stickyId = account_id;
        return stickyId;
    } else return "";
}

/**
 * Gets the currently used network.
 * Network is decided by the Inkay config file.
 * @returns
 * `nn` for Nintendo,
 * `pn` for Pretendo,
 * nothing for neither
 */
std::string getNetwork() {
    if (configSmallImg) {
        if (INKAY_EXISTS) {
            std::ifstream acc(INKAY_CONFIG);
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
            return "nn";
        } else {
            return "nn";
        }
    } else {
        return "";
    }
}
