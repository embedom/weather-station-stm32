/**
 ******************************************************************************
 * @file        : bme280_it.cpp
 * @author      : embedom
 * @date        : 2026-04-10
 * @brief       : Interrupts handling for SPI communication with BME280 sensor.
 ******************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <cstdint>
#include "hardware_config.h"
#include "bme280_sensor.hpp"

namespace BME280
{

/********************************* FUNCTIONS **********************************/

static SPI_HandleTypeDef *SPI_HANDLE = nullptr;
static DMA_HandleTypeDef *DMA_RX_HANDLE = nullptr;
static DMA_HandleTypeDef *DMA_TX_HANDLE = nullptr;
static Bme280Sensor *CLASS_INSTANCE = nullptr;

static void notifyDmaTransferComplete(void)
{
    if(CLASS_INSTANCE != nullptr)
    {
        CLASS_INSTANCE->onDmaComplete();
    }
}

static void notifyDmaTransferError(void)
{
    if(CLASS_INSTANCE != nullptr)
    {
        CLASS_INSTANCE->onDmaError();
    }
}

void Bme280Sensor::setInterruptCallbacksCfg(SPI_HandleTypeDef *SpiHandle,
                                            DMA_HandleTypeDef *RxDmaHandle,
                                            DMA_HandleTypeDef *TxDmaHandle,
                                            Bme280Sensor *ClassInstance)
{
    SPI_HANDLE = SpiHandle;
    DMA_RX_HANDLE = RxDmaHandle;
    DMA_TX_HANDLE = TxDmaHandle;
    CLASS_INSTANCE = ClassInstance;
}

extern "C" void DMA1_Stream3_IRQHandler(void)
{
    if(DMA_RX_HANDLE != nullptr)
    {
        HAL_DMA_IRQHandler(DMA_RX_HANDLE);
    }
}

extern "C" void DMA1_Stream4_IRQHandler(void)
{
    if(DMA_TX_HANDLE != nullptr)
    {
        HAL_DMA_IRQHandler(DMA_TX_HANDLE);
    }
}

extern "C" void SPI2_IRQHandler(void)
{
    if(SPI_HANDLE != nullptr)
    {
        HAL_SPI_IRQHandler(SPI_HANDLE);
    }
}

extern "C" void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if((hspi != nullptr) && (hspi == SPI_HANDLE))
    {
        notifyDmaTransferComplete();
    }
}

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if((hspi != nullptr) && (hspi == SPI_HANDLE))
    {
        notifyDmaTransferComplete();
    }
}

extern "C" void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if((hspi != nullptr) && (hspi == SPI_HANDLE))
    {
        notifyDmaTransferComplete();
    }
}

extern "C" void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if((hspi != nullptr) && (hspi == SPI_HANDLE))
    {
        notifyDmaTransferError();
    }
}

} //namespace BME280
