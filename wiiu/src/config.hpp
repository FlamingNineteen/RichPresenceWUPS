#include <wups.h>
#include <wups/config.h>
#include <wups/config/WUPSConfigCategory.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemIPAddress.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/config_api.h>

#include "consts.hpp"

// Set default values for config options
#define CONFIG_ENABLED_DEFAULT_VALUE true
#define CONFIG_NET_ID_DEFAULT_VALUE true
#define CONFIG_SMALL_IMG_DEFAULT_VALUE true
#define CONFIG_TIMESET_DEFAULT_VALUE 0
#define CONFIG_CTRL_DEFAULT_VALUE CTRLCOUNT
#define CONFIG_DST_DEFAULT_VALUE true
#define CONFIG_TITLE_DEFAULT_VALUE false
#define CONFIG_LANG_DEFAULT_VALUE ENGLISH
#define CONFIG_IP_FILTER_DEFAULT_VALUE false
#define CONFIG_IP_DEFAULT_VALUE (uint32_t) UINT32_MAX
#define CONFIG_PORT_DEFAULT_VALUE 5005
#define CONFIG_COD_DEFAULT_VALUE true

// Set config IDs for config options
#define CONFIG_ENABLED_CONFIG_ID "enabled"
#define CONFIG_NET_ID_CONFIG_ID "netid"
#define CONFIG_TIMESET_CONFIG_ID "timeset"
#define CONFIG_CTRL_CONFIG_ID "display"
#define CONFIG_SMALL_IMG_CONFIG_ID "smallimg"
#define CONFIG_DST_CONFIG_ID "dst"
#define CONFIG_TITLE_CONFIG_ID "title"
#define CONFIG_LANG_CONFIG_ID "lang"
#define CONFIG_IP_FILTER_CONFIG_ID "ipfilter"
#define CONFIG_IP_CONFIG_ID "ip"
#define CONFIG_PORT_CONFIG_ID "port"
#define CONFIG_COD_CONFIG_ID "cod"

// Create a variable for each config option
bool configEnabled     = CONFIG_ENABLED_DEFAULT_VALUE;
bool configNetId       = CONFIG_NET_ID_DEFAULT_VALUE;
int configTimeset      = CONFIG_TIMESET_DEFAULT_VALUE;
CtrlOptions configCtrl = CONFIG_CTRL_DEFAULT_VALUE;
bool configSmallImg    = CONFIG_SMALL_IMG_DEFAULT_VALUE;
bool configDst         = CONFIG_DST_DEFAULT_VALUE;
bool configTitle       = CONFIG_TITLE_DEFAULT_VALUE;
LangOptions configLang = CONFIG_LANG_DEFAULT_VALUE;
bool configIpFilter    = CONFIG_IP_FILTER_DEFAULT_VALUE;
uint32_t configIp      = CONFIG_IP_DEFAULT_VALUE;
int configPort         = CONFIG_PORT_DEFAULT_VALUE;
bool configCod         = CONFIG_COD_CONFIG_ID;

/**
 * Callbacks that will be called if the config has been changed
 */
void boolItemChanged(ConfigItemBoolean *item, bool newValue) {
    if (std::string_view(CONFIG_ENABLED_CONFIG_ID) == item->identifier) {
        configEnabled = newValue;
    }
    
    if (std::string_view(CONFIG_NET_ID_CONFIG_ID) == item->identifier) {
        configNetId = newValue;
    }

    if (std::string_view(CONFIG_SMALL_IMG_CONFIG_ID) == item->identifier) {
        configSmallImg = newValue;
    }

    if (std::string_view(CONFIG_TITLE_CONFIG_ID) == item->identifier) {
        configTitle = newValue;
    }

    if (std::string_view(CONFIG_DST_CONFIG_ID) == item->identifier) {
        configDst = newValue;
    }

    if (std::string_view(CONFIG_IP_FILTER_CONFIG_ID) == item->identifier) {
        configIpFilter = newValue;
    }

    if (std::string_view(CONFIG_COD_CONFIG_ID) == item->identifier) {
        configCod = newValue;
    }

    // If the value has changed, we store it in the storage.
    WUPSStorageAPI::Store(item->identifier, newValue);
}

void integerRangeItemChanged(ConfigItemIntegerRange *item, int newValue) {
    if (std::string_view(CONFIG_TIMESET_CONFIG_ID) == item->identifier) {
        configTimeset = newValue;
    }

    if (std::string_view(CONFIG_PORT_CONFIG_ID) == item->identifier) {
        configPort = newValue;
    }

    // If the value has changed, we store it in the storage.
    WUPSStorageAPI::Store(item->identifier, newValue);
}

void ipAddressItemChanged(ConfigItemIPAddress *item, uint32_t newValue) {
    if (std::string_view(CONFIG_IP_CONFIG_ID) == item->identifier) {
        configIp = newValue;
    }

    // If the value has changed, we store it in the storage.
    WUPSStorageAPI::Store(item->identifier, newValue);
}

void multipleValueItemChanged(ConfigItemMultipleValues *item, u_int32_t newValue) {
    if (std::string_view(CONFIG_CTRL_CONFIG_ID) == item->identifier) {
        configCtrl = (CtrlOptions) newValue;
    }

    if (std::string_view(CONFIG_LANG_CONFIG_ID) == item->identifier) {
        configLang = (LangOptions) newValue;
    }

    // If the value has changed, we store it in the storage.
    WUPSStorageAPI::Store(item->identifier, newValue);
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    // Create a new WUPSConfigCategory from the root handle
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    try {
        /* 
         * Setup Category
        */
        auto setupCat = WUPSConfigCategory::Create("Setup");
        setupCat.add(WUPSConfigItemStub::Create("This plugin works with a computer application."));
        setupCat.add(WUPSConfigItemStub::Create("That application must be running to update rich presence."));
        setupCat.add(WUPSConfigItemStub::Create("Check this plugin's repository for more information:"));
        setupCat.add(WUPSConfigItemStub::Create("https://github.com/flamingnineteen/RichPresenceWUPS"));
        
        /* 
         * Display Category
        */
        auto displayCat = WUPSConfigCategory::Create("Display");

        // Enable boolean
        displayCat.add(WUPSConfigItemBoolean::Create(CONFIG_ENABLED_CONFIG_ID, "Enable rich presence updates",
                                                    CONFIG_ENABLED_DEFAULT_VALUE, configEnabled,
                                                    boolItemChanged));
        
        // Controller count options
        constexpr WUPSConfigItemMultipleValues::ValuePair ctrlOptValues[] = {
            {NODISPLAY, "none"},
            {CTRLCOUNTNODRC, "exclude Gamepad"},
            {CTRLCOUNT, "all"}
        };

        // Controller count multiselect
        displayCat.add(WUPSConfigItemMultipleValues::CreateFromValue(CONFIG_CTRL_CONFIG_ID, "Show controller count",
                                                                    CONFIG_CTRL_DEFAULT_VALUE, configCtrl,
                                                                    ctrlOptValues,
                                                                    multipleValueItemChanged));
        
        // Network ID boolean
        displayCat.add(WUPSConfigItemBoolean::Create(CONFIG_NET_ID_CONFIG_ID, "Show Network ID",
                                                    CONFIG_NET_ID_DEFAULT_VALUE, configNetId,
                                                    boolItemChanged));

        // Small image boolean
        displayCat.add(WUPSConfigItemBoolean::Create(CONFIG_SMALL_IMG_CONFIG_ID, "Show currently used network",
                                                    CONFIG_SMALL_IMG_DEFAULT_VALUE, configSmallImg,
                                                    boolItemChanged));

        // Timeset integer range
        displayCat.add(WUPSConfigItemIntegerRange::Create(CONFIG_TIMESET_CONFIG_ID, "Offset \"elapsed time\" timezone for correct display",
                                                         CONFIG_TIMESET_DEFAULT_VALUE, configTimeset,
                                                         -12, 12,
                                                         &integerRangeItemChanged));
        
        // Daylight savings time boolean
        displayCat.add(WUPSConfigItemBoolean::Create(CONFIG_DST_CONFIG_ID, "Conform to Daylight Savings Time",
                                                    CONFIG_DST_DEFAULT_VALUE, configDst,
                                                    boolItemChanged));

        // Title boolean
        displayCat.add(WUPSConfigItemBoolean::Create(CONFIG_TITLE_CONFIG_ID, "Display full title",
                                                    CONFIG_TITLE_DEFAULT_VALUE, configTitle,
                                                    boolItemChanged));
            
        // Primary language options
        constexpr WUPSConfigItemMultipleValues::ValuePair langOptValues[] = {
            {ENGLISH, "English (Default)"},
            {JAPANESE, "Japanese"},
            {FRENCH, "French"},
            {GERMAN, "German"},
            {ITALIAN, "Italian"},
            {SPANISH, "Spanish"},
            {SIMP_CHINESE, "Simplified Chinese"},
            {KOREAN, "Korean"},
            {DUTCH, "Dutch"},
            {PORTUGUESE, "Portuguese"},
            {RUSSIAN, "Russian"},
            {TRAD_CHINESE, "Traditional Chinese"},
        };
        
        // Primary language multiselect
        displayCat.add(WUPSConfigItemMultipleValues::CreateFromValue(CONFIG_LANG_CONFIG_ID, "Primary title display language",
                                                                    CONFIG_LANG_DEFAULT_VALUE, configLang,
                                                                    langOptValues,
                                                                    multipleValueItemChanged));

        /* 
         * Advanced Category
        */
        auto advCat = WUPSConfigCategory::Create("Advanced");

        // IP filter boolean
        advCat.add(WUPSConfigItemBoolean::Create(CONFIG_IP_FILTER_CONFIG_ID, "Only send data to a specific IP address",
                                                CONFIG_IP_FILTER_DEFAULT_VALUE, configIpFilter,
                                                &boolItemChanged));

        // Sender ip address selection
        advCat.add(WUPSConfigItemIPAddress::Create(CONFIG_IP_CONFIG_ID, "IP address to send data to",
                                                    CONFIG_IP_DEFAULT_VALUE, configIp,
                                                    &ipAddressItemChanged));

        // Port integer range
        advCat.add(WUPSConfigItemIntegerRange::Create(CONFIG_PORT_CONFIG_ID, "UDP port (default 5005)",
                                                         CONFIG_PORT_DEFAULT_VALUE, configPort,
                                                         0, 65535,
                                                         &integerRangeItemChanged));

        // Call of Duty patch boolean
        advCat.add(WUPSConfigItemBoolean::Create(CONFIG_COD_CONFIG_ID, "Prevent Call of Duty crashes",
                                                    CONFIG_COD_DEFAULT_VALUE, configCod,
                                                    boolItemChanged));

        /* 
         * Contribute Category
        */
        auto helpCat = WUPSConfigCategory::Create("Contribute");
        helpCat.add(WUPSConfigItemStub::Create("The plugin might be missing images of some Wii U games."));
        helpCat.add(WUPSConfigItemStub::Create("If you are interested in adding game images, and"));
        helpCat.add(WUPSConfigItemStub::Create("have a Github account, check out this repository:"));
        helpCat.add(WUPSConfigItemStub::Create("https://github.com/flamingnineteen/RichPresenceWUPS-DB"));

        /*
         * Root Category
        */
        root.add(std::move(setupCat));
        root.add(std::move(displayCat));
        root.add(std::move(advCat));
        root.add(std::move(helpCat));

        return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
    } catch (std::exception &e) {return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;}
}

// void ConfigMenuClosedCallback() {
//     WUPSStorageAPI::SaveStorage();
// }
