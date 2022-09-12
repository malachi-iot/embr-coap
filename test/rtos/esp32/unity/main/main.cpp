#include <esp-helper.h>

#include "esp_system.h"
#include "esp_wifi.h"

#include "unity.h"

extern "C" void app_main()
{
    init_flash();
    
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    wifi_init_sta();
#else
    wifi_init_sta(event_handler);
#endif

    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();

    /* This function will not return, and will be busy waiting for UART input.
     * Make sure that task watchdog is disabled if you use this function.
     */
    unity_run_menu();    
}

