/**
 *   WIFI Service inclusiv Event Handling 
 *   supports the EVENT definiton 
 * 
 * 
 * 
 * 
 */


#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_service.h"
#include "ota_core.h"


/*! Buffer to save ESP32 MAC address */
uint8_t esp32_mac[6];

static esp_err_t esp_event_handler(void *ctx, system_event_t *event)
{
    assert(event != NULL);

    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        notify_wifi_connected();
        ESP_LOGI(TAG, "Connected to WI-FI, IP address: %s", ip4addr_ntoa((ip4_addr_t *)&event->event_info.got_ip.ip_info.ip));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        esp_wifi_connect();
        notify_wifi_disconnected();
        break;
    default:
        break;
    }
    return ESP_OK;
}





void initialise_wifi(const char *running_partition_label)
{
    assert(running_partition_label != NULL);

    tcpip_adapter_init();

    APP_ABORT_ON_ERROR(esp_event_loop_init(esp_event_handler, NULL));           // Create eventhandler Task
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();                        // Init config
    APP_ABORT_ON_ERROR(esp_wifi_init(&cfg));                                    // alloc WIFI resources
    APP_ABORT_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_FLASH));               // set Flash to init Wifi

    wifi_config_t wifi_config = {};
    APP_ABORT_ON_ERROR(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config));     // get configuration

    // Error Handling if wifi parameter not in falsh memory
    if (wifi_config.sta.ssid[0] == '\0' || wifi_config.sta.password[0] == '\0')
    {
        ESP_LOGW(TAG, "Flash memory doesn't contain any Wi-Fi credentials");
        if (strcmp(FACTORY_PARTITION_LABEL, running_partition_label) == 0)
        {
            ESP_LOGW(TAG, "Factory partition is running, Wi-Fi credentials from config are used and will be saved to the flash memory");
            wifi_sta_config_t wifi_sta_config = {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
            };

            wifi_config.sta = wifi_sta_config;
        }
        else
        {
            ESP_LOGE(TAG, "Wi-Fi credentials were not found, running partition is not '%s'", FACTORY_PARTITION_LABEL);
            APP_ABORT_ON_ERROR(ESP_FAIL);
        }
    }
    else
    {
        ESP_LOGI(TAG, "Wi-Fi credentials found in flash memory for SSID: %s ",wifi_config.sta.ssid);
    }


    APP_ABORT_ON_ERROR(esp_wifi_get_mac(ESP_IF_WIFI_STA, esp32_mac))           // Get MAC Address of NIC
    ESP_LOGI(TAG, "MAC address: %02X:%02X:%02X:%02X:%02X:%02X", esp32_mac[0], esp32_mac[1], esp32_mac[2], esp32_mac[3], esp32_mac[4], esp32_mac[5]);
    APP_ABORT_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA));                      // Set mode

    APP_ABORT_ON_ERROR(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));     // set configuration 
    APP_ABORT_ON_ERROR(esp_wifi_start());                                       // Start adapter
}
