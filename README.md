# Weather Station STM32

Firmware for a **STM32F746NG** weather station. The application runs on
**FreeRTOS**, periodically collects data from **DS18B20** and **BME280** sensors, exchanges data
between tasks through static FreeRTOS queues, and communicates with an HTTP server over Ethernet
and **LwIP**.

At the moment, DS18B20 temperature measurements are sent to the backend. BME280 data is read,
compensated, logged, and published through ITC; the BME280 HTTP endpoint is already defined in the
code and is intended for the next integration step.

## Key Features

- STM32F746NG, Cortex-M7, 216 MHz system clock from a 25 MHz HSE.
- FreeRTOS with statically allocated sensor and network tasks.
- DS18B20 over a One-Wire bus implemented with USART + DMA.
- BME280 over SPI + DMA, with temperature, humidity, and pressure readings.
- RMII Ethernet with LAN8742 PHY, LwIP in RTOS mode.
- Simple blocking HTTP/1.1 client built on a TCP transport layer.
- Inter-task communication through static FreeRTOS queues.
- Diagnostics through SEGGER RTT.
- CMake + Ninja + arm-none-eabi-gcc, with automatic tool bootstrapping.
- Clang-format and clang-tidy integrated with CMake.

## Architecture

The entry point is `Core/main.cpp`. Platform setup, HAL, RTT, ITC queues, and application tasks are
initialized from `Core/App/app_init.cpp`.

Main modules:

| Module | Files | Responsibility |
|---|---|---|
| Application init | `Core/App/app_init.*` | Platform startup, HAL, RTT, ITC, and FreeRTOS scheduler startup |
| Sensor task | `Core/App/Sensors/sensors_task.*` | Cyclic DS18B20 and BME280 reads every 2 s |
| DS18B20 | `Core/App/Sensors/ds18b20/*` | State-machine based One-Wire handling over UART + DMA |
| BME280 | `Core/App/Sensors/bme280/*` | Configuration, calibration, and measurement compensation over SPI + DMA |
| ITC | `Core/App/InterTaskCom/*` | Static FreeRTOS queues for measurement payloads |
| Network | `Core/App/Network/*` | lwIP, Ethernet, TCP transport, and HTTP client |
| Hardware configuration | `Core/Hardware/*` | Clocks, MPU, GPIO, DMA, ETH, USART6, SPI2, TIM7 |
| Application configuration | `Config/app_config.hpp` | Priorities, task cycles, device IP, and server address |

Data flow:

1. `SensorsTask` initializes DS18B20 and BME280.
2. After conversion completes, DS18B20 publishes temperature on `ItcChannel::DS18B20`.
3. BME280 publishes temperature, humidity, and pressure on `ItcChannel::BME280`.
4. `NetworkTask` waits for the Ethernet link, receives the DS18B20 and BME280 payloads from ITC, and sends it to the HTTP API.

## Application Configuration

The main application settings are in `Config/app_config.hpp`:

| Parameter | Current value |
|---|---|
| Device IP | `192.168.1.240` |
| Netmask | `255.255.255.0` |
| Gateway | `192.168.1.1` |
| Sensor task cycle | `2000 ms` |
| Network task cycle | `5000 ms` |
| DS18B20 count | `1` |
| BME280 count | `1` |

DNS and DHCP are not used at the moment. The TCP transport expects a textual IPv4 address.

## Toolchain

During CMake configuration, tools can be automatically downloaded into the `Tools/` directory.
Archives are fetched from `embedom/tools` releases and verified with SHA256.

| Tool | Version | CMake option |
|---|---:|---|
| ARM GNU Toolchain | `15.2.rel1` | `BOOTSTRAP_ARM_TOOLCHAIN` |
| Ninja | `1.13.2` | `BOOTSTRAP_NINJA` |
| clang-tidy / clang-format | `22.1.3` | `BOOTSTRAP_CLANG_TOOLS` |

Each bootstrap step can be disabled.

## Repository Layout

```text
CMake/                Tool bootstrap and dependency setup
Config/               FreeRTOS, HAL, RTT, and application configuration
Core/
  App/                Application logic, sensors, network, ITC
  Hardware/           HAL configuration, clocks, GPIO, DMA, ETH
  Startup/            STM32F746 startup code
  Terminal/           SEGGER RTT logging
Debug/                OpenOCD configuration
Drivers/              STM32F7 HAL and CMSIS
Middlewares/          FreeRTOS Kernel and lwIP
SeggerRTT/            SEGGER RTT sources
Tools/                Locally bootstrapped tools
```

## Integration Status

Implemented:

- Platform and FreeRTOS initialization,
- DS18B20 reads through UART/DMA,
- BME280 reads through SPI/DMA,
- ITC queues for DS18B20 and BME280,
- RMII Ethernet with lwIP,
- HTTP client and DS18B20, BME280 payload upload to the backend.

Planned next steps:

- Extend Weather Station Api according to backend (config fetch, update, user control params),
- Add an additional transport layer class using Mbed-TLS.
- Add a separate repository fetch containing the stand alone terminal implementation (SEGGER RTT)
- Add support for MQTT protocol
