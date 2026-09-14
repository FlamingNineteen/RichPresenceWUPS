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

/**
 * A configuration option.
 */
template <typename T>
struct ConfigOption {
    std::string id; // Config ID
    T def;          // Default value
    T value;        // Value of the item

    ConfigOption(const char *i, T d) {
        id = std::string(i);
        def = value = d;
    }
};

/**
 * Options for controller display.
 */
enum CtrlDisplay {
    // Do not display controller count
    NOCTRLCOUNT,
    
    // Display the controller count, excluding the Gamepad
    CTRLCOUNTNODRC,
    
    // Display the total controller count
    CTRLCOUNT,

    // Display based on the `drc_use` tag in the game's `meta.xml`
    CTRLCOUNTMETA
};

/**
 * Options for display language.
 */
enum LangOptions {
    ENGLISH,
    JAPANESE,
    FRENCH,
    GERMAN,
    ITALIAN,
    SPANISH,
    SIMP_CHINESE,
    KOREAN,
    DUTCH,
    PORTUGUESE,
    RUSSIAN,
    TRAD_CHINESE
};

/**
 * Options for network display
 */
enum NetDisplay {
    // Do not display network ID
    NONETDISPLAY,

    // Display network ID if the game uses network accounts
    NETDISPLAYMETA,

    // Display network ID
    NETDISPLAY
};

/**
 * All config options defined as one structure.
 */
struct {
    ConfigOption<bool> enabled = 
    ConfigOption<bool>("enabled", true);

    ConfigOption<NetDisplay> net_id = 
    ConfigOption<NetDisplay>("netid", NETDISPLAYMETA);

    ConfigOption<bool> small_img = 
    ConfigOption<bool>("smallimg", true);

    ConfigOption<int> timeset = 
    ConfigOption<int>("timeset", 0);

    ConfigOption<CtrlDisplay> ctrl = 
    ConfigOption<CtrlDisplay>("display", CTRLCOUNTMETA);

    ConfigOption<bool> dst = 
    ConfigOption<bool>("dst", true);

    ConfigOption<bool> title = 
    ConfigOption<bool>("title", true);

    ConfigOption<LangOptions> lang = 
    ConfigOption<LangOptions>("lang", ENGLISH);

    ConfigOption<bool> ip_filter = 
    ConfigOption<bool>("ipfilter", false);

    ConfigOption<uint32_t> ip = 
    ConfigOption<uint32_t>("ip", UINT32_MAX);

    ConfigOption<int> port = 
    ConfigOption<int>("port", 5005);

    ConfigOption<bool> cod = 
    ConfigOption<bool>("cod", true);
} config;

/**
 * Callbacks that will be called if the config has been changed
 */
void boolItemChanged(ConfigItemBoolean *item, bool newValue) {
    if (std::string_view(config.enabled.id) == item->identifier) {
        config.enabled.value = newValue;
    }

    if (std::string_view(config.small_img.id) == item->identifier) {
        config.small_img.value = newValue;
    }

    if (std::string_view(config.title.id) == item->identifier) {
        config.title.value = newValue;
    }

    if (std::string_view(config.dst.id) == item->identifier) {
        config.dst.value = newValue;
    }

    if (std::string_view(config.ip_filter.id) == item->identifier) {
        config.ip_filter.value = newValue;
    }

    if (std::string_view(config.cod.id) == item->identifier) {
        config.cod.value = newValue;
    }

    // If the value has changed, we store it in the storage.
    WUPSStorageAPI::Store(item->identifier, newValue);
}

void integerRangeItemChanged(ConfigItemIntegerRange *item, int newValue) {
    if (std::string_view(config.timeset.id) == item->identifier) {
        config.timeset.value = newValue;
    }

    if (std::string_view(config.port.id) == item->identifier) {
        config.port.value = newValue;
    }

    // If the value has changed, we store it in the storage.
    WUPSStorageAPI::Store(item->identifier, newValue);
}

void ipAddressItemChanged(ConfigItemIPAddress *item, uint32_t newValue) {
    if (std::string_view(config.ip.id) == item->identifier) {
        config.ip.value = newValue;
    }

    // If the value has changed, we store it in the storage.
    WUPSStorageAPI::Store(item->identifier, newValue);
}

void multipleValueItemChanged(ConfigItemMultipleValues *item, uint32_t newValue) {
    if (std::string_view(config.ctrl.id) == item->identifier) {
        config.ctrl.value = (CtrlDisplay) newValue;
    }

    if (std::string_view(config.net_id.id) == item->identifier) {
        config.net_id.value = (NetDisplay) newValue;
    }

    if (std::string_view(config.lang.id) == item->identifier) {
        config.lang.value = (LangOptions) newValue;
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
        displayCat.add(WUPSConfigItemBoolean::Create(config.enabled.id, "Enable rich presence updates",
                                                    config.enabled.def, config.enabled.value,
                                                    boolItemChanged));
        
        // Controller count options
        constexpr WUPSConfigItemMultipleValues::ValuePair ctrlOptValues[] = {
            {NOCTRLCOUNT, "none"},
            {CTRLCOUNTNODRC, "exclude Gamepad"},
            {CTRLCOUNTMETA, "based on the current app"},
            {CTRLCOUNT, "all"}
        };

        // Controller count multiselect
        displayCat.add(WUPSConfigItemMultipleValues::CreateFromValue(config.ctrl.id, "Show controller count",
                                                                    config.ctrl.def, config.ctrl.value,
                                                                    ctrlOptValues,
                                                                    multipleValueItemChanged));
        
        // Network ID options
        constexpr WUPSConfigItemMultipleValues::ValuePair netIdOptValues[] = {
            {NONETDISPLAY, "never"},
            {NETDISPLAYMETA, "on apps with online features"},
            {NETDISPLAY, "always"}
        };

        // Network ID multiselect
        displayCat.add(WUPSConfigItemMultipleValues::CreateFromValue(config.net_id.id, "Show network ID",
                                                                    config.net_id.def, config.net_id.value,
                                                                    netIdOptValues,
                                                                    multipleValueItemChanged));

        // Small image boolean
        displayCat.add(WUPSConfigItemBoolean::Create(config.small_img.id, "Show currently used network",
                                                    config.small_img.def, config.small_img.value,
                                                    boolItemChanged));

        // Timeset integer range
        displayCat.add(WUPSConfigItemIntegerRange::Create(config.timeset.id, "Offset \"elapsed time\" timezone for correct display",
                                                         config.timeset.def, config.timeset.value,
                                                         -12, 12,
                                                         &integerRangeItemChanged));
        
        // Daylight savings time boolean
        displayCat.add(WUPSConfigItemBoolean::Create(config.dst.id, "Conform to Daylight Savings Time",
                                                    config.dst.def, config.dst.value,
                                                    boolItemChanged));

        // Title boolean
        displayCat.add(WUPSConfigItemBoolean::Create(config.title.id, "Display full title",
                                                    config.title.def, config.title.value,
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
        displayCat.add(WUPSConfigItemMultipleValues::CreateFromValue(config.lang.id, "Primary title display language",
                                                                    config.lang.def, config.lang.value,
                                                                    langOptValues,
                                                                    multipleValueItemChanged));

        /* 
         * Advanced Category
        */
        auto advCat = WUPSConfigCategory::Create("Advanced");

        // IP filter boolean
        advCat.add(WUPSConfigItemBoolean::Create(config.ip_filter.id, "Only send data to a specific IP address",
                                                config.ip_filter.def, config.ip_filter.value,
                                                &boolItemChanged));

        // Sender ip address selection
        advCat.add(WUPSConfigItemIPAddress::Create(config.ip.id, "IP address to send data to",
                                                    config.ip.def, config.ip.value,
                                                    &ipAddressItemChanged));

        // Port integer range
        advCat.add(WUPSConfigItemIntegerRange::Create(config.port.id, "UDP port (default 5005)",
                                                         config.port.def, config.port.value,
                                                         0, 65535,
                                                         &integerRangeItemChanged));

        // Call of Duty patch boolean
        advCat.add(WUPSConfigItemBoolean::Create(config.cod.id, "Prevent Call of Duty crashes",
                                                    config.cod.def, config.cod.value,
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
