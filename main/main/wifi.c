/*
 * wifi.c
 * Author: Aaliya S Mohammed
 * Description: Wi-Fi station mode provisioning and
 *              connection management with auto-reconnect
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"

static const char *TAG = "WIFI";

/* Event group bits */
#define WIFI_CONNECTED_BIT   BIT0
#define WIFI_FAIL_BIT        BIT1

#define WIFI_MAX_RETRY       5

static EventGroupHandle_t wifi_event_group = NULL;
static int retry_count = 0;
static bool wifi_connected = false;

/*
 * wifi_event_handler()
 * Handles Wi-Fi and IP events — connection, disconnection, retry
 */
static void wifi_event_handler(void *arg,
                                esp_event_base_t event_base,
                                int32_t event_id,
                                void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        if (retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGW(TAG, "Wi-Fi disconnected — retry %d/%d",
                     retry_count, WIFI_MAX_RETRY);
        } else {
            ESP_LOGE(TAG, "Wi-Fi connection failed after %d retries",
                     WIFI_MAX_RETRY);
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }

    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Wi-Fi connected — IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));
        retry_count  = 0;
        wifi_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/*
 * wifi_init()
 * Initialize Wi-Fi in station mode and connect to AP
 *
 * ssid     : Wi-Fi network name
 * password : Wi-Fi password
 */
esp_err_t wifi_init(const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "Initializing Wi-Fi...");

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &wifi_event_handler, NULL, NULL));

    /* Set Wi-Fi credentials */
    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid,
            ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password,
            password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    /* Wait for connection or failure */
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(10000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wi-Fi connected successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Wi-Fi connection failed");
        return ESP_FAIL;
    }
}

/*
 * wifi_is_connected()
 * Returns true if Wi-Fi is currently connected
 */
bool wifi_is_connected(void)
{
    return wifi_connected;
}
