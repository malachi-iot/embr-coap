# Overview

Requires `esp-idf` v4.x

## Results

These tests are all for variants of Espressif ESP32

|   Date  | Project      | Board                | Chip           | esp-idf  | Result   | Notes
| ------- | ------------ | -------------------- | -------------- | -------  | -------- | -----
| 18DEC22 | boilerplate  | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 26JAN23 | boilerplate  | Wemos D1             | ESP32-WROOM-32 | v5.0.0   | Pass     | 
| 17SEP23 | boilerplate  | RejsaCAN v3.1        | ESP32S3        | v5.1.1   | Pass     |
| 20SEP23 | boilerplate  | Seeed Xiao           | ESP32C3        | v5.1.1   | Pass     |
| 22APR22 | echo         |                      | ESP32S         | v4.4.0   | Pass     | 
| 09DEC22 | echo         | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 02JAN23 | echo         | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Pass     |
| 30DEC22 | echo-subject | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 26JAN23 | echo-subject | Wemos D1             | ESP32-WROOM-32 | v5.0.0   | Pass     | 
| 30DEC22 | observe      | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 13MAR23 | observe      | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.1   | Pass     | 
| 09DEC22 | retry        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Compiles | Doesn't do anything of interest yet
| 11JAN23 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     |
| 02JAN23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v4.4.3   | Pass     |
| 02JAN23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Fail     | Compiler requires LWIP_NETCONN_SEM_PER_THREAD configuration
| 13MAR23 | unity        | Wemos D1             | ESP32-WROOM-32 | v5.0.1   | Pass     | 
| 17SEP23 | unity        | RejsaCAN v3.1        | ESP32S3        | v5.1.1   | Pass     |

NOTE: `boilerplate` project is `boilerplate-subject`
