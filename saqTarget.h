/*
 * saqTarget.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 7, 2023
 *      Author: rlegault
 */

#ifndef INC_SAQTARGET_H_
#define INC_SAQTARGET_H_

#include "byte_order.h"
#include "debugCompileOptions.h"
#include <stdbool.h>
#include <stdint.h>

#define __DTCMRAM2__
#ifdef STM32H743xx
#define __ITCMRAM__ __attribute__((section(".myitcmram")))
#define __DTCMRAM__ __attribute__((section(".mydtcmram")))
#else
#define __ITCMRAM__
#define __DTCMRAM__
#endif

#define WAIT_MS(X) (X)
#define STRING_NUMER_BASE(x) (x)
// The following Macros are used to create a ENUM list and a matching
// char* table for each enum.

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
#define GENERATE_ENUM_LIST(MACRO, NAME) typedef enum NAME { MACRO(GENERATE_ENUM) } NAME;
#define GENERATE_PACKED_ENUM_LIST(MACRO, NAME) typedef enum __attribute__((packed)) NAME { MACRO(GENERATE_ENUM) } NAME;
// A #define/#undef flag must be used in one C file to identify which C file to
// create the table in when using the macro below
/* In Header File use the following
 * Example code
 #ifdef CREATE_INSTANCE
 GENERATE_ENUM_STRING_NAMES(ENUM_T, test_e);
 #else
 extern char* test_e_Strings[];
 #endif
 */
#define GENERATE_ENUM_STRING_NAMES(MACRO, NAME) const char *NAME##_Strings[] = {MACRO(GENERATE_STRING)};

// must be equal to size of debug log on sensor board and
// buffer storage area on mainboard.
#define LARGE_BUFFER_SIZE_BYTES ((20 * 1024) - 128)
#define SENSORLOG_RAM_SIZE_BYTES (19 * 1024 - 128)
#define SPI_WORKER_OFFSET_ERROR 1 // the SPI worker preloads a value after the interrupt so we need

#define PRINT_FULL_SPI_PACKET 0
#define TRACE_ANALYZE_SPI 0
#define PRINT_SPI_CRC_ERROR 0
#define PRINT_BYTE_CNT 8
#define MAX_CS_PER_SPI 8
#define MAX_SPI 3
#define MAX_CS_ID (MAX_CS_PER_SPI * MAX_SPI)

#define SENSORS_PER_BOARD 2
#define NUMBER_OF_ADC_READINGS_PER_SENSOR 4

#define BINARY_SEMAPHORE 1
#define NO_WAIT 0

// IMU information
#define IMU_MAX_BOARD 8
#define IMU_PER_BOARD 2

#define CARTESSIAN_COOR 3
#define QUATERNION_COOR 4
#define IMU0_IDX 0
#define IMU1_IDX 1

// Each thread that requires a print buffer creates its own static buffer
// So that thread trampling does not occur
#define MAX_PRINT_BUFFER_SZ 120

#define PRINT_1_OUT_OF_(x) (x)
#define THROTTLE_PRINT_OUTPUT(CNT, ONCE_PER_NUM_MESSAGES) ((CNT++ % ONCE_PER_NUM_MESSAGES) == 0)

typedef enum { ADC_MODE_SINGLE, ADC_MODE_CONTINUOUS, ADC_MODE_MAX } ADC_MODE_e;

extern ADC_MODE_e g_adcModeSwitch;

/* *******************
 * daughter board control
 *
 */
// Uses SPARE1 set to high to identify that it is connected to the main board
#define WORKER_SPI_TIMEOUT_MS 500  // watchdog update rate
#define WORKER_SPI_REBOOT_SEC 30   // reboot daughter card if no spi transactions in 10 seconds
#define WORKER_SPI_MAX_CRC_ERROR 5 // consecutive crc errors leads to reboot
/*********************/

/* ****************
 * Control daughter board process detection of missing board and recovery.
 * use CLI command
 * dbproc enable <id> 1 to force enable of a board id=0-48
 */
#define VALUE_STREAM_INTERVAL_US 2000 * MULTIPLER
#define VALUE_DB_SPI_INTERVAL_US 2000 * MULTIPLER
#define VALUE_DB_RETRY_INTERVAL_S 30          // 0.5 minutes
#define DB_MAX_UNANSWERED_RESPONSE 240        // imu commands are worst case
#define DB_MAX_UNANSWERED_RESPONSE_DISABLE 10 // number of no message before declaring the DB as disabled
#define DB_SPI_INTERVAL_MS 2

// Sensor Defaults
#define ADC_READ_RATE_x100Hz 50000 // 50000 = 500 hz 6 is the lowest value

#define DDS_CLOCK_RATE_x100Hz 1677721 // 16.77721KHz
#define DUTY_50_PERC 50

// Web request and response information
#define LOCAL_ID (0xFF)
#define MAIN_BOARD_ID (0xFE) // 254
#define BOARD_DAUGHTER(x) (x)
typedef uint8_t mcu_e;

#define DAC_MAX5717_VREF 4096 // from max5717.h

// DDS common values
// custom registers for CNC from ad9106.h
typedef enum {
    MCG_DDS_DAC_EXC=0,
    MCG_DDS_DAC_COMP_A,
    MCG_DDS_DAC_COMP_B,
    MCG_DDS_DAC_COMP_C,
} MCG_DDS_DAC_IDX_MAP_e;


#define DDS_FREQUENCY_BASE_ADDR 0x61
#define DDS_AMPLITUDE_BASE_ADDR 0x62
#define DDS_PHASE_BASE_ADDR 0x66
#define DDS_WAVEFORM_BASE_ADDR 0x6A

#define DDS_WAVEFORM_ELONGATE_BASE_ADDR 0x6E
#define DDS_WAVEFORM_INTERVAL_BASE_ADDR 0x6F
#define DDS_WAVEFORM_DELAY_BASE_ADDR 0x70

/* matches Strapping pin values */
#define macro_handlerBT(T)                                                                                             \
    T(BOARDTYPE_MCG)                                                                                                   \
    T(BOARDTYPE_ECG)                                                                                                   \
    T(BOARDTYPE_IMU_COIL)                                                                                              \
    T(BOARDTYPE_12ECG)                                                                                                 \
    T(BOARDTYPE_TBD4)                                                                                                  \
    T(BOARDTYPE_TBD5)                                                                                                  \
    T(BOARDTYPE_TBD6)                                                                                                  \
    T(BOARDTYPE_TBD7)                                                                                                  \
    T(BOARDTYPE_EMPTY)                                                                                                 \
    T(BOARDTYPE_UNKNOWN)

#define BOARDTYPE_MAX BOARDTYPE_UNKNOWN

GENERATE_ENUM_LIST(macro_handlerBT, BOARDTYPE_e)
#ifdef CREATE_STRING_LOOKUP_TABLE
GENERATE_ENUM_STRING_NAMES(macro_handlerBT, BOARDTYPE_e)
#else
extern const char *BOARDTYPE_e_Strings[];
#endif

#undef macro_handlerBT

#define U32_BITS 32

// Peripheral ID names and associated String name
#define macro_handlerPER(T)                                                                                            \
    T(PER_NOP)                                                                                                         \
    T(PER_DAC_0_EXCITATION)                                                                                            \
    T(PER_DAC_0_COMPENSATION)                                                                                          \
    T(PER_DAC_1_EXCITATION)                                                                                            \
    T(PER_DAC_1_COMPENSATION)                                                                                          \
    T(PER_DDS0_A)                                                                                                      \
    T(PER_DDS0_B) /* only on Coil Driver board */                                                                      \
    T(PER_DDS0_C) /* only on Coil Driver board */                                                                      \
    T(PER_DDS1_A)                                                                                                      \
    T(PER_DDS1_B) /* only on Coil Driver board */                                                                      \
    T(PER_DDS1_C) /* only on Coil Driver board */                                                                      \
    T(PER_ADC)                                                                                                         \
    T(PER_IMU0)                                                                                                        \
    T(PER_IMU1)                                                                                                        \
    T(PER_MCU) /* PWM and GPIO are accessed through MCU registers */                                                   \
    T(PER_GPIO)                                                                                                        \
    T(PER_EEPROM)                                                                                                      \
    T(PER_TEST_MSG) /* Send hcli via addr, send string via ptr in cncInfo_t */                                         \
    T(PER_FAN)                                                                                                         \
    T(PER_LOG)                                                                                                         \
    T(PER_DBG) /* get stats information about task, spi, etc   */                                                      \
    T(PER_PWR)                                                                                                         \
    T(PER_MAX) /* must be less than 32 so that we can bit mask the peripheral errors. */

#define macro_handlerHttp(T)                                                                                           \
    T(NO_ERROR)                                                                                                        \
    T(UNKNOWN_PERIPHERAL)                                                                                              \
    T(UNKNOWN_COMMAND)                                                                                                 \
    T(REG_WRITE_ERROR)                                                                                                 \
    T(REG_READ_ERROR)                                                                                                  \
    T(INTERNAL_COMMAND_ERROR)                                                                                          \
    T(VALUE_ERROR)

GENERATE_ENUM_LIST(macro_handlerPER, PERIPHERAL_e)
GENERATE_PACKED_ENUM_LIST(macro_handlerHttp, httpErrorCodes)

#ifdef CREATE_STRING_LOOKUP_TABLE
GENERATE_ENUM_STRING_NAMES(macro_handlerPER, PERIPHERAL_e)
GENERATE_ENUM_STRING_NAMES(macro_handlerHttp, httpErrorCodes)
#else
extern const char *PERIPHERAL_e_Strings[];
extern const char *httpErrorCodes_Strings[];
#endif
#undef macro_handler
#undef macro_handlerPER

// When accessing the PERIPHERAL DEBUG (PER_DBG)
typedef enum {
    PER_DBG_TEST_WDOG,
    PER_DBG_TEST_TASK,
    PER_DBG_TEST_STACK,
    PER_DBG_TEST_HEAP,
    PER_DBG_SYSTEM_STATUS,
    PER_DBG_REGISTER_LIST,
    PER_DBG_SPI_DB,
    PER_DBG_SPI_1,
    PER_DBG_SPI_2,
    PER_DBG_SPI_3,
    PER_DBG_DBPROC_0,
    PER_DBG_DBPROC_1,
    PER_DBG_DBPROC_2,
    PER_DBG_DBPROC_3,
    PER_DBG_DBPROC_4,
    PER_DBG_DBPROC_5,
    PER_DBG_DBPROC_6,
    PER_DBG_DBPROC_7,
    PER_DBG_DBPROC_8,
    PER_DBG_DBPROC_9,
    PER_DBG_DBPROC_10,
    PER_DBG_DBPROC_11,
    PER_DBG_DBPROC_12,
    PER_DBG_DBPROC_13,
    PER_DBG_DBPROC_14,
    PER_DBG_DBPROC_15,
    PER_DBG_DBPROC_16,
    PER_DBG_DBPROC_17,
    PER_DBG_DBPROC_18,
    PER_DBG_DBPROC_19,
    PER_DBG_DBPROC_20,
    PER_DBG_DBPROC_21,
    PER_DBG_DBPROC_22,
    PER_DBG_DBPROC_23,
    PER_DBG_MAX
} PER_DBG_ADDR_e;

// Write value to addr to take log command
typedef enum { PER_LOG_ADDR_READ, PER_LOG_ADDR_CLEAR } PER_LOG_ADDR_e;

typedef enum {
    CNC_ACTION_READ,
    CNC_ACTION_WRITE,
    CNC_ADC_CMD,
    CNC_DDS_START,
    CNC_DDS_STOP,
    CNC_DDS_SYNC,
    CNC_PWM_START,
    CNC_PWM_STOP,
    CNC_IMU_CMD,
    CNC_ACTION_READ_SMALL_BUFFER, // 1K buffer Transfer
    CNC_ACTION_READ_MED_BUFFER,   // 4K buffer Transfer
    CNC_ACTION_READ_LARGE_BUFFER, // 19K buffer transfer
    CNC_ACTION_MAX
} CNC_ACTION_e;

#define CNC_WEB_CMD_MAX 2

typedef struct {
    const char *name;
    union {
        PERIPHERAL_e enumId;
        CNC_ACTION_e cmdId;
    };
} strLUT_t, *strLUT_tp;

extern const strLUT_t peripheralLUT[];
extern const strLUT_t commandLUT[];

// If there are crc errors due to the first byte being incorrect from the DB then enable the following define.
// #define SET_FIRST_BYTE_0XA5

// Communication information for packets sent between main board and daughter board.

#define RX_RESPONSEBIT 0x80 // Identifies messages sent from DB to the Mainboard
#define RX_COILBIT 0x40

// This flag prevents the loss of a data packet, only responds with PASS/FAIL
// Comes from the Mainboard to the daughter boards
#define CMD_SHORTRSEPONSE 0x20 // Identify that only a response in the CMD header CMD_RESPONSE is required

// The bottom 5 bits are used for command identifiers
// The top 3 bits are used for the above flags that are being sent
typedef enum {
    SENSOR_DATA = 0, // request for latest stored sensor data
    // Following are sent from Mainboard, CLI to DB CNC task
    SPICMD_NOP,
    SPICMD_SENSOR, // get latest sensor reading via duplex or last cmd response
    SPICMD_CNC,    // send payload to CNC

    SPICMD_SET_SKIP_WAIT_FOR_RESPONSE, // debug cmd SPI will not send to DB but will send sensor data back.
    SPICMD_SET_LOOPBACK_OFFSET,        // debug set daughter board loopback offset counter
    SPICMD_MAX_CONSECUTIVE,            // must be last of the consecutive values

    SPICMD_UNUSED =
        CMD_SHORTRSEPONSE - 2, // SPI CMD field is not being used in this context, typically CLI sending message to CNC
    SPICMD_TRIGGER = CMD_SHORTRSEPONSE - 1, // do not free message it is static

    // the following must come last in enum list as their MSBit is set
    SPICMD_CNC_SHORT = CMD_SHORTRSEPONSE | SPICMD_CNC,
    SPICMD_STREAM_SENSOR = RX_RESPONSEBIT | SPICMD_SENSOR, // sent by DB whenever no commands responses are in the Q
    SPICMD_DDS_TRIG = RX_RESPONSEBIT | SPICMD_TRIGGER,     // sent from daughter board as first 2 messages
    SPICMD_RESP_CNC = RX_RESPONSEBIT | SPICMD_CNC,
    SPICMD_STREAM_SENSOR_W_SHORTRESPONSE = RX_RESPONSEBIT | CMD_SHORTRSEPONSE | SPICMD_SENSOR,
    // sent by DB when the data pkt has IMU information and no commands in responses are in the Q
    SPICMD_RESP_COIL = RX_RESPONSEBIT | RX_COILBIT | SPICMD_SENSOR,

} spiDbMbCmd_e;

_Static_assert(SPICMD_MAX_CONSECUTIVE < SPICMD_UNUSED, "Too many command, Require refactoring CMD");
_Static_assert(sizeof(spiDbMbCmd_e) == sizeof(uint8_t), "ENUM size is too large");

typedef struct {
    spiDbMbCmd_e cmdId;
    const char *cmdName;
} spiDbMbCmd_string_t;

#define DBCOMM_CNT (SPICMD_MAX_CONSECUTIVE + 8 + 1) // 8 non consecutive commands and 1 because we are 0 based array

#ifdef __DBCOMM_CREATE__
#define tostr__(V) #V
#define spiDbMbCmd_CreateInstanceConsecutive(V) [V] = {.cmdId = V, .cmdName = tostr__(V)}

// only create the instance in ctrlSpiCommTask.c
spiDbMbCmd_string_t spiDbMbCmd_string[DBCOMM_CNT] = {
    spiDbMbCmd_CreateInstanceConsecutive(SENSOR_DATA),
    spiDbMbCmd_CreateInstanceConsecutive(SPICMD_NOP),
    spiDbMbCmd_CreateInstanceConsecutive(SPICMD_SENSOR),
    spiDbMbCmd_CreateInstanceConsecutive(SPICMD_CNC),
    spiDbMbCmd_CreateInstanceConsecutive(SPICMD_SET_LOOPBACK_OFFSET),
    spiDbMbCmd_CreateInstanceConsecutive(SPICMD_MAX_CONSECUTIVE),
    {.cmdId = SPICMD_UNUSED, .cmdName = tostr__(SPICMD_UNUSED)},
    {.cmdId = SPICMD_TRIGGER, .cmdName = tostr__(SPICMD_TRIGGER)},
    {.cmdId = SPICMD_CNC_SHORT, .cmdName = tostr__(SPICMD_CNC_SHORT)},
    {.cmdId = SPICMD_STREAM_SENSOR, .cmdName = tostr__(SPICMD_STREAM_SENSOR)},
    {.cmdId = SPICMD_DDS_TRIG, .cmdName = tostr__(SPICMD_DDS_TRIG)},
    {.cmdId = SPICMD_RESP_CNC, .cmdName = tostr__(SPICMD_RESP_CNC)},
    {.cmdId = SPICMD_STREAM_SENSOR, .cmdName = tostr__(SPICMD_STREAM_SENSOR)},
    {.cmdId = SPICMD_RESP_COIL, .cmdName = tostr__(SPICMD_RESP_COIL)},
};
const char *spiDbMbCmdToString(spiDbMbCmd_e cmd) {
    if (cmd <= SPICMD_MAX_CONSECUTIVE) {
        return spiDbMbCmd_string[cmd].cmdName;
    }
    for (int i = SPICMD_MAX_CONSECUTIVE + 1; i < DBCOMM_CNT; i++) {
        if (cmd == spiDbMbCmd_string[i].cmdId) {
            return (spiDbMbCmd_string[i].cmdName);
        }
    }
    return "UNKNOWNED";
}
#else
extern spiDbMbCmd_string_t spiDbMbCmd_string[DBCOMM_CNT];
const char *spiDbMbCmdToString(spiDbMbCmd_e cmd);
#endif

#define NUMBER_DDS_EXCITATION_AMPLITUDE_READINGS 2 // DDS Excitation setting
#define NUMBER_DAC_COMPENSATION_AMPLITUDE_READINGS 3 // DAC Compensation A,B,C
#define NUMBER_OF_SENSOR_READINGS 8
#define SIZE_OF_IMU_FRAGMENT 4
#define NUMBER_OF_IMU_FRAGMENTS 3

// IMU FLAG Macros
#define NIBBLE_SIZE 4
typedef struct {
    uint8_t shift;
    uint8_t mask;
} nibbleInfo_t;
#ifdef CREATE_STRING_LOOKUP_TABLE
#define NIBBLE_MASK_LOW 0x0F
#define NIBBLE_MASK_HIGH 0xF0
const nibbleInfo_t imuNibbleInfo[IMU_PER_BOARD] = {{0, NIBBLE_MASK_LOW}, {NIBBLE_SIZE, NIBBLE_MASK_HIGH}};
#else
extern const nibbleInfo_t imuNibbleInfo[IMU_PER_BOARD];
#endif

#define PAYLOAD_CLEAR_NIBBLE(idx, v) (v &= ~imuNibbleInfo[idx].mask)

#define PAYLOAD_SET_NIBBLE_FLAG(idx, v, flag)                                                                          \
    (v = (v & (~imuNibbleInfo[idx].mask)) | (flag << imuNibbleInfo[idx].shift))
#define PAYLOAD_GET_FLAG(idx, flag) ((flag & imuNibbleInfo[idx].mask) >> (imuNibbleInfo[idx].shift))
#define PAYLOAD_TRIBLE_INDEX(flag) (flag - IMU_DATA_FLAG_SENT_HIGH_TRIBBLE)

// IMU data translation macros

// Translate the Big Endian U32 value to a u32 float in little endian format
#define u32BE_to_floatLE(dest, u32value)                                                                               \
    {                                                                                                                  \
        uint32_t temp = htob32(u32value);                                                                              \
        dest = *((float *)&temp);                                                                                      \
    }

// Return the float value of the magnet from the binary representation
#define IMU_16BIT_VALUE_DIVIDER 16.0
#define MAGNET_VALUE(magnet) ((int16_t)magnet / IMU_16BIT_VALUE_DIVIDER)

// Return the float value of the magnet from the binary representation
#define TEMPERATURE_VALUE(x) ((int16_t)x / IMU_16BIT_VALUE_DIVIDER)

// This Payload Structure is used to send sensor data from the sensor board
// to the main board via spi when no command responses are waiting.


#define BYTES_IN_24_BIT (24/8)
#define USING32_ADC_SAMPLES_IN_SPI 1   // Setting to 0 is untested.

#if USING32_ADC_SAMPLES_IN_SPI
#define VALUE_X_BIT value32bit
#define READ_XBITSVALUE READ_32BITSVALUE
#define READ_XBITUVALUE READ_32BITUVALUE
#define WRITE_XBITVALUE WRITE_32BITVALUE
#else
#define VALUE_X_BIT value24bit;
#define READ_XBITSVALUE READ_24BITSVALUE
#define READ_XBITUVALUE READ_24BITUVALUE
#define WRITE_XBITVALUE WRITE_24BITVALUE
#endif
typedef struct {
    // Concerned that the Mainboard translating 24 bit to 32 bit is going to tax the main board mcu
    // since it has to repeat this 8*22 times.
    // So going to take the hit on the SPI bandwidth side and send 8 wasted bytes per board instead
    // and thus have the sensor boards do the 24 to 32 bit translations.
#if USING32_ADC_SAMPLES_IN_SPI
    uint32_t value32bit;
#else
    uint8_t value24bit[BYTES_IN_24_BIT];
#endif

}adc24Reading_t;

#define NEGATIVE_BIT 0x80
#define NEGATIVE_EXT(b) (0xFF<<b)
// copy 3 bytes from y to x, actual memcpy would take more cpu cycles, z is to keep it the same parameters as memcpy
#define READ_24BITUVALUE(p) ((p[0]<<16) | (p[1]<<8) | (p[2]<<0))
#define READ_24BITSVALUE(p) (((p[0]&NEGATIVE_BIT) ? NEGATIVE_EXT(24) : 0) | (p[0]<<16) | (p[1]<<8) | (p[2]<<0))
#define WRITE_24BITVALUE(p,v) {p[0]=((v&0x00FF0000)>>16); p[1]=((v&0x0000FF00)>>8); p[2]=((v&0x000000FF)>>0);}

#define READ_32BITUVALUE(p) (*(uint32_t*)p)
#define READ_32BITSVALUE(p) (*(int32_t*)p)
#define WRITE_32BITVALUE(p,v) *(uint32_t*)p = v



//#define MEMCPY_3BYTES(x, y, z)
typedef struct __attribute__((packed)) {
    uint16_t dacCompensationAmplitude;
    // DDS has 24bits but we will just capture the high 16 bits
    uint16_t ddsExcitationAmplitude;
    uint16_t trimCompensationA;
    uint16_t trimCompensationC;
    // 8 bytes of data
} coilData_t, *coilData_tp;

typedef struct __attribute__((packed)) {
    union {
        struct {
             adc24Reading_t adc[NUMBER_OF_SENSOR_READINGS];
             coilData_t ctrlData[SENSORS_PER_BOARD];
        };
        struct {
            uint32_t imu[IMU_PER_BOARD][SIZE_OF_IMU_FRAGMENT];
            uint32_t imuFlag; // identify if imu0 and or imu1 has IMU data
                              // and which fragment.
        };
    };
} rx_dbNopPayload_t, *rx_dbNopPayload_tp;

// This Cmd payload structure is used to send commands from the Main board
// to the sensor boards. The payload is CMD specific.
typedef struct __attribute__((packed)) {
    uint8_t payload[sizeof(adc24Reading_t)*NUMBER_OF_SENSOR_READINGS + (sizeof(coilData_t)*SENSORS_PER_BOARD)-sizeof(uint32_t)];
    uint32_t flag2;
} dbCmdPayload_t;

// SPI Packet sent back and fourth between Main board and daughter board

typedef uint8_t spiDBMBPacket_payload_t[sizeof(rx_dbNopPayload_t)];

#define CMD_UID_DONT_CARE 0
#define FLAG_LARGEBUFFER 1
typedef struct __attribute__((packed)) {
    uint16_t cmdUid; // allow for match of cmd response to command sent
    httpErrorCodes cmdResponse;
    uint8_t flags;
} spiDbMbPacketCmdResponse_t, *spiDbMbPacketCmdResponse_tp;

_Static_assert(sizeof(spiDbMbPacketCmdResponse_t) == 4, "spiDbMbPacketCmdResponse_t size error");

typedef struct __attribute__((packed, aligned(4))) {
    union {
        uint16_t pktId;
        uint8_t pkt8Id[2];
    };
    uint8_t cmd;   // enum spiDbMbCmd_e
    uint8_t xInfo; // sensor or cmd counter to identify stale data
    spiDbMbPacketCmdResponse_t cmdResponse;
} spiDbMbPacketHeader_t, *spiDbMbPacketHeader_tp;

#define SIZEOF_CRC sizeof(uint32_t)

#define spiDbMbPacketSizeof (sizeof(spiDbMbPacketHeader_t) + sizeof(spiDBMBPacket_payload_t) + SIZEOF_CRC)

// Data structure of the SPI packet that is used for communication between
// main board and sensor board and back again.
typedef struct __attribute__((packed, aligned(4))) {
    union {
        uint32_t u32[spiDbMbPacketSizeof / sizeof(uint32_t)];
        uint8_t u8[spiDbMbPacketSizeof];
        struct {
            spiDbMbPacketHeader_t header;
            union {
                spiDBMBPacket_payload_t spiDBMBPacket_payload; // command dependent
                uint8_t *largeBufferPtr;
            };
            uint32_t crc; // U32 crc calculation using the STM32F411 CRC engine parameters.
        };
    };
} spiDbMbPacket_t, *spiDbMbPacket_tp;

_Static_assert(sizeof(spiDbMbPacket_t) == spiDbMbPacketSizeof, "size is not 60 bytes");
_Static_assert(sizeof(dbCmdPayload_t) == sizeof(rx_dbNopPayload_t), "size is not 40 bytes");
_Static_assert(sizeof(dbCmdPayload_t) == sizeof(spiDBMBPacket_payload_t), "size is not 40 bytes");




#if 0// macro to print sizeof values at compile time       | u24bit | u32bit |
char (*__kaboom)[sizeof(spiDbMbPacket_t)] = 1;          // |   52   |   60   | bytes
char (*__kaboom)[spiDbMbPacketSizeof] = 1;              // |   52   |   60   | bytes
char (*__kaboom)[sizeof(dbCmdPayload_t)] = 1;           // |   40   |   48   | bytes
char (*__kaboom)[sizeof(rx_dbNopPayload_t)] = 1;        // |   40   |   48   | bytes
char (*__kaboom)[sizeof(spiDBMBPacket_payload_t)] = 1;  // |   40   |   48   |bytes
void kaboom_print(void) {
    printf("%d",__kaboom);
}

#endif


#define RETURN_CODE uint32_t
#define RETURN_OK 0
#define RETURN_ERR_GEN -1
#define RETURN_ERR_PARAM -2
#define RETURN_ERR_STATE -3 // the board is not the proper state to continue

#define PRINT_1_OUT_OF_(x) (x)

typedef enum {
    IMU_DATA_FLAG_NEW, //< received new data
    IMU_DATA_FLAG_SENT_HIGH_TRIBBLE,
    IMU_DATA_FLAG_SENT_MED_TRIBBLE,
    IMU_DATA_FLAG_SENT_LOW_TRIBBLE,
    IMU_DATA_FLAG_SENT_EMPTY_TRIBBLE, //< stay in sent EMPTY until NEW data is received by MSGQ
} IMU_DATA_FLAG_e;

typedef enum {
    DATA_TYPE_EULER_2NDBYTE = 0xCE, //< 2nd Byte value of stream if it is Euler
    DATA_LEN_EULER_3RDBYTE = 0x2C,  //< 3rd Byte value of stream if it is Euler
    DATA_TYPE_QUAT_2NDBYTE = 0xBC,  //< 2nd Byte value of stream if it is QUAT
    DATA_LEN_QUAT_3RDBYTE = 0x30    //< 3rd Byte value of stream if it is QUAT
} BINARY_DATA_FIELD_e;

typedef enum {
    RX_TYPE_DATA_ASCII,
    RX_TYPE_DATA_BINARY,
    RX_TYPE_CMD,
    RX_TYPE_MAX,
} RX_TYPE_e;

// Wave for names for Coil Driver Board DDS output and associated string name
#define macro_waveformDDS(T)                                                                                           \
    T(WAVEFORM_CHIRP)                                                                                                  \
    T(WAVEFORM_ECG)                                                                                                    \
    T(WAVEFORM_SINE)                                                                                                   \
    T(WAVEFORM_MAX)

GENERATE_ENUM_LIST(macro_waveformDDS, WAVEFORM_e)
#ifdef CREATE_STRING_DDS_LOOKUP_TABLE
GENERATE_ENUM_STRING_NAMES(macro_waveformDDS, WAVEFORM_e)
#else
extern const char *WAVEFORM_e_Strings[];
#endif

#undef macro_waveformDDS

// Identify all 6 DDSes on Coil driver board and DDS0_A & DDS1_A on the MCG board
// and their associated string name
#define macro_handlerDDS(T)                                                                                            \
    T(DDS0_A)                                                                                                          \
    T(DDS0_B)                                                                                                          \
    T(DDS0_C)                                                                                                          \
    T(DDS1_A)                                                                                                          \
    T(DDS1_B)                                                                                                          \
    T(DDS1_C)                                                                                                          \
    T(NUM_DDS_DEVICES)

GENERATE_ENUM_LIST(macro_handlerDDS, DDS_ID_e)
#ifdef CREATE_STRING_DDS_LOOKUP_TABLE
GENERATE_ENUM_STRING_NAMES(macro_handlerDDS, DDS_ID_e)
#else
extern const char *DDS_ID_e_Strings[];
#endif

#undef macro_handlerDDS

typedef struct __attribute__((packed)) {
    uint32_t alphaRoll;
    uint32_t alphaPitch;
    uint32_t alphaHeading;
    uint32_t accel[CARTESSIAN_COOR];
    uint32_t gyro[CARTESSIAN_COOR];
    int16_t magnet[CARTESSIAN_COOR];
    int16_t temperature;
} eulerData_t, *eulerData_tp;

typedef struct __attribute__((packed)) {
    uint32_t quat[QUATERNION_COOR];
    uint32_t accel[CARTESSIAN_COOR];
    uint32_t gyro[CARTESSIAN_COOR];
    int16_t magnet[CARTESSIAN_COOR];
    int16_t temperature;
} quaternionData_t, *quaternionData_tp;

typedef struct {
    IMU_DATA_FLAG_e flag;
    BINARY_DATA_FIELD_e dataType;
    union {
        uint8_t darray[sizeof(quaternionData_t)];
        uint32_t tribble[NUMBER_OF_IMU_FRAGMENTS][SIZE_OF_IMU_FRAGMENT];
        struct __attribute__((packed)) {
            uint32_t empty;
            eulerData_t eulerData;
        };
        quaternionData_t quanternionData;
    };
} imuData_t, *imuData_tp;

#define SNPRINTF_TEST_AND_ADD(tmp,nxt, action) if(tmp <= 0 ) {action;} nxt+=tmp

#endif /* INC_SAQTARGET_H_ */
