########################### STM32 drivers config ##############################
set(STM32F7xx_HAL_DIR_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F7xx_HAL)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/
    ${STM32F7xx_HAL_DIR_PATH}/Inc
    ${STM32F7xx_HAL_DIR_PATH}/Inc/Legacy
)

set(HAL_SOURCES
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_cortex.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_tim.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_tim_ex.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_uart.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_uart_ex.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_rcc.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_rcc_ex.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_flash.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_flash_ex.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_gpio.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_dma.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_dma_ex.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_pwr.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_pwr_ex.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_spi.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_spi_ex.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_exti.c
    ${STM32F7xx_HAL_DIR_PATH}/Src/stm32f7xx_hal_eth.c
)

# Suppress compiler warnings for third-party drivers sources
set_source_files_properties(${HAL_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")