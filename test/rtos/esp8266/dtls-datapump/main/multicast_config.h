#include "sdkconfig.h"

#define DEMO_WIFI_SSID      CONFIG_WIFI_SSID
#define DEMO_WIFI_PASSWORD  CONFIG_WIFI_PASSWORD

#define MSGBUFSIZE          1024

#define HELLO_PORT 1887                //multicast port
#define HELLO_GROUP "239.0.18.87"      //multicast ip

#define SEND_MODE      0              //1: enable send task.0:disable send task
#define RECV_MODE      0              //1: enable recv task,0:disable recv task
