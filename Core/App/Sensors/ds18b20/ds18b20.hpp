/**
 ******************************************************************************
 * @file        : ds18b20.hpp
 * @author      : embedom
 * @date        : 2026-03-28
 * @brief       : Description
 ******************************************************************************
 */

#pragma once

/********************************* INCLUDES **********************************/

#include <stdio.h>
#include <cstdint>

#include "app_config.hpp"
#include "stm32f7xx_hal.h"

namespace DS18B20
{

/******************************** CONSTEXPR **********************************/

constexpr uint8_t FRAME_SIZE = 8U;
constexpr uint8_t ROM_CODE_SIZE = 8U;
constexpr uint8_t SCRATCHPAD_SIZE = 9U;
constexpr uint8_t MAX_POSSIBLE_FRAME_SIZE = 72U;
constexpr int16_t INVALID_TEMP = 0x7FFF; /* Sentinel value for invalid temperature */

/********************************* TYPEDEFS **********************************/

enum State : uint8_t
{
    STATE_UNINITIALIZED = 0,
    STATE_IDLE,
    STATE_SEARCHING,
    STATE_RESET_FOR_CONVERT,
    STATE_SKIP_ROM_FOR_CONVERT,
    STATE_CONVERT_T,
    STATE_WAIT_CONVERSION,
    STATE_RESET_FOR_READ,
    STATE_SKIP_ROM_FOR_READ,
    STATE_SEND_MATCH_ROM_FOR_READ,
    STATE_MATCH_ROM_FOR_READ,
    STATE_READ_SCRATCH_CMD,
    STATE_READ_SCRATCH_CPLT
};

enum SearchState : uint8_t
{
    SRCH_IDLE = 0,
    SRCH_RESET,
    SRCH_CMD,
    SRCH_READ_BITS,
    SRCH_WRITE_DIR,
    SRCH_SEQ_COMPLETE
};

enum Status : uint8_t
{
    OK = 0,
    ERROR
};

/*********************************** CLASS ***********************************/

class DS18B20Sensor
{
    public:
    DS18B20Sensor() = default;
    ~DS18B20Sensor() = default;
    /**************************** Public Members *****************************/
    void initialize(void);
    void startMeasure(void);
    int16_t getTemperature(uint8_t SensorIndex) const;
    uint8_t getNumSensorsFound(void) const;
    bool isSensorReady(void);

    /* UART/Timer connected functions */
    void handleUartRxDmaComplete(void);
    void handleUartTxDmaComplete(void);
    void handleConversionTimerElapsed(void);
    void handleUartError(void);

    /**************************** Private Members ****************************/
    private:
    State _State = STATE_UNINITIALIZED;
    int16_t _LastTempCelsius[NUMBER_OF_DS18B20_SENSORS] = { INVALID_TEMP };
    uint8_t _ScratchIndex = 0U;
    uint8_t _Scratchpad[SCRATCHPAD_SIZE] = { 0U };
    uint8_t _RxBuffer[MAX_POSSIBLE_FRAME_SIZE] = { 0U };
    uint8_t _NumSensorsFound = 0U;
    uint8_t _CurrentSensorIndex = 0U;

#ifdef ENABLE_DS18B20_ROM_SEARCH
    SearchState _SearchState = SRCH_IDLE;
    uint8_t _RomCodes[NUMBER_OF_DS18B20_SENSORS][ROM_CODE_SIZE] = {};
    uint8_t _MatchRomTxBuf[NUMBER_OF_DS18B20_SENSORS][MAX_POSSIBLE_FRAME_SIZE] = {};
    uint8_t _SearchBitNumber = 0U;
    uint8_t _SearchLastDiscrepancy = 0U;
    uint8_t _SearchLastZero = 0U;
    bool _SearchLastDeviceFlag = false;
    uint8_t _SearchRomCode[ROM_CODE_SIZE] = { 0U };
    uint8_t _SearchTxBuf[2U] = { 0U };
#endif

    /* UART/Timer connected variables */
    volatile bool _UartRxDmaDone = false;
    volatile bool _UartTxDmaDone = false;
    UART_HandleTypeDef _UartHandle = {};
    DMA_HandleTypeDef _UartRxDmaHandle = {};
    DMA_HandleTypeDef _UartTxDmaHandle = {};
    TIM_HandleTypeDef _TimerHandle = {};

    void setInterruptCallbacksCfg(DS18B20Sensor *Instance, UART_HandleTypeDef *UartHandle,
                                  TIM_HandleTypeDef *TimHandle);
    void handleTransactionComplete(void);
    uint8_t calculateCrc(const uint8_t *Data, uint8_t Length);
    uint8_t decodeByte(const uint8_t *Frame);
    void encodeByte(uint8_t Byte, uint8_t *Frame);
    void encodeMatchRomFrames(void);
    Status setUartBaudrate(uint32_t BaudRate);
    Status startDmaTransmitReceive(const uint8_t *TxBuffer, uint8_t *RxBuffer, uint16_t Length);
    Status startConversionSequence(void);
    Status startReadScratchpadSequence(void);
    Status startConversionTempTimer(void);
    void setErrorAndIdle(void);

#ifdef ENABLE_DS18B20_ROM_SEARCH
    void startSearchRom(void);
    Status startSearchNextPass(void);
    void handleSearchTransaction(void);
    void processSearchComplete(void);
#endif

}; //class DS18B20Sensor

} //namespace DS18B20