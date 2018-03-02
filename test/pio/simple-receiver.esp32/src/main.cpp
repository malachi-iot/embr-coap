#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


extern "C" void coap_daemon(void *pvParameters);


extern "C" int app_main()
{
    xTaskCreate(coap_daemon, "COAP daemon", 2048, NULL, 2, NULL);

    return 0;
}