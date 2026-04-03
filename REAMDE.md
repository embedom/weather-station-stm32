# Weather Station STM32

A weather station based on the **STM32F746NG** microcontroller. The project uses **FreeRTOS** for task management, **DS18B20** temperature sensor (One-Wire via UART + DMA), and the **lwIP** for network communication. Sensor data is exchanged between tasks through an ITC (Inter-Task Communication) mechanism built on FreeRTOS queues. Diagnostics are handled via **SEGGER RTT**.

## Tech Stack

- MCU: STM32F746NG (ARM Cortex-M7)
- RTOS: FreeRTOS
- Network: lwIP (tcp/ip) + http client
- Sensor: DS18B20 (One-Wire / UART Half-Duplex + DMA)
- Build: CMake + arm-none-eabi-gcc
- Debug: OpenOCD + SEGGER RTT