# Weather Station STM32

A weather station based on the **STM32F746NG** microcontroller. The project uses **FreeRTOS** for task management, **DS18B20** temperature sensor (One-Wire via UART + DMA), and the **lwIP** for network communication. Sensor data is exchanged between tasks through an ITC (Inter-Task Communication) mechanism built on FreeRTOS queues. Diagnostics are handled via **SEGGER RTT**.

## Tech Stack

- MCU: STM32F746NG (ARM Cortex-M7)
- RTOS: FreeRTOS
- Network: lwIP (tcp/ip) + http client
- Sensors: BME280 (SPI) and DS18B20 (One-Wire using UART)
- Build: CMake + Ninja + arm-none-eabi-gcc
- Code quality: clang-tidy + clang-format
- Debug: OpenOCD + SEGGER RTT

## Toolchain Bootstrap

All build tools are **automatically downloaded and verified** (SHA256) during CMake configure — no manual installation required. Binaries are fetched from [embedom/tools](https://github.com/embedom/tools/releases) and stored in the `Tools/` directory.

| Tool | Version | CMake option |
|---|---|---|
| ARM GNU Toolchain | 15.2.rel1 | `BOOTSTRAP_ARM_TOOLCHAIN` |
| Ninja | 1.13.2 | `BOOTSTRAP_NINJA` |
| clang-tidy / clang-format | 22.1.3 | `BOOTSTRAP_CLANG_TOOLS` |

Supported platforms: **macOS (ARM64)** and **Linux (x86_64)**.

All options are `ON` by default. To skip bootstrapping set corresponding option to `OFF`: