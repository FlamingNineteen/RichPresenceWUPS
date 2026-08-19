/**
 * The version number of the plugin.
 */
#define VERSION "v2.11"

#pragma once

#include <padscore/wpad.h>

/**
 * Options for controller display.
 */
enum DisplayOptions {
    // Do not display controller count
    NODISPLAY = 0,
    
    // Display the controller count, excluding the Gamepad
    CTRLCOUNTNODRC = 1,
    
    // Display the total controller count
    CTRLCOUNT = 2
};

/**
 * An array of all seven WPAD channels.
 */
const WPADChan WPAD_CHANS[7] = {WPAD_CHAN_0, WPAD_CHAN_1, WPAD_CHAN_2, WPAD_CHAN_3, WPAD_CHAN_4, WPAD_CHAN_5, WPAD_CHAN_6};
