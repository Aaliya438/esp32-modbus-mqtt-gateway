/*
 * at_cmd.c
 * Author: Aaliya S Mohammed
 * Description: AT command interface over UART
 *              Handles device configuration, diagnostics,
 *              and field programming via serial console
 */

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "at_cmd.h"

static const char *TAG = "AT_CMD";

#define AT_UART_PORT     UART_NUM_0
#define AT_BUF_SIZE      256
#define AT_MAX_CMD_LEN   64

/* Supported AT commands */
#define CMD_STATUS       "AT+STATUS"
#define CMD_VER          "AT+VER"
#define CMD_REBOOT       "AT+REBOOT"
#define CMD_WIFI_SSID    "AT+WIFI_SSID"
#define CMD_WIFI_PASS    "AT+WIFI_PASS"
#define CMD_MB_PORT      "AT+MB_PORT"
#define CMD_MB_TIMEOUT   "AT+MB_TIMEOUT"
#define CMD_LOG_LEVEL    "AT+LOG_LEVEL"
#define CMD_IPCONFIG     "AT+IPCONFIG"
#define CMD_FACTORY      "AT+FACTORY"

/* Response helpers */
static void at_send_ok(void) {
    uart_write_bytes(AT_UART_PORT, "\r\nOK\r\n", 6);
}

static void at_send_error(void) {
    uart_write_bytes(AT_UART_PORT, "\r\nERROR\r\n", 9);
}

static void at_send_response(const char *msg) {
    uart_write_bytes(AT_UART_PORT, "\r\n", 2);
    uart_write_bytes(AT_UART_PORT, msg, strlen(msg));
    uart_write_bytes(AT_UART_PORT, "\r\n", 2);
    at_send_ok();
}

/*
 * at_parse_command()
 * Parse and execute an incoming AT command string
 */
static void at_parse_command(const char *cmd)
{
    ESP_LOGI(TAG, "Received: %s", cmd);

    if (strcmp(cmd, "AT") == 0) {
        at_send_ok();

    } else if (strcmp(cmd, CMD_STATUS) == 0) {
        at_send_response("STATUS=RUNNING");

    } else if (strcmp(cmd, CMD_VER) == 0) {
        at_send_response("VER=1.0.0");

    } else if (strcmp(cmd, CMD_IPCONFIG) == 0) {
        /* TODO: Return actual IP config */
        at_send_response("IP=192.168.1.100,GW=192.168.1.1");

    } else if (strncmp(cmd, CMD_WIFI_SSID, strlen(CMD_WIFI_SSID)) == 0) {
        /* TODO: Parse and save SSID to NVS */
        ESP_LOGI(TAG, "Set WIFI_SSID: %s", cmd + strlen(CMD_WIFI_SSID) + 1);
        at_send_ok();

    } else if (strncmp(cmd, CMD_WIFI_PASS, strlen(CMD_WIFI_PASS)) == 0) {
        /* TODO: Parse and save password to NVS */
        at_send_ok();

    } else if (strncmp(cmd, CMD_MB_PORT, strlen(CMD_MB_PORT)) == 0) {
        /* TODO: Set Modbus port number */
        at_send_ok();

    } else if (strncmp(cmd, CMD_MB_TIMEOUT, strlen(CMD_MB_TIMEOUT)) == 0) {
        /* TODO: Set Modbus timeout value */
        at_send_ok();

    } else if (strncmp(cmd, CMD_LOG_LEVEL, strlen(CMD_LOG_LEVEL)) == 0) {
        /* TODO: Set ESP log level */
        at_send_ok();

    } else if (strcmp(cmd, CMD_FACTORY) == 0) {
        /* TODO: Erase NVS and reboot */
        ESP_LOGW(TAG, "Factory reset triggered!");
        at_send_ok();

    } else if (strcmp(cmd, CMD_REBOOT) == 0) {
        at_send_ok();
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();

    } else {
        ESP_LOGW(TAG, "Unknown command: %s", cmd);
        at_send_error();
    }
}

/*
 * at_cmd_task()
 * FreeRTOS task — reads UART and processes AT commands
 */
void at_cmd_task(void *pvParameters)
{
    uint8_t buf[AT_BUF_SIZE];
    char    cmd[AT_MAX_CMD_LEN];
    int     cmd_len = 0;

    ESP_LOGI(TAG, "AT command task started");

    while (1) {
        int len = uart_read_bytes(AT_UART_PORT,
                                  buf, sizeof(buf),
                                  pdMS_TO_TICKS(100));
        for (int i = 0; i < len; i++) {
            char c = (char)buf[i];
            if (c == '\r' || c == '\n') {
                if (cmd_len > 0) {
                    cmd[cmd_len] = '\0';
                    at_parse_command(cmd);
                    cmd_len = 0;
                }
            } else if (cmd_len < AT_MAX_CMD_LEN - 1) {
                cmd[cmd_len++] = c;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
