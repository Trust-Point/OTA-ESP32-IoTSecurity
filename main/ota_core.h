

#ifndef PRJ_MAIN_MODULE
#define PRJ_MAIN_MODULE

#include "freertos/event_groups.h"

#define HASH_LEN 32 /* SHA-256 digest length */


/*! Factory partiton label */
#define FACTORY_PARTITION_LABEL "factory"


// Event Definition

#define WIFI_CONNECTED_EVENT BIT0
#define WIFI_DISCONNECTED_EVENT BIT1
#define OTA_START_TRIGGER_EVENT BIT4
#define OTA_CONFIG_UPDATED_EVENT BIT5
#define OTA_TASK_IN_NORMAL_STATE_EVENT BIT6




static const char *TAG = "TRUST-POINT OTA: ";
static const char *TAGMAIN = "TRUST-POINT MAINTASK: ";



/**
 * @brief Set of states for @ref ota_task(void)
 */
enum STATE
{
    STATE_INIT,
    STATE_WAIT_WIFI,
    STATE_OTA_REQUEST,
    STATE_APP_LOOP,
    STATE_CONNECTION_IS_OK
};


/**
 * Macro to check the error code.
 * Prints the error code, error location, and the failed statement to serial output.
 * Unlike to ESP_ERROR_CHECK() method this macros abort the application's execution if it was built as 'release'.
 */
#define APP_ABORT_ON_ERROR(x)                                     \
    do                                                            \
    {                                                             \
        esp_err_t __err_rc = (x);                                 \
        if (__err_rc != ESP_OK)                                   \
        {                                                         \
            _esp_error_check_failed(__err_rc, __FILE__, __LINE__, \
                                    __ASSERT_FUNC, #x);           \
        }                                                         \
    } while (0);


/*! Updates application event bits on changing Wi-Fi state */
void notify_wifi_connected();
void notify_wifi_disconnected();


void diagnostic_partition_table(const char *partTableRunning,uint8_t lenPartTable);
enum STATE ota_core_task(EventGroupHandle_t *p_eventGrpHdl, enum STATE state );

#endif
