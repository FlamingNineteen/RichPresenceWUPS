/**
 * The version number of the plugin.
 */
#define VERSION "v2.11"

#pragma once

#include <string.h>

#include <padscore/wpad.h>

/**
 * Options for controller display.
 */
enum CtrlOptions {
    // Do not display controller count
    NODISPLAY,
    
    // Display the controller count, excluding the Gamepad
    CTRLCOUNTNODRC,
    
    // Display the total controller count
    CTRLCOUNT
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
 * An array of all seven WPAD channels.
 */
const WPADChan WPAD_CHANS[7] = {WPAD_CHAN_0, WPAD_CHAN_1, WPAD_CHAN_2, WPAD_CHAN_3, WPAD_CHAN_4, WPAD_CHAN_5, WPAD_CHAN_6};

/**
 * An array of language codes as seen in `meta.xml`.
 * Each item maps to the integer value of a language
 * in the LangOptions enum
 */
const std::string LANG_STRINGS[12] = {"en", "ja", "fr", "de", "it", "es", "zhs", "ko", "nl", "pt", "ru", "zht"};
