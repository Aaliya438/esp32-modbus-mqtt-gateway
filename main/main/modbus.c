/*
 * modbus.c
 * Author: Aaliya S Mohammed
 * Description: Modbus RTU driver over RS485 (UART)
 *              Handles slave polling, register reading,
 *              and error recovery
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "modbus.h"

static const char *TAG = "MODBUS";

/* UART and RS485 config */
#define MODBUS_UART_PORT    UART_NUM_1
#define MODBUS_BAUD_RATE    9600
#define MODBUS_TX_PIN       GPIO_NUM_17
#define MODBUS_RX_PIN       GPIO_NUM_16
#define MODBUS_DE_PIN       GPIO_NUM_0   /* RS485 Driver Enable */
#define MODBUS_BUF_SIZE     256
#define MODBUS_TIMEOUT_MS   500

/* Modbus function codes */
#define MB_FC_READ_HOLDING_REGS  0x03

/*
 * modbus_init()
 * Initialize UART for Modbus RTU over RS485
 */
esp_err_t modbus_init(void)
{
    ESP_LOGI(TAG, "Initializing Modbus RTU over RS485...");

    uart_config_t uart_config = {
        .baud_rate  = MODBUS_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };

    esp_err_t err;

    err = uart_param_config(MODBUS_UART_PORT, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed");
        return err;
    }

    err = uart_set_pin(MODBUS_UART_PORT,
                       MODBUS_TX_PIN,
                       MODBUS_RX_PIN,
                       MODBUS_DE_PIN,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed");
        return err;
    }

    err = uart_driver_install(MODBUS_UART_PORT,
                              MODBUS_BUF_SIZE * 2,
                              0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed");
        return err;
    }

    /* Set RS485 half-duplex mode */
    uart_set_mode(MODBUS_UART_PORT, UART_MODE_RS485_HALF_DUPLEX);

    ESP_LOGI(TAG, "Modbus RTU initialized successfully");
    return ESP_OK;
}

/*
 * modbus_read_holding_registers()
 * Poll a Modbus slave for holding registers
 *
 * slave_addr : Modbus slave address (1–247)
 * start_reg  : Starting register address
 * num_regs   : Number of registers to read
 * out_data   : Output buffer for register values
 */
esp_err_t modbus_read_holding_registers(uint8_t  slave_addr,
                                         uint16_t start_reg,
                                         uint8_t  num_regs,
                                         uint16_t *out_data)
{
    if (out_data == NULL) return ESP_ERR_INVALID_ARG;

    /* Build Modbus RTU request frame */
    uint8_t request[8];
    request[0] = slave_addr;
    request[1] = MB_FC_READ_HOLDING_REGS;
    request[2] = (start_reg >> 8) & 0xFF;
    request[3] = start_reg & 0xFF;
    request[4] = 0x00;
    request[5] = num_regs;

    /* TODO: Append CRC16 */

    /* Send request over RS485 */
    uart_flush(MODBUS_UART_PORT);
    int sent = uart_write_bytes(MODBUS_UART_PORT,
                                (const char *)request,
                                sizeof(request));
    if (sent < 0) {
        ESP_LOGE(TAG, "UART write failed");
        return ESP_FAIL;
    }

    /* Wait for response */
    uint8_t response[MODBUS_BUF_SIZE];
    int len = uart_read_bytes(MODBUS_UART_PORT,
                              response,
                              sizeof(response),
                              pdMS_TO_TICKS(MODBUS_TIMEOUT_MS));
    if (len <= 0) {
        ESP_LOGW(TAG, "Slave 0x%02X timeout — no response", slave_addr);
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "Slave 0x%02X responded — %d bytes", slave_addr, len);

    /* TODO: Validate CRC and parse register values */

    return ESP_OK;
}
