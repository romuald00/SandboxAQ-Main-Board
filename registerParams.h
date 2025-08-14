/*
 * registerParams.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 5, 2023
 *      Author: rlegault
 */

#ifndef LIB_INC_REGISTERPARAMS_H_
#define LIB_INC_REGISTERPARAMS_H_

typedef struct registerInfo *registerInfo_tp;

#include "board_registerParams.h"
#include "cli/cli.h"
#include "saqTarget.h"

#include "cmsis_os.h"

#include <stdint.h>

#define regIntAccess(var) var.u.dataInt
#define regUintAccess(var) var.u.dataUint

#define REG_VALUE_SZ 50
#define DATA_STRING_SZ 30

typedef enum {
    REG_FIRST,                ///< Dummy
    FW_VERSION_MAJ,           ///< Firmware Major Version value
    FW_VERSION_MIN,           ///< Firmware Minor Version value
    FW_VERSION_MAINT,         ///< Firmware Maintenance Version value
    FW_VERSION_BUILD,         ///< Firmware Build Version value
    HW_TYPE,                  ///< EEPROM HW TYPE
    HW_VERSION,               ///< EEPROM HW VERSION
    REBOOT_FLAG,              ///< Is a user reboot requested
    REBOOT_DELAY_MS,          ///< Time in msec before reboot
    UPTIME_SEC,               ///< Up time in secs of the MCU
    ADC_READ_RATE,            ///< ADC conversion rate
    ADC_READ_DUTY,            ///< ADC % ratio to hold the line high vs low
    SERIAL_NUMBER,            ///< Eeprom stored register
    IP_ADDR,                  ///< Eeprom stored register
    HTTP_PORT,                ///< Eeprom stored register
    UDP_TX_PORT,              ///< Eeprom stored register
    GATEWAY,                  ///< Eeprom stored register
    NETMASK,                  ///< Eeprom stored register
    UDP_SERVER_IP,            ///< Eeprom stored register
    UDP_SERVER_PORT,          ///< Eeprom stored register
    DEPRECATED_TCP_CLIENT_IP, ///< Eeprom stored register
    TCP_CLIENT_PORT,          ///< Eeprom stored register
    IP_TX_DATA_TYPE,          ///< Eeprom stored register
    UNIQUE_ID,                ///< Eeprom stored register
    ADC_MODE,                 ///< Eeprom stored register only read on boot
    STREAM_INTERVAL_US,
    DB_SPI_INTERVAL_US,
    DB_RETRY_INTERVAL_S,
    TRIGGER_MASK_0,
    SENSOR_BOARD_0,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_1,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_2,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_3,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_4,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_5,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_6,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_7,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_8,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_9,  ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_10, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_11, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_12, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_13, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_14, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_15, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_16, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_17, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_18, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_19, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_20, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_21, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_22, ///< EEPROM Stored register, SB expected in slot x
    SENSOR_BOARD_23, ///< EEPROM Stored register, SB expected in slot x
    MFG_WRITE_EN,    ///< Must be set to true before any MFG write commands are accepted
    DDS_CLK_RATE,
    DDS_CLK_DUTY,
    GREEN_LED_ON,         ///< Bool Green Led on otherwise blinking
    RED_LED_ON,           ///< Bool RED Led on otherwise use state off or blinking.
    PERIPHERAL_FAIL_MASK, ///< Each bit represents a peripheral
    FAN_POP,              ///< Fan population, bitwise b0=FAN1, b1=FAN2 population
    MB_REG_MAX
} REGISTER_MB_ID; // must occur before include of board_registersParams.h

typedef enum {
    SB_REG_FIRST,        ///< Dummy
    SB_FW_VERSION_MAJ,   ///< Firmware Major Version value
    SB_FW_VERSION_MIN,   ///< Firmware Minor Version value
    SB_FW_VERSION_MAINT, ///< Firmware Maintenance Version value
    SB_FW_VERSION_BUILD, ///< Firmware Build Version value
    SB_HW_TYPE,          ///< EEPROM HW TYPE
    SB_HW_VERSION,       ///< EEPROM HW VERSION
    SB_SERIAL_NUMBER,    ///< Eeprom stored register
    SB_REBOOT_FLAG,      ///< Is a user reboot requested
    SB_REBOOT_DELAY_MS,  ///< Time in msec before reboot
    SB_UPTIME_SEC,       ///< Up time in secs of the MCU
    SB_ADC_MODE,         ///< Eeprom stored register only read on boot
    SB_GREEN_LED_ON,     ///< Bool Green Led on otherwise blinking
    SB_RED_LED_ON,       ///< Bool RED Led on otherwise use state off or blinking.
    SB_LOOPBACK_SENSOR,
    SB_LOOPACK_SENSOR_INTERVAL,
    SB_PERIPHERAL_FAIL_MASK, ///< Each bit represents a peripheral
    SB_IMU0_PRESENT,         ///< MFG data identify if imu0 is populated
    SB_IMU1_PRESENT,         ///< MFG data identify if imu1 is populated
    SB_IMU_BAUD,             ///< IMU UART Speed, default 460800
    SB_CFG_ALLOWED,          ///< Allow changes to various MFG data stored in eeprom
    SB_ECG_STAT_DATA,        ///< Read only, ECG Lead Connection status and GPIO.
                             ///< data format is 1100<LOFF_STATP[7:0]><LOFF_STATN[7:0]><GPIO[7:4]>
                             ///< status word of 24bitsSTATUS 24bit value
    SB_ECG_LEG_LEAD_CONNECT, ///< LEG LEAD CONNECT ADC register 3, bit 0 RLD_STAT
    SB_REG_MAX
} REGISTER_DB_ID; // must occur before include of board_registersParams.h

// the DB register unique id start at LAST_COMMON_IDX. the main board needs to
// know the name to access them.

typedef enum { DATA_UINT, DATA_INT, DATA_STRING } REGISTER_TYPE;

typedef struct registerInfo {
    union {
        REGISTER_MB_ID mbId; // main board registers
        REGISTER_DB_ID sbId; // sensor board registers
    };
    REGISTER_TYPE type;
    size_t size;
    union {
        uint32_t dataUint;
        int32_t dataInt;
        char *dataString;
    } u; // items are 32 bit
} registerInfo_t, *registerInfo_tp;

typedef struct {
    registerInfo_t info;
    const char *name;
    RETURN_CODE (*writePtr)(registerInfo_tp info);
    RETURN_CODE (*readPtr)(registerInfo_tp info);
} registerNameInfo_t;

#ifdef STM32H743xx
typedef struct {
    osSemaphoreId mutex;
    registerNameInfo_t reg[MB_REG_MAX];
} paramStorage_t;

#define EEPROM_PARAM_ELEMENT(NAME, TYPE, SIZE)                                                                         \
    [NAME] = {.info = {.mbId = NAME, .type = TYPE, .size = SIZE},                                                      \
              .name = #NAME,                                                                                           \
              .readPtr = eepromParamRead,                                                                              \
              .writePtr = eepromParamWrite}

#else
typedef struct {
    osSemaphoreId mutex;
    registerNameInfo_t reg[SB_REG_MAX];
} paramStorage_t;

#define EEPROM_PARAM_ELEMENT(NAME, TYPE)                                                                               \
    [NAME] = {                                                                                                         \
        .info = {.sbId = NAME, .type = TYPE}, .name = #NAME, .readPtr = eepromParamRead, .writePtr = eepromParamWrite}

#endif

#ifdef STM32H743xx
#define REG_MAX MB_REG_MAX
#define REGISTER_ID REGISTER_MB_ID
#define REGINFO_ID regInfo->mbId
#define INFO_ID(x) paramStorage.reg[x].info.mbId
#define REGDATA_ID regData.mbId

#else
#define REG_MAX SB_REG_MAX
#define REG_FIRST SB_REG_FIRST
#define REGISTER_ID REGISTER_DB_ID
#define REGISTER_FIRST SB_REGISTER_FIRST
#define REGINFO_ID regInfo->sbId
#define INFO_ID(x) paramStorage.reg[x].info.sbId
#define REGDATA_ID regData.sbId
#endif

/**
 * @brief initialize the run time register table
 * @return 0 if successful SEE RETURN_CODE for details.
 */
RETURN_CODE registerInit(void);

/**
 * @brief Populate a register ID using the register name
 * @param name Name of the register to get ID for
 * @param regInfo return struct that will be populated the register identifier
 * @return 0 if successful SEE RETURN_CODE for details.
 */
RETURN_CODE registerByName(const char *name, registerInfo_tp regInfo);

/**
 * @brief Populate read a register name for a given register
 * @param regInfo struct that contains the register identifier
 * @param name Name of the register corresponding to the register ID
 * @return 0 if successful SEE RETURN_CODE for details.
 */
RETURN_CODE registerGetName(registerInfo_tp regInfo, const char **name);

/**
 * @brief Populate read a registers type for a given register idx
 * @param idx index of register
 * @param p_type pointer to return the type in
 * @return 0 if successful SEE RETURN_CODE for details.
 */
RETURN_CODE registerGetType(uint32_t idx, REGISTER_TYPE *p_type);

/**
 * @brief Read the current runtime value of the register
 * @param regInfo struct that contains the register identifier and a paramter
 *        to write the value to.
 * @return 0 if successful SEE RETURN_CODE for details.
 */
RETURN_CODE registerRead(registerInfo_tp regInfo);

/**
 * @brief Write the value in regInfo->u. to the runtime storage location for
 *        the register id
 * @return 0 if successful SEE RETURN_CODE for details.
 *
 * @note Some registers have a write pointer that can be used to can take
 * special handling such update local copies or write to the eeprom for storage.
 */
RETURN_CODE registerWrite(const registerInfo_tp regInfo);

/**
 * @brief Write the value in regInfo->u. to the runtime storage location for
 *        the register id
 * @return void
 *
 * @note This function skips the write pointer and writes to the storage location.
 */

void registerWriteForce(const registerInfo_tp regInfo);

/**
 * @brief Read register value from the eeprom
 *
 * @param regInfo struct that contains the register identifier and a paramter
 *        to write the value to.
 *
 * @return 0 if successful SEE RETURN_CODE for details.
 *
 */
RETURN_CODE eepromParamRead(const registerInfo_tp regInfo);

/**
 * @brief Write register value to the eeprom
 *
 * @param regInfo struct that contains the register identifier and a paramter
 *        to read the value from.
 *
 * @return 0 if successful SEE RETURN_CODE for details.
 *
 */
RETURN_CODE eepromParamWrite(const registerInfo_tp regInfo);

/**
 * @brief CLI command that reads the register
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t cliHandler_readreg(CLI *hCli, int argc, char *argv[]);

/**
 * @brief CLI command that writes the register
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t cliHandler_writereg(CLI *hCli, int argc, char *argv[]);

/**
 * @brief CLI command that lists the register
 *
 * @param[in] CLI *hCli: CLI instance
 * @param[in] int argc: Number or command line arguments
 * @param[in] char *argv[]: List of command line arguments
 *
 * @return CLI_RESULT_SUCCESS on success
 **/
int16_t cliHandler_listreg(CLI *hCli, int argc, char *argv[]);

/**
 * @fn adcSetPwmRate
 *
 * @brief Change the frequency of the ADC sample rate.
 *
 * @param[in]     freq Frequency of ADC sample rate
 *
 * @return RETURN_CODE
 **/
RETURN_CODE adcSetPwmRate(registerInfo_tp info);

/**
 * @fn ddsSetPwmRate
 *
 * @brief Change the frequency of the DDS PWM rate
 *
 * @param[in]     freq Frequency of DDS pwm rate
 *
 * @return RETURN_CODE
 **/
RETURN_CODE adcSetPwmDuty(registerInfo_tp info);
/**
 * @fn adcSetPwmRate
 *
 * @brief Change the ADC sample rate duty cycle.
 *
 * @param[in]     duty ADC sample rate duty cycle
 *
 * @return RETURN_CODE
 **/
RETURN_CODE ddsSetPwmRate(registerInfo_tp info);

/**
 * @fn ddsSetPwmDuty
 *
 * @brief Change the DDS PWM duty
 *
 * @param[in]     duty DDS pwm rate duty cycle
 *
 * @return RETURN_CODE
 **/
RETURN_CODE ddsSetPwmDuty(registerInfo_tp info);

/**
 * @fn updateResetDelay
 *
 * @brief Change the delay time before resetting board.
 *
 * @param[in]     register information
 *
 * @return RETURN_CODE
 **/
RETURN_CODE updateResetDelay(registerInfo_tp info);

/**
 * @fn updateResetFlag
 *
 * @brief Change reset flag to indicate if a reset should occur
 *
 * @param[in]     register information
 *
 * @return RETURN_CODE
 **/
RETURN_CODE updateResetFlag(registerInfo_tp info);

/**
 * @fn getUptimeAsRegister
 *
 * @brief get the uptime value
 *
 * @param[in]     register information
 *
 * @return RETURN_CODE
 **/
RETURN_CODE getUptimeAsRegister(const registerInfo_tp regInfo);

/**
 * @fn noWriteFn
 *
 * @brief if a write is attempted then flag it as a failure
 *
 * @param[in]     register information
 *
 * @return RETURN_CODE
 **/
RETURN_CODE noWriteFn(const registerInfo_tp regInfo);

/**
 * @fn copyToBuffer_listreg
 *
 * @brief copy the reglist information to a buffer.
 *
 * @param[in]     buf   memory location to copy information to
 *
 * @param[in]     bufSz  maximum memory size for copying information to
 *
 * @return amount of characters written to the buffer
 **/
uint32_t copyToBuffer_listreg(char *buf, uint32_t bufSz);

#endif /* LIB_INC_REGISTERPARAMS_H_ */
