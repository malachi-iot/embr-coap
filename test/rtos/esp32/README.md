# Overview

Requires `esp-idf` v4.x

## Results

These tests are all for variants of Espressif ESP32

|   Date  | Project      | Board                | Chip           | esp-idf  | Result   | Notes
| ------- | ------------ | -------------------- | -------------- | -------  | -------- | -----
| 20SEP22 | boilerplate  |                      | ESP32S         | v4.4.2   | Pass     | 
| 18DEC22 | boilerplate  | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 22APR22 | echo         |                      | ESP32S         | v4.4.0   | Pass     | 
| 09DEC22 | echo         | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 02JAN23 | echo         | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Pass     |
| 01MAY22 | echo-subject |                      | ESP32S         | v4.4.0   | Pass     | 
| 30DEC22 | echo-subject | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 30DEC22 | observe      | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 06JAN23 | observe      | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.0   | Pass     | 
| 09DEC22 | retry        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Compiles | Doesn't do anything of interest yet
| 11JAN23 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     |
| 02JAN23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v4.4.3   | Pass     |
| 02JAN23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Fail     | Compiler requires LWIP_NETCONN_SEM_PER_THREAD configuration

NOTE: `boilerplate` project is `boilerplate-subject`
