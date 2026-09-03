/*
 * mqtt_client.c
 * Author: Aaliya S Mohammed
 * Description: MQTT client handler — connects to broker,
 *              publishes telemetry data from Modbus devices
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT";

/* MQTT broker config */
#define MQTT_BROKER_URI     "mqtt://broker.hivemq.com"
#define MQTT_TOPIC_TELEMETRY "gateway/telemetry"
#define MQTT_TOPIC_STATUS    "gateway/status"
#define MQTT_QOS             1
#define MQTT_RETAIN          0

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

/*
 * mqtt_event_handler()
 * Handles MQTT connection, disconnection, and message events
 */
static void mqtt_event_handler(void *handler_args,
                                esp_event_base_t base,
                                int32_t event_id,
                                void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected to broker");
            mqtt_connected = true;
            /* Publish online status */
            esp_mqtt_client_publish(mqtt_client,
                                    MQTT_TOPIC_STATUS,
                                    "online", 0,
                                    MQTT_QOS,
                                    MQTT_RETAIN);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected — will retry");
            mqtt_connected = false;
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred");
            break;

        default:
            break;
    }
}

/*
 * mqtt_init()
 * Initialize and start the MQTT client
 */
esp_err_t mqtt_init(void)
{
    ESP_LOGI(TAG, "Initializing MQTT client...");

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client init failed");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(mqtt_client,
                                   ESP_EVENT_ANY_ID,
                                   mqtt_event_handler,
                                   NULL);

    esp_err_t err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MQTT client start failed");
        return err;
    }

    ESP_LOGI(TAG, "MQTT client started");
    return ESP_OK;
}

/*
 * mqtt_publish_telemetry()
 * Publish a JSON telemetry payload to the broker
 *
 * payload : JSON string e.g. {"voltage":230,"current":5.2}
 */
esp_err_t mqtt_publish_telemetry(const char *payload)
{
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected — skipping publish");
        return ESP_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_publish(mqtt_client,
                                          MQTT_TOPIC_TELEMETRY,
                                          payload, 0,
                                          MQTT_QOS,
                                          MQTT_RETAIN);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "MQTT publish failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Published telemetry: %s", payload);
    return ESP_OK;
}
