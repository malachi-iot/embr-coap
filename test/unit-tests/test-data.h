#pragma once

#include <stdint.h>

static uint8_t buffer_16bit_delta[] = {
        0x40, 0x01, 0x01, 0x23, // 4: mid = 123, tkl = 0, GET
        0xE1, // 5: option with 16-bit delta 1 length 1
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


static uint8_t buffer_plausible[] = {
        0x40, 0x00, 0x00, 0x00, // 4: fully blank header
        0xB4, // 5: option delta 11 (Uri-Path) with value length of 4
        'T',
        'E',
        'S',
        'T',
        0x03, // 9: option delta 0 (Uri-Path, again) with value length of 3
        'P', 'O', 'S',
        0xFF,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};

static uint8_t buffer_with_token[] = {
    0x42, 0x01, 0x01, 0x23, // 4: mid = 123, tkl = 2, GET
    0x77, 0x78,             // 2: token of 0x7778

    0xFF,
    'h', 'i'
};


const static uint8_t buffer_payload_only[] =
{
    0x40, 0x00, 0x00, 0x00, // 4: fully blank header
    0xFF,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};


const static uint8_t buffer_oversimplified_observe[] =
{
    0x40, 0x00, 0x01, 0x23, // 4: mid = 123, tkl = 0, GET
    0x60    // the most minimal registration possible. a 0-length uint == 0.  clever
};

const static uint8_t buffer_simplest_request[] =
{
    0x40, 0x00, 0x01, 0x23, // mid = 123, tkl = 0, GET, CON
};


const static uint8_t buffer_ack[] =
{
    0x60, 0x00, 0x01, 0x23  // mid = 123, tkl = 0, GET, ACK
};



static uint8_t cbor_cred[] =
{
0xA2,                       // map(2)
    0x64,                    // text(4)
        0x73, 0x73, 0x69, 0x64,          // "ssid"
    0x69,                    // text(9)
        0x73, 0x73, 0x69, 0x64, 0x5F, 0x6E, 0x61, 0x6D, 0x65, // "ssid_name"
    0x64,                    // text(4)
        0x70, 0x61, 0x73, 0x73,           // "pass"
    0x66,                    // text(6)
        0x73, 0x65, 0x63, 0x72, 0x65, 0x74        // "secret"
};

static uint8_t cbor_int[] =
{
    0xA1,             // map(1)
    0x63,          // text(3)
        0x76, 0x61, 0x6C,   // "val"
    0x3A, 0x07, 0x5B, 0xCD, 0x14 // negative(123456788)
};

