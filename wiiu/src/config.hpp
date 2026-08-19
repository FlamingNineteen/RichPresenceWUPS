#include <wups.h>
#include <wups/config.h>
#include <wups/config/WUPSConfigCategory.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
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
#define CONFIG_PORT_DEFAULT_VALUE 5005
#define CONFIG_COD_DEFAULT_VALUE true

// Set config IDs for config options
#define CONFIG_ENABLED_CONFIG_ID "enabled"
#define CONFIG_NET_ID_CONFIG_ID "netid"
#define CONFIG_TIMESET_CONFIG_ID "timeset"
#define CONFIG_CTRL_CONFIG_ID "display"
#define CONFIG_SMALL_IMG_CONFIG_ID "smallimg"
#define CONFIG_DST_CONFIG_ID "dst"
#define CONFIG_PORT_CONFIG_ID "port"
#define CONFIG_COD_CONFIG_ID "cod"

// Create a variable for each config option
bool configEnabled        = CONFIG_ENABLED_DEFAULT_VALUE;
bool configNetId          = CONFIG_NET_ID_DEFAULT_VALUE;
int configTimeset         = CONFIG_TIMESET_DEFAULT_VALUE;
DisplayOptions configCtrl = CONFIG_CTRL_DEFAULT_VALUE;
bool configSmallImg       = CONFIG_SMALL_IMG_DEFAULT_VALUE;
bool configDst            = CONFIG_DST_DEFAULT_VALUE;
int configPort            = CONFIG_PORT_DEFAULT_VALUE;
bool configCod            = CONFIG_COD_CONFIG_ID;

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

    if (std::string_view(CONFIG_DST_CONFIG_ID) == item->identifier) {
        configDst = newValue;
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

void multipleValueItemChanged(ConfigItemMultipleValues *item, u_int32_t newValue) {
    // If the value has changed, we store it in the storage.
    if (std::string_view(CONFIG_CTRL_CONFIG_ID) == item->identifier) {
        configCtrl = (DisplayOptions) newValue;
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

        // Enable boolean
        configCat.add(WUPSConfigItemBoolean::Create(CONFIG_ENABLED_CONFIG_ID, "Enable rich presence updates",
                                                    CONFIG_ENABLED_DEFAULT_VALUE, configEnabled,
                                                    boolItemChanged));
        
        // Display options
        constexpr WUPSConfigItemMultipleValues::ValuePair displayOptValues[] = {
                {NODISPLAY, "none"},
                {CTRLCOUNTNODRC, "exclude Gamepad"},
                {CTRLCOUNT, "all"}
        };

        // Display multiselect
        configCat.add(WUPSConfigItemMultipleValues::CreateFromValue(CONFIG_CTRL_CONFIG_ID, "Show controller count",
                                                                    CONFIG_CTRL_DEFAULT_VALUE, configCtrl,
                                                                    displayOptValues,
                                                                    multipleValueItemChanged));
        
        // Network ID boolean
        configCat.add(WUPSConfigItemBoolean::Create(CONFIG_NET_ID_CONFIG_ID, "Show Network ID",
                                                    CONFIG_NET_ID_DEFAULT_VALUE, configNetId,
                                                    boolItemChanged));

        // Small image boolean
        configCat.add(WUPSConfigItemBoolean::Create(CONFIG_SMALL_IMG_CONFIG_ID, "Show currently used network",
                                                    CONFIG_SMALL_IMG_DEFAULT_VALUE, configSmallImg,
                                                    boolItemChanged));

        // Timeset integer range
        configCat.add(WUPSConfigItemIntegerRange::Create(CONFIG_TIMESET_CONFIG_ID, "Offset \"elapsed time\" timezone for correct display",
                                                         CONFIG_TIMESET_DEFAULT_VALUE, configTimeset,
                                                         -12, 12,
                                                         &integerRangeItemChanged));
        
        // Daylight savings time boolean
        configCat.add(WUPSConfigItemBoolean::Create(CONFIG_DST_CONFIG_ID, "Conform to Daylight Savings Time",
                                                    CONFIG_DST_DEFAULT_VALUE, configDst,
                                                    boolItemChanged));

        // Port integer range
        configCat.add(WUPSConfigItemIntegerRange::Create(CONFIG_PORT_CONFIG_ID, "UDP port (default 5005)",
                                                         CONFIG_PORT_DEFAULT_VALUE, configPort,
                                                         0, 65535,
                                                         &integerRangeItemChanged));

        // Call of Duty patch boolean
        configCat.add(WUPSConfigItemBoolean::Create(CONFIG_COD_CONFIG_ID, "Enable Call of Duty patches",
                                                    CONFIG_COD_DEFAULT_VALUE, configCod,
                                                    boolItemChanged));
        
        root.add(std::move(configCat));

        // Contribute category
        auto helpCat = WUPSConfigCategory::Create("Contribute");
        helpCat.add(WUPSConfigItemStub::Create("The plugin might be missing images of some Wii U games."));
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
