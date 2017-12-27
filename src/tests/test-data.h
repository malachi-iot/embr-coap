#pragma once

#include <stdint.h>

static uint8_t buffer_16bit_delta[] = {
        0x40, 0x00, 0x00, 0x00, // 4: fully blank header
        0xE1, // 5: option with delta 1 length 1
        0x00, // 6: delta ext byte #1
        0x01, // 7: delta ext byte #2
        0x03, // 8: value single byte of data
        0x12, // 9: option with delta 1 length 2
        0x04, //10: value byte of data #1
        0x05, //11: value byte of data #2
        0xFF, //12: denotes a payload presence
        // payload itself
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};

