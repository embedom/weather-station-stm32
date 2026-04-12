/**
 *****************************************************************************
 * @file        : ds18b20.cpp
 * @author      : embedom
 * @date        : 2026-03-28
 * @brief       : DS18B20 one-wire (UART full-duplex) with DMA state machine.
 *****************************************************************************
 */

/********************************* INCLUDES **********************************/

#include <cstdint>
#include "misc_compiler.h"
#include "stm32f7xx_hal.h"
#include "hardware_config.h"
#include "ds18b20.hpp"
#include "SEGGER_RTT.h"

namespace DS18B20
{

/******************************** CONSTEXPR **********************************/

constexpr uint8_t RESET_FRAME_SIZE = 1U;
constexpr uint8_t CMD_RESET = 0xF0U;
constexpr uint8_t CMD_SKIP_ROM_INDEX = 0U;
constexpr uint8_t CMD_READ_ROM_INDEX = 1U;
constexpr uint8_t CMD_CONVERT_TEMP_INDEX = 2U;
constexpr uint8_t CMD_READ_SCRATCHPAD_INDEX = 3U;
constexpr uint8_t NUMBER_OF_COMMANDS = 4U;

constexpr uint8_t FULL_BYTE_HIGH = 0xFFU;
constexpr uint8_t MAX_DEVICE_ID_BITS = 64U;
constexpr uint8_t BITS_IN_BYTE = 0x08U;
constexpr uint8_t HIGH_BIT_MASK = 0xF0U;
constexpr uint8_t LOW_BIT_MASK = 0x00U;
constexpr uint8_t CRC_POLYNOMIAL = 0x8CU;

constexpr uint8_t SENSOR_CMD_ENCODE_FRAMES[NUMBER_OF_COMMANDS][FRAME_SIZE] = {
    { 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF }, //0xCC - SKIP_ROM
    { 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00 }, //0x33 - READ_ROM
    { 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00 }, //0x44 - CONVERT_T
    { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF }  //0xBE - READ_SCRATCH
};

constexpr uint8_t DUMMY_READ_TX_FRAME[FRAME_SIZE] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

constexpr uint8_t CMD_SEARCH_ROM_FRAME[FRAME_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF //0xF0 - SEARCH_ROM
};

constexpr uint32_t RESET_BAUDRATE = 9600U;
constexpr uint32_t DATA_BAUDRATE = 115200U;

constexpr uint32_t CONVERT_TEMP_TIME_MS = 800U;

constexpr uint8_t CMD_MATCH_ROM = 0x55U;
constexpr uint8_t CMD_SEARCH_ROM = 0xF0U;

/********************************* TYPEDEFS **********************************/

/********************************** PUBLIC ***********************************/

void DS18B20Sensor::initialize(void)
{
    if(_State == STATE_UNINITIALIZED)
    {
        HW_Status_t Status = HW_STATUS_OK;
        Status = HW_DS18B20_UartInit(&_UartHandle, &_UartRxDmaHandle, &_UartTxDmaHandle);
        if(Status != HW_STATUS_ERROR)
        {
            Status = HW_DS18B20_TimerInit(&_TimerHandle);
        }

        if(Status != HW_STATUS_ERROR)
        {
            setInterruptCallbacksCfg(this, &_UartHandle, &_TimerHandle);
        }

        if(setUartBaudrate(RESET_BAUDRATE) != Status::OK)
        {
            Error_Handler();
        }
        SEGGER_RTT_printf(0, "DS18B20 sensor initialized\n");
        _State = STATE_IDLE;
        // startSearchRom();
    }
}

void DS18B20Sensor::startMeasure(void)
{
    if(_State == STATE_IDLE && _NumSensorsFound > 0U)
    {
        _CurrentSensorIndex = 0U;
        if(startConversionSequence() != Status::OK)
        {
            setErrorAndIdle();
        }
    }
}

uint16_t DS18B20Sensor::getTemperature(uint8_t SensorIndex) const
{
    return _LastTempRaw[SensorIndex];
}

/********************************* PRIVATE ***********************************/

void DS18B20Sensor::handleTransactionComplete(void)
{
    Status Status = Status::OK;
    switch(_State)
    {
    case STATE_RESET_FOR_CONVERT:
    {
        if(_RxBuffer[0] == CMD_RESET)
        {
            setErrorAndIdle();
            return;
        }
        Status = setUartBaudrate(DATA_BAUDRATE);
        if(Status != Status::OK)
        {
            setErrorAndIdle();
            return;
        }
        _State = STATE_SKIP_ROM_FOR_CONVERT;
        Status = startDmaTransmitReceive(
            SENSOR_CMD_ENCODE_FRAMES[CMD_SKIP_ROM_INDEX], _RxBuffer, FRAME_SIZE);
        break;
    }
    case STATE_SKIP_ROM_FOR_CONVERT:
    {
        _State = STATE_CONVERT_T;
        Status = startDmaTransmitReceive(
            SENSOR_CMD_ENCODE_FRAMES[CMD_CONVERT_TEMP_INDEX], _RxBuffer, FRAME_SIZE);
        break;
    }
    case STATE_CONVERT_T:
    {
        _State = STATE_WAIT_CONVERSION;
        Status = startConversionTempTimer();
        break;
    }
    case STATE_RESET_FOR_READ:
    {
        if(_RxBuffer[0] == CMD_RESET)
        {
            setErrorAndIdle();
            return;
        }
        Status = setUartBaudrate(DATA_BAUDRATE);
        if(Status != Status::OK)
        {
            setErrorAndIdle();
            return;
        }
        _State = STATE_MATCH_ROM_FOR_READ;
        // Status = startDmaTransmitReceive(_MatchRomTxBuf[_CurrentSensorIndex], _RxBuffer, MATCH_ROM_FRAME_SIZE);
        Status = startDmaTransmitReceive(
            SENSOR_CMD_ENCODE_FRAMES[CMD_SKIP_ROM_INDEX], _RxBuffer, FRAME_SIZE);
        break;
    }
    case STATE_MATCH_ROM_FOR_READ:
    {
        _State = STATE_READ_SCRATCH_CMD;
        Status = startDmaTransmitReceive(
            SENSOR_CMD_ENCODE_FRAMES[CMD_READ_SCRATCHPAD_INDEX], _RxBuffer, FRAME_SIZE);
        break;
    }
    case STATE_READ_SCRATCH_CMD:
    {
        _ScratchIndex = 0U;
        _State = STATE_READ_BYTE;
        Status = startDmaTransmitReceive(DUMMY_READ_TX_FRAME, _RxBuffer, FRAME_SIZE);
        break;
    }
    case STATE_READ_BYTE:
    {
        _Scratchpad[_ScratchIndex] = decodeByte(_RxBuffer);
        _ScratchIndex++;

        if(_ScratchIndex < SCRATCHPAD_SIZE)
        {
            Status = startDmaTransmitReceive(DUMMY_READ_TX_FRAME, _RxBuffer, FRAME_SIZE);
        }
        else
        {
            uint8_t ScratchpadCrcReadValue = _Scratchpad[SCRATCHPAD_SIZE - 1U];
            if(calculateCrc(_Scratchpad, SCRATCHPAD_SIZE - 1U) == ScratchpadCrcReadValue)
            {
                _LastTempRaw[_CurrentSensorIndex] = static_cast<uint16_t>(
                    (static_cast<uint16_t>(_Scratchpad[1]) << BITS_IN_BYTE) | _Scratchpad[0]);
            }
            else
            {
                _LastTempRaw[_CurrentSensorIndex] = INVALID_TEMP;
            }
            _CurrentSensorIndex++;
            if(_CurrentSensorIndex < _NumSensorsFound)
            {
                if(startReadScratchpadSequence() != Status::OK)
                {
                    setErrorAndIdle();
                }
                return;
            }
            _State = STATE_IDLE;
            return;
        }
        break;
    }
    case STATE_SEARCHING:
    {
        handleSearchTransaction();
        return;
    }
    default:
    {
        setErrorAndIdle();
        return;
    }
    }

    if(Status != Status::OK)
    {
        setErrorAndIdle();
    }
}

void DS18B20Sensor::handleConversionTimerElapsed(void)
{
    if(_State != STATE_WAIT_CONVERSION)
    {
        return;
    }
    HAL_TIM_Base_Stop_IT(&_TimerHandle);

    if(startReadScratchpadSequence() != Status::OK)
    {
        setErrorAndIdle();
    }
}

void DS18B20Sensor::handleUartError(void)
{
    if(_State == STATE_SEARCHING)
    {
        processSearchComplete();
        return;
    }
    setErrorAndIdle();
}

Status DS18B20Sensor::setUartBaudrate(uint32_t BaudRate)
{
    _UartHandle.Init.BaudRate = BaudRate;
    return HAL_UART_Init(&_UartHandle) == HAL_OK ? Status::OK : Status::ERROR;
}

Status DS18B20Sensor::startDmaTransmitReceive(const uint8_t *TxBuffer, uint8_t *RxBuffer,
                                              uint16_t Length)
{
    if(HAL_UART_Receive_DMA(&_UartHandle, RxBuffer, Length) != HAL_OK)
    {
        return Status::ERROR;
    }
    if(HAL_UART_Transmit_DMA(&_UartHandle, TxBuffer, Length) != HAL_OK)
    {
        HAL_UART_DMAStop(&_UartHandle);
        return Status::ERROR;
    }
    return Status::OK;
}

void DS18B20Sensor::setErrorAndIdle(void)
{
    HAL_TIM_Base_Stop_IT(&_TimerHandle);
    for(uint8_t SensorIndex = 0U; SensorIndex < NUMBER_OF_SENSORS; SensorIndex++)
    {
        _LastTempRaw[SensorIndex] = INVALID_TEMP;
    }
    _State = STATE_IDLE;
    DEBUG_BRKPT();
}

Status DS18B20Sensor::startConversionSequence(void)
{
    Status Status = setUartBaudrate(RESET_BAUDRATE);
    if(Status != Status::OK)
    {
        return Status::ERROR;
    }
    _State = STATE_RESET_FOR_CONVERT;
    return startDmaTransmitReceive(&CMD_RESET, _RxBuffer, RESET_FRAME_SIZE);
}

Status DS18B20Sensor::startReadScratchpadSequence(void)
{
    Status Status = setUartBaudrate(RESET_BAUDRATE);
    if(Status != Status::OK)
    {
        return Status::ERROR;
    }
    _State = STATE_RESET_FOR_READ;
    return startDmaTransmitReceive(&CMD_RESET, _RxBuffer, RESET_FRAME_SIZE);
}

Status DS18B20Sensor::startConversionTempTimer(void)
{
    __HAL_TIM_SET_COUNTER(&_TimerHandle, 0U);
    __HAL_TIM_SET_AUTORELOAD(&_TimerHandle, CONVERT_TEMP_TIME_MS - 1U);
    __HAL_TIM_CLEAR_IT(&_TimerHandle, TIM_IT_UPDATE);
    return HAL_TIM_Base_Start_IT(&_TimerHandle) == HAL_OK ? Status::OK : Status::ERROR;
}

uint8_t DS18B20Sensor::calculateCrc(const uint8_t *Data, uint8_t Length)
{
    uint8_t Crc = 0U;
    while(Length > 0U)
    {
        uint8_t InByte = *Data++;
        for(uint8_t i = 0U; i < BITS_IN_BYTE; ++i)
        {
            const uint8_t Mix = (Crc ^ InByte) & 0x01U;
            Crc >>= 1U;
            if(Mix != 0U)
            {
                Crc ^= CRC_POLYNOMIAL;
            }
            InByte >>= 1U;
        }
        Length--;
    }
    return Crc;
}

uint8_t DS18B20Sensor::decodeByte(const uint8_t *Frame)
{
    uint8_t Value = 0U;
    for(uint8_t Index = 0U; Index < FRAME_SIZE; ++Index)
    {
        if(Frame[Index] > HIGH_BIT_MASK)
        {
            Value |= static_cast<uint8_t>(1U << Index);
        }
    }
    return Value;
}

void DS18B20Sensor::encodeByte(uint8_t Byte, uint8_t *Frame)
{
    for(uint8_t i = 0U; i < BITS_IN_BYTE; i++)
    {
        Frame[i] = (Byte & (1U << i)) ? 0xFFU : 0x00U; //NOLINT
    }
}

void DS18B20Sensor::encodeMatchRomFrames(void)
{
    for(uint8_t Sensor = 0U; Sensor < _NumSensorsFound; Sensor++)
    {
        encodeByte(CMD_MATCH_ROM, &_MatchRomTxBuf[Sensor][0]);
        for(uint8_t i = 0U; i < ROM_CODE_SIZE; i++)
        {
            encodeByte(_RomCodes[Sensor][i],
                       &_MatchRomTxBuf[Sensor][FRAME_SIZE + (i * FRAME_SIZE)]);
        }
    }
}

uint8_t DS18B20Sensor::getNumSensorsFound(void) const
{
    return _NumSensorsFound;
}

bool DS18B20Sensor::isSearchComplete(void) const
{
    return (_State != STATE_SEARCHING) && (_State != STATE_UNINITIALIZED);
}

void DS18B20Sensor::startSearchRom(void)
{
    _NumSensorsFound = 0U;
    _SearchLastDiscrepancy = 0U;
    _SearchLastDeviceFlag = false;
    for(uint8_t i = 0U; i < ROM_CODE_SIZE; i++)
    {
        _SearchRomCode[i] = 0U;
    }
    _State = STATE_SEARCHING;

    if(startSearchNextPass() != Status::OK)
    {
        processSearchComplete();
    }
}

Status DS18B20Sensor::startSearchNextPass(void)
{
    _SearchBitNumber = 0U;
    _SearchLastZero = 0U;
    _SearchState = SRCH_RESET;

    Status Status = setUartBaudrate(RESET_BAUDRATE);
    if(Status != Status::OK)
    {
        return Status::ERROR;
    }
    return startDmaTransmitReceive(&CMD_RESET, _RxBuffer, RESET_FRAME_SIZE);
}

void DS18B20Sensor::handleSearchTransaction(void)
{
    Status Status = Status::OK;

    switch(_SearchState)
    {
    case SRCH_RESET:
    {
        if(_RxBuffer[0] == CMD_RESET)
        {
            processSearchComplete();
            return;
        }
        Status = setUartBaudrate(DATA_BAUDRATE);
        if(Status != Status::OK)
        {
            processSearchComplete();
            return;
        }
        _SearchState = SRCH_CMD;
        Status = startDmaTransmitReceive(CMD_SEARCH_ROM_FRAME, _RxBuffer, FRAME_SIZE);
        break;
    }
    case SRCH_CMD:
    {
        _SearchBitNumber = 1U;
        _SearchLastZero = 0U;
        _SearchState = SRCH_READ_BITS;
        _SearchTxBuf[0] = FULL_BYTE_HIGH;
        _SearchTxBuf[1] = FULL_BYTE_HIGH;
        Status = startDmaTransmitReceive(_SearchTxBuf, _RxBuffer, 2U);
        break;
    }
    case SRCH_READ_BITS:
    {
        uint8_t IdBit = (_RxBuffer[0] > HIGH_BIT_MASK) ? 1U : 0U;
        uint8_t CmpIdBit = (_RxBuffer[1] > HIGH_BIT_MASK) ? 1U : 0U;

        if((IdBit == 1U) && (CmpIdBit == 1U))
        {
            processSearchComplete();
            return;
        }

        uint8_t SearchDirection;
        if(IdBit != CmpIdBit)
        {
            SearchDirection = IdBit;
        }
        else
        {
            if(_SearchBitNumber < _SearchLastDiscrepancy)
            {
                uint8_t ByteIdx = (_SearchBitNumber - 1U) / BITS_IN_BYTE;
                uint8_t BitIdx = (_SearchBitNumber - 1U) % BITS_IN_BYTE;
                SearchDirection = (_SearchRomCode[ByteIdx] >> BitIdx) & 0x01U;
            }
            else
            {
                SearchDirection = (_SearchBitNumber == _SearchLastDiscrepancy) ? 1U : 0U;
            }
            if(SearchDirection == 0U)
            {
                _SearchLastZero = _SearchBitNumber;
            }
        }

        uint8_t ByteIdx = static_cast<uint8_t>((_SearchBitNumber - 1U) / BITS_IN_BYTE);
        uint8_t BitIdx = static_cast<uint8_t>((_SearchBitNumber - 1U) % BITS_IN_BYTE);
        if(SearchDirection != 0U)
        {
            _SearchRomCode[ByteIdx] |= static_cast<uint8_t>(1U << BitIdx);
        }
        else
        {
            _SearchRomCode[ByteIdx] &= static_cast<uint8_t>(~(1U << BitIdx));
        }

        _SearchTxBuf[0] = (SearchDirection != 0U) ? FULL_BYTE_HIGH : 0x00U;
        _SearchState = SRCH_WRITE_DIR;
        Status = startDmaTransmitReceive(_SearchTxBuf, _RxBuffer, 1U);
        break;
    }
    case SRCH_WRITE_DIR:
    {
        _SearchBitNumber++;
        if(_SearchBitNumber <= MAX_DEVICE_ID_BITS)
        {
            _SearchState = SRCH_READ_BITS;
            _SearchTxBuf[0] = FULL_BYTE_HIGH;
            _SearchTxBuf[1] = FULL_BYTE_HIGH;
            Status = startDmaTransmitReceive(_SearchTxBuf, _RxBuffer, 2U);
        }
        else
        {
            _SearchLastDiscrepancy = _SearchLastZero;
            if(_SearchLastDiscrepancy == 0U)
            {
                _SearchLastDeviceFlag = true;
            }

            if(calculateCrc(_SearchRomCode, ROM_CODE_SIZE - 1U) ==
               _SearchRomCode[ROM_CODE_SIZE - 1U])
            {
                for(uint8_t i = 0U; i < ROM_CODE_SIZE; i++)
                {
                    _RomCodes[_NumSensorsFound][i] = _SearchRomCode[i];
                }
                _NumSensorsFound++;

                if(!_SearchLastDeviceFlag && (_NumSensorsFound < NUMBER_OF_SENSORS))
                {
                    Status = startSearchNextPass();
                }
                else
                {
                    processSearchComplete();
                    return;
                }
            }
            else
            {
                processSearchComplete();
                return;
            }
        }
        break;
    }
    default:
    {
        processSearchComplete();
        return;
    }
    }

    if(Status != Status::OK)
    {
        processSearchComplete();
    }
}

void DS18B20Sensor::processSearchComplete(void)
{
    _SearchState = SRCH_IDLE;
    _State = STATE_IDLE;
    if(_NumSensorsFound > 0U)
    {
        encodeMatchRomFrames();
    }
}

} //namespace DS18B20