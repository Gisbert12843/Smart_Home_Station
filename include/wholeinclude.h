#pragma once

#include <algorithm>
#include <functional>
#include <inttypes.h>
#include <iostream>
#include <map>
#include "mutex"
#include <optional>
#include <stdio.h>
#include <string>
#include <string.h>
#include <tuple>
#include <utility>
#include <vector>
#include "memory"
#include <typeinfo>

#include "driver/gpio.h"


#include "esp_err.h"
#include "esp_event.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_task_wdt.h"


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"


#include "lvgl.h"

#include "dhcpserver/dhcpserver.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"


#include "nvs_flash.h"
#include "sdkconfig.h"
