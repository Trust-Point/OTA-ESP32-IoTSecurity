/**
 * @file wifi_service.h
 */

#ifndef PRJ_WIFI_MODULE
#define PRJ_WIFI_MODULE

#include "protocol_examples_common.h"


#define WIFI_SSID CONFIG_EXAMPLE_WIFI_SSID
#define WIFI_PASS CONFIG_EXAMPLE_WIFI_PASSWORD


void initialise_wifi(const char *running_partition_label);

#endif
