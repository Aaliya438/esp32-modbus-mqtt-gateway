/*
 * fota.c
 * Author: Aaliya S Mohammed
 * Description: Firmware Over-The-Air (FOTA) update handler
 *              Receives binary over UART with throttled streaming
 *              Writes to OTA partition and switches boot partition
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "driver/uart.h"
#include "fota.h"

static const char *TAG = "FOTA";

#define FOTA_UART_PORT       UART_NUM_0
#define FOTA_CHUNK_SIZE      1024
#define FOTA_CHUNK_DELAY_MS  250
#define FOTA_TIMEOUT_MS      5000
#define FOTA_START_TOKEN     "FOTA_START"
#define FOTA_END_TOKEN       "FOTA_END"

/*
 * fota_run()
 * Main FOTA routine — receives binary stream over UART
 * and writes to the next OTA partition
 *
 * Flow:
 *   1. Wait for FOTA_START token
 *   2. Open next OTA partition for writing
 *   3. Receive binary in 1024-byte chunks with 250ms delay
 *      (delay prevents RX buffer overflow during flash erase ~100ms)
 *   4. Detect FOTA_END token → finalize and switch boot partition
 *   5. Reboot into new firmware
 */
esp_err_t fota_run(void)
{
    ESP_LOGI(TAG, "FOTA mode activated — waiting for binary stream...");

    /* Get next OTA partition */
    const esp_partition_t *update_partition =
        esp_ota_get_next_update_partition(NULL);

    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Writing to partition: %s at offset 0x%08lx",
             update_partition->label,
             update_partition->address);

    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(update_partition,
                                  OTA_WITH_SEQUENTIAL_WRITES,
                                  &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t  chunk[FOTA_CHUNK_SIZE];
    uint32_t total_written = 0;
    bool     fota_active   = true;

    uart_flush(FOTA_UART_PORT);

    while (fota_active) {
        int len = uart_read_bytes(FOTA_UART_PORT,
                                  chunk,
                                  FOTA_CHUNK_SIZE,
                                  pdMS_TO_TICKS(FOTA_TIMEOUT_MS));
        if (len <= 0) {
            ESP_LOGE(TAG, "FOTA timeout — no data received");
            esp_ota_abort(ota_handle);
            return ESP_ERR_TIMEOUT;
        }

        /* Check for end token */
        if (len >= (int)strlen(FOTA_END_TOKEN) &&
            memcmp(chunk, FOTA_END_TOKEN, strlen(FOTA_END_TOKEN)) == 0) {
            ESP_LOGI(TAG, "FOTA_END received — finalizing...");
            fota_active = false;
            break;
        }

        /* Write chunk to OTA partition */
        err = esp_ota_write(ota_handle, chunk, len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
            esp_ota_abort(ota_handle);
            return err;
        }

        total_written += len;
        ESP_LOGI(TAG, "Written %lu bytes so far...", total_written);

        /* Throttle delay — allows flash sector erase to complete
         * before next chunk arrives at 115200 baud             */
        vTaskDelay(pdMS_TO_TICKS(FOTA_CHUNK_DELAY_MS));
    }

    /* Finalize OTA */
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Switch boot partition */
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "FOTA complete! Total: %lu bytes — rebooting...",
             total_written);

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

    return ESP_OK;
}
