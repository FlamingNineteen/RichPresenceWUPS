#define VERSION "v2.11"

#pragma once

#include <padscore/wpad.h>

enum DisplayOptions {
    NODISPLAY = 0, CTRLCOUNTNODRC = 1, CTRLCOUNT = 2
};

const WPADChan channels[7] = {WPAD_CHAN_0, WPAD_CHAN_1, WPAD_CHAN_2, WPAD_CHAN_3, WPAD_CHAN_4, WPAD_CHAN_5, WPAD_CHAN_6};
