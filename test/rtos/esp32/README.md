# Overview

Requires `esp-idf` v4.x

## Results

These tests are all for variants of Espressif ESP32

|   Date  | Project      | Board                | Chip           | esp-idf  | Result   | Notes
| ------- | ------------ | -------------------- | -------------- | -------  | -------- | -----
| 25SEP24 | echo         |                      | ESP32          | v5.3.1   | Compiles | 
| 09DEC22 | echo         | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 02JAN23 | echo         | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Pass     |
| 25SEP24 | echo-subject | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.3.1   | Fail     | breaking change in tuple it seems
| 26JAN23 | echo-subject | Wemos D1             | ESP32-WROOM-32 | v5.0.0   | Pass     | 
| 10JAN24 | echo-subject | WaveShare DevKit     | ESP32C6        | v5.1.2   | Pass     | 
| 15JAN24 | echo-subject | Seeed Xiao           | ESP32S3        | v5.1.2   | Partial  | Gets IP.  Test paused for unrelated reasons
| 30DEC22 | observe      | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     | 
| 13MAR23 | observe      | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.0.1   | Pass     | 
| 18DEC22 | peripheral   | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v5.1.1   | Pass     | 
| 26JAN23 | peripheral   | Wemos D1             | ESP32-WROOM-32 | v5.0.0   | Pass     | 
| 17SEP23 | peripheral   | RejsaCAN v3.1        | ESP32S3        | v5.1.1   | Pass     |
| 24OCT23 | peripheral   | Seeed Xiao           | ESP32C3        | v5.1.1   | Pass     |
| 15JAN24 | peripheral   | Seeed Xiao           | ESP32S3        | v5.1.2   | Pass     |
| 08JAN24 | peripheral   | Lilygo QT Pro        | ESP32S3        | v5.1.2   | Pass     |
| 10JAN24 | peripheral   | WaveShare DevKit     | ESP32C6        | v5.1.2   | Pass     | 
| 09DEC22 | retry        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Compiles | Doesn't do anything of interest yet
| 11JAN23 | unity        | ESP-WROVER-KIT v4.1  | ESP32-WROVER-E | v4.4.3   | Pass     |
| 05OCT24 | unity        | Seeed Xiao           | ESP32C3        | v5.2.3   | Pass     |
| 02JAN23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v4.4.3   | Pass     |
| 02JAN23 | unity        | ESP32-C3-DevKitM-1   | ESP32C3        | v5.0.0   | Fail     | Compiler requires LWIP_NETCONN_SEM_PER_THREAD configuration
| 13MAR23 | unity        | Wemos D1             | ESP32-WROOM-32 | v5.0.1   | Pass     | 
| 17SEP23 | unity        | RejsaCAN v3.1        | ESP32S3        | v5.1.1   | Pass     |

NOTE: `peripheral` project used to be called `boilerplate-subject`
