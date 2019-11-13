#include <esp-helper.h>

#include "esp_system.h"
#include "esp_wifi.h"

extern "C" void app_main()
{
    init_flash();
    
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    wifi_init_sta();
#else
    wifi_init_sta(event_handler);
#endif
}

