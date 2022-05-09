/* Scheduler include files. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_system.h"
#include <esp_wifi.h>
#include "esp_log.h"
#include "driver/gpio.h"

#include "nvs.h"
#include "nvs_flash.h"


#include "ota_core.h"

#include "wifi_manager.h"


#define ESP_INTR_FLAG_DEFAULT 0
gpio_num_t gpio_contact_switch_num = GPIO_NUM_0;
static xQueueHandle gpio_evt_queue = NULL; // Transaktionswarteschlange

static EventGroupHandle_t event_group;


/**
 * @brief Main application task, it wait until OTA_TASK_IN_NORMAL_STATE.
 *
 * @param pvParameters Pointer to the task arguments
 */
static void main_application_task(void *pvParameters)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAGMAIN, "Starting Main Task  and wait for OTA Ready");
    while (1)
    {
        xEventGroupWaitBits(event_group, OTA_TASK_IN_NORMAL_STATE_EVENT, false, true, portMAX_DELAY);
        ESP_LOGI(TAGMAIN, "+++++++++++++++ Signed Application Release: +++++++++++++++");
        esp_app_desc_t running_app_info;
        if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) 
        {
            ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


static void gpio_task(void *arg)
{
    uint32_t io_num;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            xEventGroupSetBits(event_group, OTA_START_TRIGGER_EVENT);
            ESP_LOGI(TAGMAIN, "***** Trigger OTA Start *********");
        }
    }
}

static void ota_task(void *pvParameters)
{
	ESP_LOGI(TAG, "Starting OTA Core Task!!");
    enum STATE state = STATE_INIT; 

    while(1)
    {
       state = ota_core_task(&event_group, state);
    }
}

void cb_connection_ok(void *pvParameter){
    ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

    /* transform IP to human readable string */
    char str_ip[16];
    esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

    ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);
    xEventGroupSetBits(event_group, WIFI_CONNECTED_EVENT);

}


void app_main()
{
    esp_log_level_set("wifi", ESP_LOG_ERROR);
    esp_log_level_set("system_api", ESP_LOG_ERROR);
    esp_log_level_set("wifi_init", ESP_LOG_ERROR);
    esp_log_level_set("spi_flash", ESP_LOG_ERROR);
    esp_log_level_set("ota_core", ESP_LOG_DEBUG);      
    

	ESP_LOGI(TAG," Start Main App. To Trigger OTA Process please press BOOT Button <<<<");
    gpio_set_direction(gpio_contact_switch_num, GPIO_MODE_INPUT);
    gpio_set_intr_type(gpio_contact_switch_num, GPIO_INTR_POSEDGE);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    event_group = xEventGroupCreate();
    wifi_manager_start();
    /* register a callback as an example to how you can integrate your code with the wifi manager */
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
    
    xTaskCreate(&gpio_task, "gpio_task", 2048, NULL, 10, NULL);
	xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
	xTaskCreate(&main_application_task, "main_application_task", 8192, NULL, 5, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(gpio_contact_switch_num, gpio_isr_handler, (void *)gpio_contact_switch_num);

}
/*
void notify_wifi_connected()
{
    xEventGroupClearBits(event_group, WIFI_DISCONNECTED_EVENT);
    xEventGroupSetBits(event_group, WIFI_CONNECTED_EVENT);
}

void notify_wifi_disconnected()
{
    xEventGroupClearBits(event_group, WIFI_CONNECTED_EVENT);
    xEventGroupSetBits(event_group, WIFI_DISCONNECTED_EVENT);
}
*/