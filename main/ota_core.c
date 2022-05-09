/**
 * @file ota_core.c
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "ota_core.h"
//#include "wifi_service.h"
#include "cJSON.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");
#define BUFFSIZE 1024
#define OTA_URL_SIZE 256
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };


/**
 * @brief print_sha256  service function to print the hash value
 *
 * @param image_hash : The array with the hash value
 * @param label : The array with the label printe befor hash value
 */
static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

/**
 * @brief diagnostic_partition_table  get Partition information and print information
 *
 * @param partTableRunning : pointer to store the Running Partion
 * @param lenPartTable : lenght of the partTableRunning
 */
void diagnostic_partition_table(const char *partTableRunning, uint8_t lenPartTable)
{
	uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;
    
    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");
    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    strncpy(partTableRunning, running_partition->label, lenPartTable);
    ESP_LOGI(TAG, "Running partition: %s", partTableRunning);

    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running_partition, &ota_state) == ESP_OK) 
    {
    	ESP_LOGI(TAG, "Ready to run");	
    }
    else
    {
    	ESP_LOGI(TAG, " Clean partition Table");	
    }
}


static enum STATE connection_state(BaseType_t actual_event, const char *current_state_name)
{
    assert(current_state_name != NULL);

    if (actual_event & WIFI_DISCONNECTED_EVENT)
    {
        ESP_LOGE(TAG, "%s state, Wi-Fi not connected, wait for the connect", current_state_name);
        return STATE_WAIT_WIFI;
    }

    return STATE_CONNECTION_IS_OK;
}


static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}



static void __attribute__((noreturn)) task_fatal_error(void)
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);

    while (1) {
        ;
    }
}

static void infinite_loop(void)
{
    int i = 0;
    ESP_LOGI(TAG, "When a new firmware is available on the server, press the reset button to download it");
    while(1) 
    {
        ESP_LOGI(TAG, "Waiting for a new firmware ... %d", ++i);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void ota_service()
{
    esp_err_t err;
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    ESP_LOGI(TAG, "Starting OTA");
    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    esp_http_client_config_t config = 
    {
        .url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = 15000,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTPS connection");
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Etablished Connection to Update Server: %s",CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL);

    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTPS connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);
    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    // Now lets start with the Download of the new App
    int binary_file_length = 0;
    bool image_header_was_checked = false;

    while (1) 
    {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) 
        {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) 
        {
            if (image_header_was_checked == false) 
            {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) 
                {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) 
                    {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }
                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) 
                    {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) 
                    {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) 
                        {
                            ESP_LOGW(TAG, "New version is the same as invalid version.");
                            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                            http_cleanup(client);
                            infinite_loop();
                        }
                    }
#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                        http_cleanup(client);
                        infinite_loop();
                    }
#endif
                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
                    if (err != ESP_OK) 
                    {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        esp_ota_abort(update_handle);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else 
                {
                    ESP_LOGE(TAG, "received package is not fit len %d",data_read);
                    http_cleanup(client);
                    esp_ota_abort(update_handle);
                    task_fatal_error();
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) 
            {
                http_cleanup(client);
                esp_ota_abort(update_handle);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGI(TAG, " \b/ Written image length %d", binary_file_length);
        } else if (data_read == 0) 
        {
           /*
            * As esp_http_client_read never returns negative error code, we rely on
            * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) 
            {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) 
            {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) 
    {
        ESP_LOGE(TAG, "Error in receiving complete file");
        http_cleanup(client);
        esp_ota_abort(update_handle);
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    return ;
}



/**
 * @brief diagnostic_partition_table  get Partition information and print information
 *
 * @param partTableRunning : pointer to store the Running Partion
 * @param lenPartTable : lenght of the partTableRunning
 */
enum STATE ota_core_task(EventGroupHandle_t *p_eventGrpHdl, enum STATE state)
{
    enum STATE current_connection_state = STATE_CONNECTION_IS_OK;
    char running_partition_label[sizeof(((esp_partition_t *)0)->label)];
    uint8_t lenPartTbl = sizeof(running_partition_label);
    BaseType_t actual_event = 0x00;


    if (state != STATE_INIT)
    {
        if (state != STATE_APP_LOOP)
        {
            xEventGroupClearBits(*p_eventGrpHdl, OTA_TASK_IN_NORMAL_STATE_EVENT);
        }
        actual_event = xEventGroupWaitBits(*p_eventGrpHdl,
                                           WIFI_CONNECTED_EVENT | WIFI_DISCONNECTED_EVENT | OTA_START_TRIGGER_EVENT,
                                           false, false, portMAX_DELAY);
    }

 	switch (state)
    {
    	case STATE_INIT:
        {
        	ESP_LOGI(TAG,"STATE INIT");
            xEventGroupClearBits(*p_eventGrpHdl, OTA_TASK_IN_NORMAL_STATE_EVENT);
            diagnostic_partition_table(running_partition_label,lenPartTbl);
            esp_err_t err = nvs_flash_init();
            if (err == ESP_ERR_NVS_NO_FREE_PAGES)
            {
                	// OTA app partition table has a smaller NVS partition size than the non-OTA
                	// partition table. This size mismatch may cause NVS initialization to fail.
                	// If this happens, we erase NVS partition and initialize NVS again.
              	APP_ABORT_ON_ERROR(nvs_flash_erase());
               	err = nvs_flash_init();
            }
            APP_ABORT_ON_ERROR(err);
            // initialise_wifi(running_partition_label);
            ESP_LOGI(TAG,"set to STATE_WAIT_WIFI");
            state = STATE_WAIT_WIFI;
            break;
        }
        case STATE_WAIT_WIFI:
        {
            if (actual_event & WIFI_DISCONNECTED_EVENT)
            {
                ESP_LOGI(TAG, "STATE_WAIT_WIFI state, Wi-Fi not connected, wait for the connect");
                state = STATE_WAIT_WIFI;
                break;
            }
            if (actual_event & WIFI_CONNECTED_EVENT)
            {
                ESP_LOGI(TAG, "STATE_WAIT_WIFI state, Wi-Fi connected, set to STATE_APP_LOOP ");
                state = STATE_APP_LOOP;
                xEventGroupSetBits(*p_eventGrpHdl, OTA_TASK_IN_NORMAL_STATE_EVENT);
                break;
            }
        }

        case STATE_APP_LOOP:
        {
            current_connection_state = connection_state(actual_event,"STATE_APP_LOOP");
            if (current_connection_state != STATE_CONNECTION_IS_OK)
            {
                state = current_connection_state;
                break;
            }
            if(actual_event & OTA_START_TRIGGER_EVENT)
            {
                ESP_LOGD(TAG,"STATE APP_LOOP Trigger OTA ");
                xEventGroupClearBits(*p_eventGrpHdl, OTA_START_TRIGGER_EVENT);                
                state = STATE_OTA_REQUEST;
                break;
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        	state = STATE_APP_LOOP;
        	break;
        }
        default:
        {
           	ESP_LOGE(TAG, "Unexpected state %i",state);
        	//state = STATE_INIT;
            break;
        }
        case STATE_OTA_REQUEST:
        {
            current_connection_state = connection_state(actual_event,"STATE_APP_LOOP");
            if (current_connection_state != STATE_CONNECTION_IS_OK)
            {
                state = current_connection_state;
                break;
            }
            ESP_LOGD(TAG,"STATE_OTA_REQUEST Start OTA ");
            ota_service();
            state = STATE_APP_LOOP;
            break;
        }
    }
    //vTaskDelay(1000 / portTICK_PERIOD_MS);
    return state;
}





