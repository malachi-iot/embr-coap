# Raspberry Pi Pico testing

## FreeRTOS

## Tests

These tests are all 1:1 with the esp32 tests of the same name

### Support Library

Assists in setting up and polling for LwIP/WiFi

## Results

|   Date  | Project      | Board                | PICO_SDK | FreeRTOS | Result | Notes |
| ------- | ------------ | -------------------- | -------  | -------- | ------ | ----- |
| 28DEC22 | boilerplate  | Raspberry Pi Pico W  | v1.4.0   |  none    | Pass   | Only returns 404 and 501s, but that counts
| 28DEC22 | echo         | Raspberry Pi Pico W  | v1.4.0   |  none    | Pass   | 
| 28DEC22 | echo-subject | Raspberry Pi Pico W  | v1.4.0   |  none    | Pass   | 
| 28DEC22 | unity        | Raspberry Pi Pico W  | v1.4.0   |  none    | N/A    | Not ready yet

