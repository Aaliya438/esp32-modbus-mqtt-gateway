# ESP32 Modbus-MQTT IoT Gateway

A production-inspired IoT gateway firmware built on **ESP32-C6** using **ESP-IDF** and **FreeRTOS** — bridging industrial Modbus RTU (RS485) devices to MQTT cloud telemetry.

## 🔧 Features

- Modbus RTU polling over RS485 (multiple slave devices)
- Real-time MQTT publishing to cloud broker
- FreeRTOS multitasking — separate tasks for polling, publishing, and watchdog
- AT command interface over UART for device configuration and diagnostics
- FOTA (Firmware Over-The-Air) update support over UART
- UDP multicast auto-discovery for zero-touch provisioning
- Alarm handling and heartbeat monitoring

## 🛠️ Tech Stack

| Component | Details |
|---|---|
| MCU | ESP32-C6-MINI-1-H8 |
| Framework | ESP-IDF v5.x |
| RTOS | FreeRTOS |
| Protocol | Modbus RTU over RS485 |
| Cloud | MQTT (HiveMQ / Mosquitto) |
| Language | Embedded C |

## 📁 Project Structure

```
esp32-modbus-mqtt-gateway/
├── main/
│   ├── modbus/        # Modbus RTU driver
│   ├── mqtt/          # MQTT client
│   ├── fota/          # OTA update handler
│   ├── at_cmd/        # AT command parser
│   └── main.c
├── CMakeLists.txt
└── README.md
```

## 🚧 Status

🔨 **In Progress** — Core Modbus + MQTT integration underway

## 👩‍💻 Author

**Aaliya S Mohammed**  
Firmware Engineer | Bengaluru, India  
[LinkedIn](https://linkedin.com/in/aaliya-s-mohammed-9575882a2)
