/*
 * eeprom.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 1, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_EEPROM_H_
#define APP_INC_EEPROM_H_

#include "cmsis_os.h"
#include "saqTarget.h"
#include <stdint.h>

#ifdef STM32H743xx

#define macro_handler(T)                                                                                               \
    T(EEPROM_INIT_KEY)                                                                                                 \
    T(EEPROM_SERIAL_NUMBER)                                                                                            \
    T(EEPROM_HW_TYPE)                                                                                                  \
    T(EEPROM_HW_VERSION)                                                                                               \
    T(EEPROM_IP_ADDR)                                                                                                  \
    T(EEPROM_HTTP_PORT)                                                                                                \
    T(EEPROM_UDP_TX_PORT)                                                                                              \
    T(EEPROM_GATEWAY)                                                                                                  \
    T(EEPROM_NETMASK)                                                                                                  \
    T(EEPROM_SERVER_UDP_IP)                                                                                            \
    T(EEPROM_SERVER_UPD_PORT)                                                                                          \
    T(EEPROM_CLIENT_TCP_IP)                                                                                            \
    T(EEPROM_CLIENT_TCP_PORT)                                                                                          \
    T(EEPROM_IP_TX_DATA_TYPE)                                                                                          \
    T(EEPROM_UNIQUE_ID)                                                                                                \
    T(EEPROM_ADC_MODE)                                                                                                 \
    T(EEPROM_SENSOR_BOARD_0)                                                                                           \
    T(EEPROM_SENSOR_BOARD_1)                                                                                           \
    T(EEPROM_SENSOR_BOARD_2)                                                                                           \
    T(EEPROM_SENSOR_BOARD_3)                                                                                           \
    T(EEPROM_SENSOR_BOARD_4)                                                                                           \
    T(EEPROM_SENSOR_BOARD_5)                                                                                           \
    T(EEPROM_SENSOR_BOARD_6)                                                                                           \
    T(EEPROM_SENSOR_BOARD_7)                                                                                           \
    T(EEPROM_SENSOR_BOARD_8)                                                                                           \
    T(EEPROM_SENSOR_BOARD_9)                                                                                           \
    T(EEPROM_SENSOR_BOARD_10)                                                                                          \
    T(EEPROM_SENSOR_BOARD_11)                                                                                          \
    T(EEPROM_SENSOR_BOARD_12)                                                                                          \
    T(EEPROM_SENSOR_BOARD_13)                                                                                          \
    T(EEPROM_SENSOR_BOARD_14)                                                                                          \
    T(EEPROM_SENSOR_BOARD_15)                                                                                          \
    T(EEPROM_SENSOR_BOARD_16)                                                                                          \
    T(EEPROM_SENSOR_BOARD_17)                                                                                          \
    T(EEPROM_SENSOR_BOARD_18)                                                                                          \
    T(EEPROM_SENSOR_BOARD_19)                                                                                          \
    T(EEPROM_SENSOR_BOARD_20)                                                                                          \
    T(EEPROM_SENSOR_BOARD_21)                                                                                          \
    T(EEPROM_SENSOR_BOARD_22)                                                                                          \
    T(EEPROM_SENSOR_BOARD_23)                                                                                          \
    T(EEPROM_FAN_POP)                                                                                                  \
    T(NUM_EEPROM_REGISTERS) // must be last, indicates number of registers

#else
#define macro_handler(T)                                                                                               \
    T(EEPROM_INIT_KEY)                                                                                                 \
    T(EEPROM_SERIAL_NUMBER)                                                                                            \
    T(EEPROM_HW_TYPE)                                                                                                  \
    T(EEPROM_HW_VERSION)                                                                                               \
    T(EEPROM_ADC_MODE)                                                                                                 \
    T(EEPROM_IMU0_PRESENT)                                                                                             \
    T(EEPROM_IMU1_PRESENT)                                                                                             \
    T(EEPROM_IMU_BAUD)                                                                                                 \
    T(NUM_EEPROM_REGISTERS) // must be last, indicates number of registers

#endif

#ifdef CREATE_INSTANCE
GENERATE_ENUM_STRING_NAMES(macro_handler, EepromRegisters_e)
#else
extern const char **EepromRegisters_e_Strings;
#endif
GENERATE_ENUM_LIST(macro_handler, EepromRegisters_e)
#undef macro_handler

typedef void (*eeprom_read_cb_t)(EepromRegisters_e idx, int offset, uint8_t *const pData);
typedef void (*eeprom_write_cb_t)(EepromRegisters_e idx, int offset, uint8_t *const pData);

typedef struct EepromRegistersInfo {
    char *name;
    uint32_t addr;
    uint32_t size;
    char *type;
    eeprom_read_cb_t read_cb;
    eeprom_write_cb_t write_cb;
} EepromRegistersInfo_t;

extern const EepromRegistersInfo_t eepromRegisterLookupTable[NUM_EEPROM_REGISTERS];

/**
 * @fn eepromInit
 *
 * @brief Initialize the eeprom, test if eeprom has data
 */
void eepromInit(void);

/**
 * @brief Allow multiple processes to have mutex acccess to the eeprom
 * @note this assumes only one device on the I2C bus as that is the actual contention issue
 *
 * @param timeout_ms Amount of time to wait for access
 * @return mutex access result
 */
osStatus eepromOpen(uint32_t timeout_ms);

/**
 * @brief Release the eeprom mutex semaphore
 * @return mutex release result
 */
osStatus eepromClose(void);

/**
 * @fn ipStringToInt
 *
 * @brief Convert IP address format to uint32_t
 *
 * @param[in] char * str: IP string
 *
 *
 * @return IP address in uint32_t
 **/
uint32_t ipStringToInt(char *str);

/**
 * @fn getRegisterFromName
 *
 * @brief Get register ENUM given register name.
 *
 * @param[in] char * name: register name
 *
 *
 * @return Register ENUM on success
 *         -1 if register not found
 **/
int32_t getRegisterFromName(char *name);

/**
 * @fn readEepromRegister
 *
 * @brief Read a register value from the EEPROM.
 *
 * @param[in] EepromRegisters_e idx: Register to read from
 * @param[in] uint8_t * const pData: Buffer to read data into
 * @param[in] size_t pDataSz: size of data to read
 *
 * @return osStatus
 **/
osStatus eepromReadRegister(EepromRegisters_e idx, uint8_t *const pData, size_t pDataSz);

/**
 * @fn writeEepromRegister
 *
 * @brief Read a register value from the EEPROM.
 *
 * @param[in] EepromRegisters_e idx: Register to read from
 * @param[in] uint8_t * const pData: Buffer of data to write
 * @param[in] size_t pDataSz: size of data to write
 *
 * @return osStatus
 **/
osStatus eepromWriteRegister(EepromRegisters_e idx, uint8_t *const pData, size_t pDataSz);

/**
 * @fn eepromReadArrayRegister
 *
 * @brief Read a register value from the EEPROM.
 *
 * @param[in] EepromRegisters_e idx: Register to read from
 * @param[in] int offset: offset into array.
 * @param[in] uint8_t * const pData: Buffer to read data into
 * @param[in] size_t pDataSz: size of data to read
 *
 * @return osStatus
 **/
osStatus eepromReadArrayRegister(EepromRegisters_e idx, int offset, uint8_t *const pData, size_t pDataSz);

/**
 * @fn eepromWriteArrayRegister
 *
 * @brief Read a register value from the EEPROM.
 *
 * @param[in] EepromRegisters_e idx: Register to read from
 * @param[in] int offset: offset into array.
 * @param[in] uint8_t * const pData: Buffer of data to write
 * @param[in] size_t pDataSz: size of data to write
 *
 * @return osStatus
 **/
osStatus eepromWriteArrayRegister(EepromRegisters_e idx, int offset, uint8_t *const pData, size_t pDataSz);

#endif /* APP_INC_EEPROM_H_ */
