/**
 * The version number of the plugin.
 */
#define VERSION "v2.2"

/**
 * The oldest version of the pc app that the plugin completely supports, as a double.
 * Specifically, this version of the pc app should be able to support all features
 * of the current version of the plugin.
 */
#define COMPATIBLE_VERSION (double) 2.2

#pragma once

#include <string.h>

#include <padscore/wpad.h>

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
