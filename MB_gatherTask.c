/*
 * MB_gatherTask.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 29, 2023
 *      Author: rlegault
 */

/*
 * Two buffers are maintained, This task will set all the clean bits
 * (identifying new data) for each DB section. As each db task gets new adc
 * readings it will get the Sema and then update its section, clearing its
 * clean bit
 * When the next start conversion interrupt fires, the gather task will lock
 * the semaphore, move the buffer to the new bufferlocation, set the clean bits
 * and send the old buffer over IP to the server. It will also gather stats for
 * all the db to identify any problematic system errors with timing.
 */

#define GENERATE_IPTYPE_STRING_NAMES
#include "MB_gatherTask.h"
#undef GENERATE_IPTYPE_STRING_NAMES
#include "MB_cncHandleMsg.h"
#include "cli/cli_print.h"
#include "cmsis_os.h"
#include "ctrlSpiCommTask.h"
#include "dbCommTask.h"
#include "dbTriggerTask.h"
#include "debugPrint.h"
#include "eeprom.h"
#include "imuLib.h"
#include "peripherals/MB_handleReg.h"
#include "pwmPinConfig.h"
#include "raiseIssue.h"
#include "realTimeClock.h"
#include "saqTarget.h"
#include "stmTarget.h"
#include "taskWatchdog.h"
#include "watchDog.h"
#include "webCliMisc.h"
#include <lwip/api.h>
#include <lwip/inet.h>
#include <lwip/opt.h>
#include <lwip/sys.h>
#include <stdbool.h>
#include <string.h>

#ifdef TRACEALYZER
#include "trcTypes.h"
extern TraceStringHandle_t gatherMsg;
#endif

#define ALL24BOARDSMASK 0x00FFFFFF
#define REMOVE_BOARD(mask, dbId) (mask & ((~(1 << dbId)) & ALL24BOARDSMASK))
#define DB_UP_RETRY_MAX 5 // try maximum of x times to bring up all the expected sensor boards.
#define WAIT_FOR_SENSOR_BOARDS_TO_SETTLE_MS(x) (x)

#define MAX_STREAM_DATA_PKT_IDX 2    // keep 2 buffers, one is being sent while the other is updated
#define MAX_ETHERNET_SIZE_BYTES 1500 // Max size before fragmentation

#define MAX_ADC_READING 4

#define SENSOR_BOARD_READING_VERSION 2
#define STREAM_PKT_VERSION 2

#define NEW_DATA_FLAG 0x80

#define NEW_DATA(value) (value & NEW_DATA_FLAG)

#define STRM_PKT_0 0
#define STRM_PKT_1 1
#define SENSOR_0 0
#define SENSOR_1 1

#define MAX_SEND_ERRORS 10 // close connection if too many errors;
#define MAX_ARR 0xFFFF

// 20 bytes
typedef struct __attribute__((packed)) {
    uint8_t version;  // sensor board reading structure version
    uint8_t sensorId; // SOURCE_READING_ID_e - FUTURE
    uint8_t boardId;  // board identifier of sensor source
    uint8_t flags;    // bit 7 (0x80) indicates if data is valid 1=valid, 0=invalid
    adc24Reading_t readings[MAX_ADC_READING*SENSORS_PER_BOARD];
    coilData_t ctrlData[SENSORS_PER_BOARD];
} sensorMCGBoardReadings_t, *sensorMCGBoardReadings_tp;

typedef struct __attribute__((packed)) {
    uint8_t version;  // sensor board reading structure version
    uint8_t sensorId; // SOURCE_READING_ID_e - FUTURE
    uint8_t boardId;  // board identifier of sensor source
    uint8_t flags;    // bit 7 (0x80) indicates if data is valid 1=valid, 0=invalid
    adc24Reading_t readings[MAX_ADC_READING*SENSORS_PER_BOARD];
} sensorECGBoardReadings_t, *sensorECGBoardReadings_tp;


#define QUATERNION_SIZE 48
#define EULER_SIZE 44
#define IMU_SPI_DATA_SPLIT_CNT 3

typedef enum { AXIS_X, AXIS_Y, AXIS_Z, AXIS_MAX } AXIS_e;

typedef enum { QUAT_0, QUAT_1, QUAT_2, QUAT_3, QUAT_MAX } QUAT_e;

typedef struct __attribute__((packed)) {
    uint32_t blank;
    eulerData_t data;
} euler_t, *euler_tp;

typedef struct __attribute__((packed)) {
    quaternionData_t data;
} quaternion_t, *quaternion_tp;

typedef struct __attribute__((packed)) {
    uint8_t version;  // sensor board reading structure version
    uint8_t sensorId; // SOURCE_READING_ID_e - FUTURE
    uint8_t boardId;  // board identifier of sensor source
    uint8_t flags;    // bit 7 (0x80) indicates if data is valid 1=valid, 0=invalid, bits 0,1 indicate board type
    union {
        quaternionData_t quat;
        euler_t euler;
        struct {
            uint8_t high4[QUATERNION_SIZE / IMU_SPI_DATA_SPLIT_CNT];
            uint8_t mid4[QUATERNION_SIZE / IMU_SPI_DATA_SPLIT_CNT];
            uint8_t low4[QUATERNION_SIZE / IMU_SPI_DATA_SPLIT_CNT];
        };
        uint8_t u8[sizeof(quaternionData_t)];
    };
} imuReadings_t, *imuReadings_tp;

#define BUS_SLOT_CNT 27

#define SIZE_OF_DATAREADINGS                                                                                           \
    (((sizeof(sensorMCGBoardReadings_t) * MAX_CS_ID +                                                 \
                           (BUS_SLOT_CNT - MAX_CS_ID) * (sizeof(imuReadings_t) - sizeof(sensorMCGBoardReadings_t)))))

// From Schematic for MCG Board coil connected to ADC
typedef enum {
    MCG_READING_COIL_C = 0,
    MCG_READING_COIL_B,
    MCG_READING_COIL_A,
    MCG_READING_TEMPERATURE,
} MCG_READING_LOCATION_e;

// From Schematic for ECG Board coil connected to ADC
typedef enum {
    ECG_READING_AUX0A = 0,
    ECG_READING_AUX0B,
    ECG_READING_AUX0C,
    ECG_READING_AUX1A,
    ECG_READING_AUX1B,
    ECG_READING_AUX1C,
    ECG_READING_ECG,
    ECG_READING_AWG
} ECG_READING_LOCATION_e;

typedef enum {
    ECG12_READING_V6 = 0,
    ECG12_READING_LA,  // this is LA-RA  LEAD-I
    ECG12_READING_LL,  // this is LL-RA  LEAD-II
    ECG12_READING_V2,
    ECG12_READING_V3,
    ECG12_READING_V4,
    ECG12_READING_V5,
    ECG12_READING_V1
} ECG12_READING_LOCATION_e;

// last calculated sizeof is 1168 bytes
typedef struct __attribute__((packed)) {
    uint32_t uid; // packet UID
    uint8_t version;
    uint8_t mcgReadingCnt;   // number of elements in mcgReading[]
    uint8_t ecgReadingCnt;   // number of elements in ecgReading[]
    uint8_t ecg12ReadingCnt; // number of elements in ecg12
    uint8_t imuReadingCnt;   // number of elements in imuReading[]
    uint8_t dummy[3];        // empty, 3 to align on 32bit
    double timeStamp;        // seconds since epoch
    // Note this only works because A IMU board takes two slots so replacing a sensor board with a coil board always
    // reduces the size. But there are built in 3 busses that can take a coil driver board without replacing 2 sensor
    // boards.
    uint8_t dataReadings[SIZE_OF_DATAREADINGS];
    // data readings structure is dynamic based on sensor board population
    // first x blocks in dataReadings are all the mcg readings
    // sencond y block in datareadings are all the ecg readings
    // third z block in datareadings are all the imu readings.
} streamSensorPkt_t, *streamSensorPkt_tp;

_Static_assert(sizeof(streamSensorPkt_t) < MAX_ETHERNET_SIZE_BYTES);

#if 0 // macro to print sizeof values at compile time
char (*__kaboom)[sizeof(streamSensorPkt_t)] = 1;
void kaboom_print(void) {
    printf("%d",__kaboom);
}
#endif

typedef struct {
    osSemaphoreId access;
    uint32_t streamDataIdx;
    uint32_t streamPktDataSize;
    streamSensorPkt_t streamPktData[MAX_STREAM_DATA_PKT_IDX];
} streamData_t, *streamData_tp;

typedef struct {
    bool statusEn;
    uint32_t missedData;
    uint32_t sentPkts[IMU_PER_BOARD];
    uint32_t overWrittenData;
    uint32_t alignmentData;
    uint32_t accessBlocked;
} dbStats_t;

typedef struct {
    dbStats_t db[MAX_CS_ID];
    uint32_t accessBlockCnt;
    uint32_t notifyTimeoutCnt;
    uint32_t sentPkts;
} gatherStats_t;

#define MB_GATHER_TASK_TIMEOUT_MS 20

#define INTERVAL_1ms 1000
#define INTERVAL_5s 5000000

#ifdef STM32F767xx
#define ARRREG 64799
#else
#define ARRREG 47999 // use this for both 1ms and 5000ms
#endif

double TS_DELTA = 0.001;

#define ONE_MICRO_SECOND 1000000

typedef union {
    sensorMCGBoardReadings_tp p_MCGsensors;
    sensorECGBoardReadings_tp p_ECGsensors;
    imuReadings_tp p_imu;
    uint8_t *p_uint8;
} sensor_u;

typedef struct {
    sensor_u dataLocation[MAX_STREAM_DATA_PKT_IDX][SENSORS_PER_BOARD];
    BOARDTYPE_e configBoardType;
    BOARDTYPE_e hwBoardType;
    bool match; // board type configuration matches read board type

    int boardTypeIdx; // used by IMU data to store the partial data structures to
                      // assemble them in the holding area
} sensorBoardDataLocation_t, *sensorBoardDataLocation_tp;

static __DTCMRAM__ sensorBoardDataLocation_t sensorBoardDataLocation[MAX_CS_ID] = {0};
static __DTCMRAM__ uint32_t sensorBoardCnt[BOARDTYPE_MAX] = {0};
bool useUdpChan = true;
struct netconn *tcpConn = NULL;

/**
 * @fn
 *
 * @brief Set the global array sensorBoardDataLocation to uninitialized data values
 **/
static void sensorBoardDataLocationInit(void) {
    memset(sensorBoardDataLocation, 0, sizeof(sensorBoardDataLocation));
    for (int i = 0; i < MAX_STREAM_DATA_PKT_IDX; i++) {
        sensorBoardDataLocation[i].configBoardType = BOARDTYPE_UNKNOWN;
        sensorBoardDataLocation[i].hwBoardType = BOARDTYPE_UNKNOWN;
    }
}

/**
 * @fn
 *
 * @brief Main board gather thread, responsible for packing up the next Ethernet data packet and sending it out.
 *        It is woken by a timer to send each packet.
 * @param arg, unused.
 *
 **/
static void mbGatherThread(const void *arg);

// Stream data is stored in memory that is not cached and accessible by the ETHERNET PHY
streamData_t streamData __attribute__((section(".streamDataSection")));

__DTCMRAM__ osThreadId mbGatherTaskHandle = NULL;

static __DTCMRAM__ imuData_t imuDataStorage[IMU_MAX_BOARD][IMU_PER_BOARD] = {0};
static __DTCMRAM__ imuReadings_t
    g_imuData[IMU_PER_BOARD]; // Used for printing the last imu data captured from any device.
static __DTCMRAM__ gatherStats_t gatherStats;
static __DTCMRAM__ StaticTask_t mbGatherTaskCtrlBlock;
static __DTCMRAM__ StackType_t mbGatherTaskStack[MBGATHER_STACK_WORDS];
static __DTCMRAM__ osStaticMutexDef_t streamDataAccessCtrlSema;

uint32_t sendErrCnt = 0;

__ITCMRAM__ void sendTcpData(void *p_data, size_t dataLen);
__ITCMRAM__ void sendUdpData(void *p_data, size_t dataLen);

/**
 * @fn
 *
 * @brief CLose the Tcp connection
 *
 **/
inline void closeTcpConnection(void);

/**
 * @fn
 *
 * @brief Thread to handle waiting on TCP connections. Will only allow one TCP connection at a time.
 *
 * @param arg - unused
 *
 **/
static void waitOnTcpServerConnectionThread(const void *arg);

/**
 * @fn
 *
 * @brief Create the Ethernet packet structure based on stored board configurations
 *
 * @return  Returns the expected mask representing all expected boards
 **/
static uint32_t createPktStructure(void);

void setUdpSendInterval(uint32_t interval_us) {
    // 16bit

    double pscD = (pwmMap[TIM_UDP_TX_SIGNAL].clockFrequency / ONE_MICRO_SECOND) * interval_us /
                  (pwmMap[TIM_UDP_TX_SIGNAL].maxArr);
    uint32_t psc = floor(pscD);
    uint32_t arr = (pwmMap[TIM_UDP_TX_SIGNAL].clockFrequency / ONE_MICRO_SECOND) * interval_us / (psc + 1) - 1;

    TS_DELTA = interval_us / (ONE_MICRO_SECOND * 1.0);

    if (interval_us == INTERVAL_1ms) {
        arr = ARRREG;
        psc = (pwmMap[TIM_UDP_TX_SIGNAL].clockFrequency / ONE_MICRO_SECOND * interval_us) / (arr + 1) - 1;
    } else if (interval_us == INTERVAL_5s) {
        arr = ARRREG;
        psc = (pwmMap[TIM_UDP_TX_SIGNAL].clockFrequency / ONE_MICRO_SECOND * interval_us) / (arr + 1) - 1;
    }

    pwmMap[TIM_UDP_TX_SIGNAL].htim->Init.Prescaler = psc;
    pwmMap[TIM_UDP_TX_SIGNAL].htim->Init.Period = arr;
    pwmMap[TIM_UDP_TX_SIGNAL].htim->Instance->PSC = psc;
    pwmMap[TIM_UDP_TX_SIGNAL].htim->Instance->ARR = arr;
    pwmMap[TIM_UDP_TX_SIGNAL].htim->Instance->CCR1 = arr / 2;
    DPRINTF_INFO("UDP TX INTERVAL %u us TIM13 PSC=%u, ARR=%u, PULSE=%u\r\n", interval_us, psc, arr, arr / 2);
}

void tcpDataServerWaitTaskInit(int priority, int stackSize) {
    osThreadDef(tcpServerConnection, waitOnTcpServerConnectionThread, priority, 0, stackSize);
    osThreadId thread = osThreadCreate(osThread(tcpServerConnection), NULL);
    assert(thread != NULL);
}

void mbGatherTaskInit(int priority, int stackSize) {

    sensorBoardDataLocationInit();
    memset(&streamData, 0, sizeof(streamData_t));

    osMutexStaticDef(streamDataAccess, &streamDataAccessCtrlSema);
    streamData.streamDataIdx = 0;

    streamData.access = osMutexCreate(osMutex(streamDataAccess));
    assert(streamData.access != NULL);

    osThreadStaticDef(mbGatherTask, mbGatherThread, priority, 0, stackSize, mbGatherTaskStack, &mbGatherTaskCtrlBlock);
    mbGatherTaskHandle = osThreadCreate(osThread(mbGatherTask), NULL);
    assert(mbGatherTaskHandle != NULL);

    registerInfo_t regInfo = {.mbId = IP_TX_DATA_TYPE, .type = DATA_UINT};
    registerRead(&regInfo);
    if (regInfo.u.dataUint == IPTYPE_TCP) {
        useUdpChan = false;
    } else {
        useUdpChan = true;
    }

    // read the default register value and then use it to set the value
    regInfo.mbId = STREAM_INTERVAL_US;
    registerRead(&regInfo);
    setUdpSendInterval(regInfo.u.dataUint);
}

__ITCMRAM__ void setDaughterboardState(int boardId, bool enable) {
    assert(boardId < MAX_CS_ID);
    if (gatherStats.db[boardId].statusEn == enable) {
        return;
    }
    gatherStats.db[boardId].statusEn = enable;
    updateSPIEnableCount(boardId, enable);
}

__ITCMRAM__ void updateSensorData(dbCommThreadInfo_tp p_threadInfo, uint8_t *sensorReadings, size_t sensorReadingCnt) {
    assert(p_threadInfo->daughterBoardId < MAX_CS_ID);
    osStatus result = osMutexWait(streamData.access, 2);
    if (result == osOK) {

        rx_dbNopPayload_tp p_payload = (rx_dbNopPayload_tp)sensorReadings;


        switch (sensorBoardDataLocation[p_threadInfo->daughterBoardId].configBoardType) {
        case BOARDTYPE_MCG: {
            // THE MCG contains extra data at the end of the sensor data, the ECG do not.
            const size_t cpySz = sizeof(coilData_t)*SENSORS_PER_BOARD + sizeof(adc24Reading_t) * NUMBER_OF_SENSOR_READINGS;
            sensorMCGBoardReadings_tp p_readings = sensorBoardDataLocation[p_threadInfo->daughterBoardId]
                                                    .dataLocation[streamData.streamDataIdx][SENSOR_0]
                                                    .p_MCGsensors;

            if (NEW_DATA(p_readings->flags)) {
                gatherStats.db[p_threadInfo->daughterBoardId].overWrittenData++;
            }

            p_readings->boardId = p_threadInfo->daughterBoardId;
            p_readings->sensorId = SENSOR_0;
            p_readings->version = STREAM_PKT_VERSION;
            p_readings->flags = NEW_DATA_FLAG | sensorBoardDataLocation[p_threadInfo->daughterBoardId].configBoardType;

            memcpy(p_readings->readings, &p_payload->adc[SENSOR_0], cpySz);
            gatherStats.db[p_threadInfo->daughterBoardId].sentPkts[0]++;
        }
        break;
        case BOARDTYPE_ECG:
        case BOARDTYPE_12ECG:
            {
            const size_t cpySz = sizeof(adc24Reading_t) * NUMBER_OF_SENSOR_READINGS;
            sensorECGBoardReadings_tp p_readings = sensorBoardDataLocation[p_threadInfo->daughterBoardId]
                                                    .dataLocation[streamData.streamDataIdx][SENSOR_0]
                                                    .p_ECGsensors;

            if (NEW_DATA(p_readings->flags)) {
                gatherStats.db[p_threadInfo->daughterBoardId].overWrittenData++;
            }

            p_readings->boardId = p_threadInfo->daughterBoardId;
            p_readings->sensorId = SENSOR_0;
            p_readings->version = STREAM_PKT_VERSION;
            p_readings->flags = NEW_DATA_FLAG | sensorBoardDataLocation[p_threadInfo->daughterBoardId].configBoardType;

            memcpy(p_readings->readings, &p_payload->adc[SENSOR_0], cpySz);
            gatherStats.db[p_threadInfo->daughterBoardId].sentPkts[0]++;
            break;
            }
        break;
        case BOARDTYPE_IMU_COIL:
            for (int imuIdx = IMU0_IDX; imuIdx <= IMU1_IDX; imuIdx++) {
                switch (PAYLOAD_GET_FLAG(imuIdx, p_payload->imuFlag)) {
                case IMU_DATA_FLAG_SENT_HIGH_TRIBBLE:
                    if (imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                            .flag != IMU_DATA_FLAG_NEW) {
                        gatherStats.db[p_threadInfo->daughterBoardId].overWrittenData++;
                    }
                    imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx].flag =
                        IMU_DATA_FLAG_SENT_HIGH_TRIBBLE;
                    memcpy(&imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                                .tribble[PAYLOAD_TRIBLE_INDEX(IMU_DATA_FLAG_SENT_HIGH_TRIBBLE)],
                           p_payload->imu[imuIdx],
                           SIZE_OF_IMU_FRAGMENT * sizeof(uint32_t));
                    break;
                case IMU_DATA_FLAG_SENT_MED_TRIBBLE:
                    if (imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                            .flag != IMU_DATA_FLAG_SENT_HIGH_TRIBBLE) {
                        if (imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                                .flag != IMU_DATA_FLAG_NEW) {
                            gatherStats.db[p_threadInfo->daughterBoardId].alignmentData++;
                        }
                        imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                            .flag = IMU_DATA_FLAG_NEW;
                    } else {
                        imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                            .flag = IMU_DATA_FLAG_SENT_MED_TRIBBLE;
                        memcpy(
                            &imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                                 .tribble[PAYLOAD_TRIBLE_INDEX(IMU_DATA_FLAG_SENT_MED_TRIBBLE)],
                            p_payload->imu[imuIdx],
                            sizeof(p_payload->imu[0]));
                    }
                    break;
                case IMU_DATA_FLAG_SENT_LOW_TRIBBLE:
                    if (imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                            .flag != IMU_DATA_FLAG_SENT_MED_TRIBBLE) {
                        if (imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                                .flag != IMU_DATA_FLAG_NEW) {
                            gatherStats.db[p_threadInfo->daughterBoardId].alignmentData++;
                        }
                        imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                            .flag = IMU_DATA_FLAG_NEW;
                    } else {
                        imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                            .flag = IMU_DATA_FLAG_NEW;
                        memcpy(
                            &imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                                 .tribble[PAYLOAD_TRIBLE_INDEX(IMU_DATA_FLAG_SENT_LOW_TRIBBLE)],
                            p_payload->imu[imuIdx],
                            sizeof(p_payload->imu[0]));

                        // send data in ethernet packet
                        imuReadings_tp p_imuReadings = sensorBoardDataLocation[p_threadInfo->daughterBoardId]
                                                           .dataLocation[streamData.streamDataIdx][imuIdx]
                                                           .p_imu;
                        p_imuReadings->boardId = p_threadInfo->daughterBoardId;
                        p_imuReadings->version = STREAM_PKT_VERSION;
                        p_imuReadings->flags =
                            NEW_DATA_FLAG | sensorBoardDataLocation[p_threadInfo->daughterBoardId].configBoardType;
                        p_imuReadings->sensorId = imuIdx;

                        memcpy(
                            &p_imuReadings->u8[0],
                            imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                                .darray,
                            sizeof(quaternionData_t));
                        memcpy(&g_imuData[imuIdx].u8[0], &p_imuReadings->u8[0], sizeof(quaternionData_t));
                        displaySentBinaryData(&p_imuReadings->u8[4], DATA_TYPE_EULER_2NDBYTE);
                        gatherStats.db[p_threadInfo->daughterBoardId].sentPkts[imuIdx]++;
                    }
                    break;
                case IMU_DATA_FLAG_NEW:
                    // fall through
                case IMU_DATA_FLAG_SENT_EMPTY_TRIBBLE:
                    if (imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx]
                            .flag != IMU_DATA_FLAG_NEW) {
                        gatherStats.db[p_threadInfo->daughterBoardId].alignmentData++;
                    }
                    imuDataStorage[sensorBoardDataLocation[p_threadInfo->daughterBoardId].boardTypeIdx][imuIdx].flag =
                        IMU_DATA_FLAG_NEW;
                    break;
                default:
                    static uint32_t throttleCnt = 0;
                    if (THROTTLE_PRINT_OUTPUT(throttleCnt, PRINT_1_OUT_OF_(1000))) {
                        DPRINTF_ERROR("Unknown IMU%d switch flag %d %u\r\n",
                                      imuIdx,
                                      PAYLOAD_GET_FLAG(imuIdx, p_payload->imuFlag));
                    }
                    break;
                }  // end switch(payload flag)
            }      // end for(imuIdx
            break; // end of case BOARDTYPE_IMU_COIL:
        default:
            break;
        }

        osMutexRelease(streamData.access);
    } else {
        gatherStats.db[p_threadInfo->daughterBoardId].accessBlocked++;
    }
}

void handleCncReadIntRegisterRequest(int destination, uint32_t address, uint32_t *value, uint32_t *uid) {
    cncMsgPayload_t cncPayload = {
        .cncMsgPayloadHeader.peripheral = PER_MCU, .cncMsgPayloadHeader.action = CNC_ACTION_READ, .cncMsgPayloadHeader.addr = address, .value = *value, .cncMsgPayloadHeader.cncActionData.result = 0xFF};
    webCncCbId_t cncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};
    cncSendMsg(destination, SPICMD_CNC, (cncPayload_tp)&cncPayload, *uid, webCncRequestCB, (uint32_t)&cncCbId);
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
    DPRINTF("TaskNotify osResult=%d\r\n", osResult);
    if (osResult == TASK_NOTIFY_OK) {
        if (cncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result != 0) {
            DPRINTF_INFO("No Register value from sensor board %d at address %d\r\n", destination, address);
            *value = 0xFF;
        } else {
            *value = cncCbId.p_payload->cmd.value;
        }
    } else {
        assert(false);
        // CNC task to timeout first.
    }
}

void handleCncReadStringRegisterRequest(
    int destination, uint32_t address, char *value, uint32_t value_len, uint32_t *uid) {
    cncMsgPayload_t cncPayload = {
        .cncMsgPayloadHeader.peripheral = PER_MCU, .cncMsgPayloadHeader.action = CNC_ACTION_READ, .cncMsgPayloadHeader.addr = address, .value = *value, .cncMsgPayloadHeader.cncActionData.result = 0xFF};
    webCncCbId_t cncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};
    cncSendMsg(destination, SPICMD_CNC, (cncPayload_tp)&cncPayload, *uid, webCncRequestCB, (uint32_t)&cncCbId);
    uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
    if (osResult == TASK_NOTIFY_OK) {
        if (cncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result != 0) {
            value[0] = '\0';
        } else {
            strncpy(value, cncCbId.p_payload->cmd.str, value_len);
            value[value_len - 1] = '\0';
        }
    } else {
        assert(false);
        // CNC task to timeout first.
    }
}

bool verifySensorConfiguration(void) {
    bool allMatched = true;
    for (int i = EEPROM_SENSOR_BOARD_0; i <= EEPROM_SENSOR_BOARD_23; i++) {
        uint32_t boardIdx = i - EEPROM_SENSOR_BOARD_0;
        // get information from each daughter board for verification

        if (sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType != BOARDTYPE_EMPTY) {
            uint32_t hwType = 0;
            uint32_t uid = 0;
            if (sensorBoardPresent(boardIdx)) {
                handleCncReadIntRegisterRequest(boardIdx, SB_HW_TYPE, &hwType, &uid);
            } else {
                hwType = BOARDTYPE_UNKNOWN;
            }
            sensorBoardDataLocation[boardIdx].hwBoardType = hwType;
            sensorBoardDataLocation[boardIdx].match = (sensorBoardDataLocation[boardIdx].configBoardType == hwType);
            if (sensorBoardDataLocation[boardIdx].configBoardType != hwType) {
                allMatched = false;
                DPRINTF_WARN("Board %d type %d does not match config %d\r\n",
                             boardIdx,
                             hwType,
                             sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType);
            }
        } else {
            sensorBoardDataLocation[boardIdx].hwBoardType = BOARDTYPE_EMPTY;
            sensorBoardDataLocation[boardIdx].match = true;
        }
    }
    DPRINTF("allMatched=%d\r\n", allMatched);
    return allMatched;
}

void jsonAddConfigSettings(int destination, json_object *jsonArray) {

    uint32_t minDest = 0;
    uint32_t maxDest = 0;

    if (destination == DESTINATION_ALL) {
        minDest = 0;
        maxDest = MAX_CS_ID;
    } else {
        minDest = destination;
        maxDest = destination + 1;
    }

    for (int i = minDest; i < maxDest; i++) {
        json_object *next = json_object_new_object();
        json_object *jint = json_object_new_int(i);
        json_object_object_add_ex(next, "Destination", jint, JSON_C_OBJECT_KEY_IS_CONSTANT);
        int type = sensorBoardDataLocation[i].configBoardType;
        type = (type < BOARDTYPE_MAX) ? type : BOARDTYPE_MAX;
        json_object *jstr = json_object_new_string(BOARDTYPE_e_Strings[type]);
        json_object_object_add_ex(next, "CFG_TYPE", jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
        json_object_array_add(jsonArray, next);
    }
    return;
}

void jsonAddConfigStatus(int destination, json_object *jsonArray) {

    uint32_t minDest = 0;
    uint32_t maxDest = 0;

    if (destination == DESTINATION_ALL) {
        minDest = 0;
        maxDest = MAX_CS_ID;
    } else {
        minDest = destination;
        maxDest = destination + 1;
    }

    for (int i = minDest; i < maxDest; i++) {
        json_object *next = json_object_new_object();
        json_object *jint = json_object_new_int(i);
        json_object_object_add_ex(next, "Destination", jint, JSON_C_OBJECT_KEY_IS_CONSTANT);

        int type = sensorBoardDataLocation[i].configBoardType;
        type = (type < BOARDTYPE_MAX) ? type : BOARDTYPE_MAX;
        json_object *jstr = json_object_new_string(BOARDTYPE_e_Strings[type]);
        json_object_object_add_ex(next, "CFG_TYPE", jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);

        bool present = sensorBoardPresent(i);
        json_object *jpresent = json_object_new_boolean(present);
        json_object_object_add_ex(next, "PRESENT", jpresent, JSON_C_OBJECT_KEY_IS_CONSTANT);

        if (present) {

            cncMsgPayload_t cncPayload = {
                .cncMsgPayloadHeader.peripheral = PER_MCU, .cncMsgPayloadHeader.action = CNC_ACTION_READ, .cncMsgPayloadHeader.addr = SB_HW_TYPE, .cncMsgPayloadHeader.cncActionData.result = 0xFF};
            webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};
            cncSendMsg(i, SPICMD_CNC, (cncPayload_tp)&cncPayload, 0, webCncRequestCB, (uint32_t)&webCncCbId);
            uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
            DPRINTF_CMD_STREAM(
                "%s %d payload=%s\r\n", __FUNCTION__, __LINE__, (char *)(&webCncCbId.p_payload->cmd.str));
            if (osResult == TASK_NOTIFY_OK) {
                if (webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {
                    json_object *jBoardType = json_object_new_int(webCncCbId.p_payload->cmd.value);
                    json_object_object_add_ex(next, "BOARD_TYPE", jBoardType, JSON_C_OBJECT_KEY_IS_CONSTANT);
                } else {
                    json_object *jstr = json_object_new_string("NOT AVAILABLE");
                    json_object_object_add_ex(next, "BOARD_TYPE", jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
                }
            }
        } else {
            json_object *jstr = json_object_new_string("NOT AVAILABLE");
            json_object_object_add_ex(next, "BOARD_TYPE", jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }

        if (present) {
            handleCncRequestStrJson(next, i, CNC_ACTION_READ, SB_SERIAL_NUMBER, 0, 0, "SERIAL");
        } else {
            json_object *jstr = json_object_new_string("NOT PRESENT");
            json_object_object_add_ex(next, "SERIAL", jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }

        if (present) {
            cncMsgPayload_t cncPayload = {
                .cncMsgPayloadHeader.peripheral = PER_MCU, .cncMsgPayloadHeader.action = CNC_ACTION_READ, .cncMsgPayloadHeader.addr = SB_PERIPHERAL_FAIL_MASK, .cncMsgPayloadHeader.cncActionData.result = 0xFF};
            webCncCbId_t webCncCbId = {.taskHandle = xTaskGetCurrentTaskHandle(), .p_payload = NULL, .xInfo = 0};
            cncSendMsg(i, SPICMD_CNC, (cncPayload_tp)&cncPayload, 0, webCncRequestCB, (uint32_t)&webCncCbId);
            uint32_t osResult = ulTaskNotifyTake(true, TRANSIENT_TASK_NOTIFY_TIMEOUT_MS);
            DPRINTF_CMD_STREAM(
                "%s %d payload=%s\r\n", __FUNCTION__, __LINE__, (char *)(&webCncCbId.p_payload->cmd.str));

            if (osResult == TASK_NOTIFY_OK) {

                if (webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result == 0) {

                    json_object *errorArray = json_object_new_array();
                    if (webCncCbId.p_payload->cmd.value == 0) {
                        json_object *jPeripheralErrorValue = json_object_new_string("NONE");
                        json_object_array_add(errorArray, jPeripheralErrorValue);
                        json_object *jstr = json_object_new_string("OK");
                        json_object_object_add_ex(next, "BOARD STATUS", jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);
                    } else {
                        for (int i = 0; i < PER_MAX; i++) {
                            if ((1 << i) & webCncCbId.p_payload->cmd.value) {
                                json_object *jstrIdx = json_object_new_string(PERIPHERAL_e_Strings[i]);
                                json_object_array_add(errorArray, jstrIdx);
                            }
                        }
                        json_object *jstrError = json_object_new_string("ERROR");
                        json_object_object_add_ex(next, "BOARD STATUS", jstrError, JSON_C_OBJECT_KEY_IS_CONSTANT);
                    }
                    json_object_object_add_ex(next, "PERIPHERAL_ERRORS", errorArray, JSON_C_OBJECT_KEY_IS_CONSTANT);

                } else {
                    json_object *jstrNA = json_object_new_string("NOT AVAILABLE");
                    json_object_object_add_ex(next, "PERIPHERAL_ERRORS", jstrNA, JSON_C_OBJECT_KEY_IS_CONSTANT);
                }

            } else {
                json_object *jstrNA2 = json_object_new_string("NOT AVAILABLE");
                json_object_object_add_ex(next, "BOARD STATUS", jstrNA2, JSON_C_OBJECT_KEY_IS_CONSTANT);
            }

        } else {
            json_object *jstrNP = json_object_new_string("NOT PRESENT");
            json_object_object_add_ex(next, "BOARD STATUS", jstrNP, JSON_C_OBJECT_KEY_IS_CONSTANT);
        }

        json_object_array_add(jsonArray, next);
    }
    return;
}

void alterConfigSettings(int destination, BOARDTYPE_e boardType) {

    uint32_t minDest = 0;
    uint32_t maxDest = 0;

    if (destination == DESTINATION_ALL) {
        minDest = 0;
        maxDest = MAX_CS_ID;
    } else {
        minDest = destination;
        maxDest = destination + 1;
    }

    for (int i = minDest; i < maxDest; i++) {
        registerInfo_t regInfo = {.mbId = SENSOR_BOARD_0 + i, .type = DATA_UINT, .u.dataUint = boardType};
        registerWrite(&regInfo);
    }
    return;
}

void jsonDetailConfigurationError(json_object *jsonArray) {

    for (int i = EEPROM_SENSOR_BOARD_0; i <= EEPROM_SENSOR_BOARD_23; i++) {
        if (sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].match == false) {
            json_object *next = json_object_new_object();
            json_object *jint = json_object_new_int(i - EEPROM_SENSOR_BOARD_0);
            json_object_object_add_ex(next, "Destination", jint, JSON_C_OBJECT_KEY_IS_CONSTANT);

            int type = sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].hwBoardType;
            type = (type < BOARDTYPE_MAX) ? type : BOARDTYPE_MAX;
            json_object *jstr = json_object_new_string(BOARDTYPE_e_Strings[type]);
            json_object_object_add_ex(next, "HW_TYPE", jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);

            type = sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType;
            type = (type < BOARDTYPE_MAX) ? type : BOARDTYPE_MAX;
            jstr = json_object_new_string(BOARDTYPE_e_Strings[type]);
            json_object_object_add_ex(next, "CFG_TYPE", jstr, JSON_C_OBJECT_KEY_IS_CONSTANT);

            json_object_array_add(jsonArray, next);
        }
    }
}

void cliDetailConfigurationError(CLI *hCli) {
    for (int i = EEPROM_SENSOR_BOARD_0; i <= EEPROM_SENSOR_BOARD_23; i++) {
        if (sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].match == false) {
            CliPrintf(hCli,
                      "Mismatch at Slot %d Type=%d Config Type=%d\r\n",
                      i - EEPROM_SENSOR_BOARD_0,
                      sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].hwBoardType,
                      sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType);
        }
    }
}
/*
 * This function reads the slot registers and counts all the MCG, ECG, IMU boards
 * It then creates a structure that corresponds to the number and assigns
 * data pointers to the location to write sensor data. This allows for quick and
 * consistent locations of board/sensor data within the streaming packet.
 *
 * Returns the expected mask representing all expected boards
 */

uint32_t createPktStructure(void) {
    uint32_t boardType;
    uint32_t mask = ALL24BOARDSMASK; // assume all 24 boards are present;
    eepromOpen(osWaitForever);
    for (int i = EEPROM_SENSOR_BOARD_0; i <= EEPROM_SENSOR_BOARD_23; i++) {
        eepromReadRegister(i, (uint8_t *)&boardType, sizeof(boardType));
        switch (boardType) {
        case BOARDTYPE_MCG:
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].boardTypeIdx = sensorBoardCnt[BOARDTYPE_MCG]++;
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType = BOARDTYPE_MCG;
            break;
        case BOARDTYPE_ECG:
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].boardTypeIdx = sensorBoardCnt[BOARDTYPE_ECG]++;
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType = BOARDTYPE_ECG;
            break;
        case BOARDTYPE_12ECG:
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].boardTypeIdx = sensorBoardCnt[BOARDTYPE_12ECG]++;
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType = BOARDTYPE_12ECG;
            break;
        case BOARDTYPE_IMU_COIL:
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].boardTypeIdx = sensorBoardCnt[BOARDTYPE_IMU_COIL]++;
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType = BOARDTYPE_IMU_COIL;
            break;
        default:
            mask = REMOVE_BOARD(mask, i);
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].boardTypeIdx = sensorBoardCnt[BOARDTYPE_EMPTY]++;
            sensorBoardDataLocation[i - EEPROM_SENSOR_BOARD_0].configBoardType = BOARDTYPE_EMPTY;
            break;
        }
    }
    eepromClose();
    uint32_t mcgOffset = 0;
    uint32_t ecgOffset = sensorBoardCnt[BOARDTYPE_MCG] * sizeof(sensorMCGBoardReadings_t);
    uint32_t origEcgOffset = ecgOffset;
    uint32_t ecg12Offset = ecgOffset + (sensorBoardCnt[BOARDTYPE_ECG] * sizeof(sensorECGBoardReadings_t));
    uint32_t origEcg12Offset = ecg12Offset;
    uint32_t imuOffset = ecg12Offset + (sensorBoardCnt[BOARDTYPE_12ECG] * sizeof(sensorECGBoardReadings_t));
    uint32_t origImuOffset = imuOffset;

    if (sensorBoardCnt[BOARDTYPE_IMU_COIL] > IMU_MAX_BOARD) {
        DPRINTF_ERROR("Too many Coil boards, %d\r\n", sensorBoardCnt[BOARDTYPE_IMU_COIL]);
    }

    for (int i = 0; i < MAX_CS_ID; i++) {
        switch (sensorBoardDataLocation[i].configBoardType) {
        case BOARDTYPE_MCG:
            sensorBoardDataLocation[i].dataLocation[STRM_PKT_0][SENSOR_0].p_uint8 =
                &streamData.streamPktData[STRM_PKT_0].dataReadings[mcgOffset];
            sensorBoardDataLocation[i].dataLocation[STRM_PKT_1][SENSOR_0].p_uint8 =
                &streamData.streamPktData[STRM_PKT_1].dataReadings[mcgOffset];
            mcgOffset += sizeof(sensorMCGBoardReadings_t);
            break;
        case BOARDTYPE_12ECG:
            // fall through
        case BOARDTYPE_ECG:
            sensorBoardDataLocation[i].dataLocation[STRM_PKT_0][SENSOR_0].p_uint8 =
                &streamData.streamPktData[STRM_PKT_0].dataReadings[ecgOffset];
            sensorBoardDataLocation[i].dataLocation[STRM_PKT_1][SENSOR_0].p_uint8 =
                &streamData.streamPktData[STRM_PKT_1].dataReadings[ecgOffset];
            ecgOffset += sizeof(sensorECGBoardReadings_t);

            break;
        case BOARDTYPE_IMU_COIL:
            sensorBoardDataLocation[i].dataLocation[STRM_PKT_0][SENSOR_0].p_uint8 =
                &streamData.streamPktData[STRM_PKT_0].dataReadings[imuOffset];
            sensorBoardDataLocation[i].dataLocation[STRM_PKT_1][SENSOR_0].p_uint8 =
                &streamData.streamPktData[STRM_PKT_1].dataReadings[imuOffset];
            imuOffset += sizeof(imuReadings_t);

            sensorBoardDataLocation[i].dataLocation[STRM_PKT_0][SENSOR_1].p_uint8 =
                &streamData.streamPktData[STRM_PKT_0].dataReadings[imuOffset];
            sensorBoardDataLocation[i].dataLocation[STRM_PKT_1][SENSOR_1].p_uint8 =
                &streamData.streamPktData[STRM_PKT_1].dataReadings[imuOffset];
            imuOffset += sizeof(imuReadings_t);
            break;
        default:
            break;
        }
    }

    size_t pktSize = &streamData.streamPktData[STRM_PKT_0].dataReadings[0] -
                     (uint8_t *)(&streamData.streamPktData[STRM_PKT_0]) + imuOffset;
    streamData.streamPktDataSize = pktSize;
    DPRINTF_RAW("###########################################\r\n");
    DPRINTF_RAW("#\tPacket Stream information\r\n");
    DPRINTF_RAW("#\tStream Total size = %d\r\n", pktSize);
    DPRINTF_RAW("#\tMax Stream size = %d\r\n", sizeof(streamSensorPkt_t));
    DPRINTF_RAW("#\tData only Size = %d\r\n", imuOffset);
    DPRINTF_RAW("#\tMCG Boards = %d, ECG Boards = %d, ECG12 Boards = %d, IMU Boards = %d\r\n",
                sensorBoardCnt[BOARDTYPE_MCG],
                sensorBoardCnt[BOARDTYPE_ECG],
                sensorBoardCnt[BOARDTYPE_12ECG],
                sensorBoardCnt[BOARDTYPE_IMU_COIL]);
    DPRINTF_RAW("###########################################\r\n");
    assert(mcgOffset <= origEcgOffset);
    assert(ecg12Offset <= origEcg12Offset);
    assert(ecgOffset <= origImuOffset);
    assert(imuOffset <= SIZE_OF_DATAREADINGS);
    return mask;
}

__ITCMRAM__ static inline void setStreamPktHeader(int idx, double timeStamp) {
    static uint32_t uid = 0;
    assert(idx < MAX_CS_ID);
    streamData.streamPktData[idx].ecgReadingCnt = sensorBoardCnt[BOARDTYPE_ECG];
    streamData.streamPktData[idx].ecg12ReadingCnt = sensorBoardCnt[BOARDTYPE_12ECG];
    streamData.streamPktData[idx].imuReadingCnt = sensorBoardCnt[BOARDTYPE_IMU_COIL];
    streamData.streamPktData[idx].mcgReadingCnt = sensorBoardCnt[BOARDTYPE_MCG];
    streamData.streamPktData[idx].timeStamp = timeStamp;
    streamData.streamPktData[idx].uid = uid;
    streamData.streamPktData[idx].version = SENSOR_BOARD_READING_VERSION;
    uid++;
}

__ITCMRAM__ static inline void clearStreamPktData(int idx) {
    assert(idx < MAX_STREAM_DATA_PKT_IDX);
    memset(&streamData.streamPktData[idx], 0, sizeof(streamData.streamPktData[0]));
}

void waitOnTcpServerConnectionThread(const void *arg) {

    err_t err;
    ip_addr_t addr;
    uint16_t port;

    struct netconn *newconn, *conn;
    registerInfo_t portReg = {.mbId = TCP_CLIENT_PORT, .type = DATA_UINT};
    registerInfo_t ipReg = {.mbId = IP_ADDR, .type = DATA_UINT};
    registerRead(&portReg);
    registerRead(&ipReg);

    conn = netconn_new(NETCONN_TCP);
    uint8_t ip[4];
    memcpy(ip, &ipReg.u.dataUint, 4);
    DPRINTF_INFO("TCP Client listening on %d.%d.%d.%d, %d\r\n", ip[0], ip[1], ip[2], ip[3], portReg.u.dataUint);
    if (conn == NULL) {
        DPRINTF_ERROR("Unable to make NETWORK TCP LISTENING CONNECTION\r\n");
        return;
    }

    err = netconn_bind(conn, IP4_ADDR_ANY, portReg.u.dataUint);

    if (err != ERR_OK) {
        DPRINTF_ERROR("netconn_bind failed %d\r\n", err);
        return;
    }
    /* create a TCP socket */

    netconn_listen(conn);

    while (1) {
        if ((err = netconn_accept(conn, &newconn)) != ERR_OK) {
            DPRINTF_ERROR("netconn accept error %d\r\n", err);
            continue;
        }

        netconn_addr(newconn, &addr, &port);
        if (tcpConn == NULL) {
            tcpConn = newconn;
            DPRINTF_GATH("Accepting connection from %s:%d\r\n", ipaddr_ntoa(&addr), port);
        } else {
            DPRINTF_GATH("Ignoring connection from %s:%d\r\n", ipaddr_ntoa(&addr), port);
        }
    }
}

__ITCMRAM__ void sendData(void *p_data, size_t dataLen) {
    if (useUdpChan) {
        sendUdpData(p_data, dataLen);
    } else {
        if (tcpConn) {
            sendTcpData(p_data, dataLen);
        }
    }
}

__ITCMRAM__ void closeTcpConnection(void) {
    netconn_close(tcpConn);
    tcpConn = NULL;
    sendErrCnt = 0;
}

__ITCMRAM__ void sendTcpData(void *p_data, size_t dataLen) {

    err_t err;

    if (tcpConn == NULL) {
        return;
    }

    if ((err = netconn_err(tcpConn)) != ERR_OK) {
        DPRINTF_INFO("Closing TCP connection due to error %d\r\n", err);
        closeTcpConnection();
        return;
    }

    // MEASURE TCP Tx Execution Time with GPIO Pin
    HAL_GPIO_WritePin(DBG2_PORT, DBG2_PIN, 1);

    if ((err = netconn_write(tcpConn, p_data, dataLen, NETCONN_COPY)) != ERR_OK) {
        DPRINTF_ERROR("netconn_send failed %d\r\n", err);
        if (sendErrCnt++ > MAX_SEND_ERRORS) {
            DPRINTF_INFO("Closing TCP connection due to SEND errors %d\r\n", err);
            closeTcpConnection();
            return;
        }
    } else {
        sendErrCnt = 0;
    }

    // MEASURE TCP TX Execution Time with GPIO Pin
    HAL_GPIO_WritePin(DBG2_PORT, DBG2_PIN, 0);
}

__ITCMRAM__ void sendUdpData(void *p_data, size_t dataLen) {

    static ip_addr_t destAddr;
    static uint32_t clientPort = 5005;
    static struct netconn *conn = NULL;
    static uint32_t cnt = 0;
    if (conn == NULL) {
        err_t err;
        conn = netconn_new(NETCONN_UDP);
        if (conn == NULL) {
            DPRINTF_ERROR("ERROR opening socket");
            assert(false);
        }

        uint32_t udpTxPort;
        eepromOpen(osWaitForever);
        eepromReadRegister(EEPROM_UDP_TX_PORT, (uint8_t *)&udpTxPort, sizeof(udpTxPort));
        eepromReadRegister(EEPROM_SERVER_UDP_IP, (uint8_t *)&destAddr, sizeof(destAddr));
        eepromReadRegister(EEPROM_SERVER_UPD_PORT, (uint8_t *)&clientPort, sizeof(clientPort));
        eepromClose();
        DPRINTF_INFO("UDP TX is sending from port %u\r\n", udpTxPort);
        err = netconn_bind(conn, IP_ADDR_ANY, udpTxPort);
        if (err != ERR_OK) {
            DPRINTF_ERROR("netconn bind error %d\r\n", err);
            assert(false);
        }

        uint8_t *pBuffer = (uint8_t *)&destAddr;
        DPRINTF_INFO(
            "Using udp server %u,%u,%u,%u port %u\r\n", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], clientPort);
    }

    struct netbuf *buf = netbuf_new();
    netbuf_ref(buf, p_data, dataLen);
    HAL_GPIO_WritePin(DBG2_PORT, DBG2_PIN, 1);
    uint32_t sendErr = netconn_sendto(conn, buf, &destAddr, clientPort);
    if (sendErr != ERR_OK) {
        if (cnt < 10 || cnt % 60000 == 0) {
            DPRINTF_ERROR("netconn_send failed %d\r\n", sendErr);
        }
        cnt++;
    }
    HAL_GPIO_WritePin(DBG2_PORT, DBG2_PIN, 0);
    netbuf_delete(buf);
}
static double timeStamp;
static uint32_t sendingIdx;
static uint32_t lastTick;

__ITCMRAM__ void mbGatherThread(const void *arg) {
    uint32_t notify;
    DPRINTF_GATH("Gather Task starting\r\n");

    uint32_t dbMask = createPktStructure();
    registerInfo_t regInfo = {0};
    uint32_t cnt = 0;
    // as long as we have all of dbMask up, there can be others up too that
    // we don't care about.
    while ((dbMask & regInfo.u.dataUint) != dbMask) {
        // Give time for sensor boards to come up and become responsive. ECG12 takes the longest time to boot
        osDelay(WAIT_FOR_SENSOR_BOARDS_TO_SETTLE_MS(2000));
        dbTriggerEnableAll();
        // Give time for sensor boards to fail
        // Multiply minimum fail time by 2
        osDelay(WAIT_FOR_SENSOR_BOARDS_TO_SETTLE_MS(DB_MAX_UNANSWERED_RESPONSE_DISABLE * DB_SPI_INTERVAL_MS * 2));
        dbTriggerMaskRead(&regInfo);
        if (cnt++ > DB_UP_RETRY_MAX) {
            break;
        }
    }
    DPRINTF_INFO("Sensor Board Up retry cnt = %d\r\n", cnt);
    if (!verifySensorConfiguration()) {
        configurationErrorRaise(NULL);
        dbTriggerMaskRead(&regInfo);
        DPRINTF_ERROR("Boards missing %x\r\n", regInfo.u.dataUint);
    }

    watchdogAssignToCurrentTask(WDT_TASK_GATHER);
    watchdogSetTaskEnabled(WDT_TASK_GATHER, 1);

    HAL_TIM_Base_Start_IT(pwmMap[TIM_UDP_TX_SIGNAL].htim);

    lastTick = HAL_GetTick();
    timeStamp = timeSinceEpoch();
    while (1) {
        // A new conversion has started. So lock down the current data and
        // send it!
        notify = ulTaskNotifyTake(true, MB_GATHER_TASK_TIMEOUT_MS);
        watchdogKickFromTask(WDT_TASK_GATHER);
        if (notify == TASK_NOTIFY_OK) {
            if ((HAL_GetTick() - lastTick > 1000)) { // approx 1/sec
                timeStamp = timeSinceEpoch();        // resync time stamp
                lastTick = HAL_GetTick();
            } else {
                timeStamp += TS_DELTA;
            }
            osStatus osResult = osMutexWait(streamData.access, 2);
            if (osResult == osOK) {
                sendingIdx = streamData.streamDataIdx;
                streamData.streamDataIdx++;
                streamData.streamDataIdx %= MAX_STREAM_DATA_PKT_IDX;
                clearStreamPktData(streamData.streamDataIdx);
                // Moving the IDX locks down this data so we can release the semaphore now.
                osMutexRelease(streamData.access);

                setStreamPktHeader(sendingIdx, timeStamp);

                sendData((void *)&streamData.streamPktData[sendingIdx], sizeof(streamData.streamPktData[sendingIdx]));

                gatherStats.sentPkts++;
                for (int i = 0; i < MAX_CS_ID; i++) {
                    // we cannot count missed data as we miss data 4 out of 5 transmissions for IMU data.
                    if (sensorBoardDataLocation[i].configBoardType == BOARDTYPE_MCG) {
                        sensorMCGBoardReadings_tp p_header =
                            sensorBoardDataLocation[i].dataLocation[streamData.streamDataIdx][0].p_MCGsensors;
                        if (NEW_DATA(p_header->flags)) {
                            gatherStats.db[i].sentPkts[SENSOR_0]++;
                        } else {
                            gatherStats.db[i].missedData++;
                        }
                    } else if (sensorBoardDataLocation[i].configBoardType == BOARDTYPE_ECG || sensorBoardDataLocation[i].configBoardType == BOARDTYPE_12ECG) {
                        sensorECGBoardReadings_tp p_header =
                            sensorBoardDataLocation[i].dataLocation[streamData.streamDataIdx][0].p_ECGsensors;
                        if (NEW_DATA(p_header->flags)) {
                            gatherStats.db[i].sentPkts[SENSOR_0]++;
                        } else {
                            gatherStats.db[i].missedData++;
                        }
                    } else if(sensorBoardDataLocation[i].configBoardType == BOARDTYPE_IMU_COIL) {
                        for (int sensorIdx = 0; sensorIdx < SENSORS_PER_BOARD; sensorIdx++) {

                            imuReadings_tp p_imuHeader =
                                sensorBoardDataLocation[i].dataLocation[streamData.streamDataIdx][sensorIdx].p_imu;
                            if (NEW_DATA(p_imuHeader->flags)) {
                                gatherStats.db[i].sentPkts[sensorIdx]++;
                            }
                        }
                    } // end of else if IMU_COIL
                } // end of for i<MAX_CS_ID
            } else {
                timeStamp = timeSinceEpoch();
                gatherStats.accessBlockCnt++;
            }

        } else {
            timeStamp = timeSinceEpoch();
            gatherStats.notifyTimeoutCnt++;
            lastTick = HAL_GetTick();
        }
    }
}

bool coilDriverBoard(uint32_t dbId) {
    assert(dbId < MAX_CS_ID);
    return (sensorBoardDataLocation[dbId].configBoardType == BOARDTYPE_IMU_COIL);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
void printStreamData(CLI *hCli, int dbId) {


    char printBuffer[MAX_PRINT_BUFFER_SZ];
    switch (sensorBoardDataLocation[dbId].configBoardType) {
    case BOARDTYPE_MCG: {


        sensorMCGBoardReadings_tp p_readings = sensorBoardDataLocation[dbId].dataLocation[streamData.streamDataIdx][SENSOR_0].p_MCGsensors;
        for (int i = SENSOR_0; i <= SENSOR_1; i++) {
            CliPrintf(
                hCli, "\t%s.C reading      = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->readings[(i * NUMBER_OF_ADC_READINGS_PER_SENSOR) + MCG_READING_COIL_C]);
            CliPrintf(
                hCli, "\t%s.B reading      = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->readings[(i * NUMBER_OF_ADC_READINGS_PER_SENSOR) + MCG_READING_COIL_B]);
            CliPrintf(
                hCli, "\t%s.A reading      = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->readings[(i * NUMBER_OF_ADC_READINGS_PER_SENSOR) + MCG_READING_COIL_A]);
            CliPrintf(
                hCli, "\t%s.TEMP reading   = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->readings[(i * NUMBER_OF_ADC_READINGS_PER_SENSOR) + MCG_READING_TEMPERATURE]);

        }
        CliPrintf (hCli, "Coil Setting data for sensors\r\n");
        for (int i = SENSOR_0; i <= SENSOR_1; i++) {
            CliPrintf(
                hCli, "\t%s.DAC_Compensation  = %lu mV\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->ctrlData[i].dacCompensationAmplitude);
            CliPrintf(
                hCli, "\t%s.DDS_Excitation    = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->ctrlData[i].ddsExcitationAmplitude);
            CliPrintf(
                hCli, "\t%s.TrimCompensationA = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->ctrlData[i].trimCompensationA);
            CliPrintf(
                hCli, "\t%s.trimCompensationC = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->ctrlData[i].trimCompensationC);

        }
    }
        break;
    case BOARDTYPE_ECG: {
        sensorECGBoardReadings_tp p_readings = sensorBoardDataLocation[dbId].dataLocation[streamData.streamDataIdx][SENSOR_0].p_ECGsensors;
        CliPrintf(hCli, "\tAUX.0A reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX0A]);
        CliPrintf(hCli, "\tAUX.0B reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX0B]);
        CliPrintf(hCli, "\tAUX.0C reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX0C]);
        CliPrintf(hCli, "\tAUX.1A reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX1A]);
        CliPrintf(hCli, "\tAUX.1B reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX1B]);
        CliPrintf(hCli, "\tAUX.1C reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX1C]);
        CliPrintf(hCli, "\tECG reading        = %lu\r\n", p_readings->readings[ECG_READING_ECG]);
        CliPrintf(hCli, "\tAWG reading        = %lu\r\n", p_readings->readings[ECG_READING_AWG]);
    }
        break;
    case BOARDTYPE_12ECG:
    {
        sensorECGBoardReadings_tp p_readings = sensorBoardDataLocation[dbId].dataLocation[streamData.streamDataIdx][SENSOR_0].p_ECGsensors;
        CliPrintf(hCli, "\tECG V6 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V6]);
        CliPrintf(hCli, "\tECG LA-RA reading  = %lu\r\n", p_readings->readings[ECG12_READING_LA]);
        CliPrintf(hCli, "\tECG LL-RA reading  = %lu\r\n", p_readings->readings[ECG12_READING_LL]);
        CliPrintf(hCli, "\tECG V2 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V2]);
        CliPrintf(hCli, "\tECG V3 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V3]);
        CliPrintf(hCli, "\tECG V4 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V4]);
        CliPrintf(hCli, "\tECG V5 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V5]);
        CliPrintf(hCli, "\tECG V1 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V1]);
    }
        break;
    case BOARDTYPE_IMU_COIL:
        imuReadings_tp p_imu;
        for (int i = SENSOR_0; i <= SENSOR_1; i++) {
            p_imu = &g_imuData[i];
            if (p_imu->euler.blank == 0) {
                float froll, fpitch, fheading;
                u32BE_to_floatLE(froll, p_imu->euler.data.alphaRoll);
                u32BE_to_floatLE(fpitch, p_imu->euler.data.alphaPitch);
                u32BE_to_floatLE(fheading, p_imu->euler.data.alphaHeading);
                snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", froll);
                CliPrintf(hCli,
                          "\t%s.alpha.roll     = %s (%x)\r\n",
                          i == SENSOR_0 ? "sensor0" : "sensor1",
                          printBuffer,
                          p_imu->euler.data.alphaRoll);
                snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", fpitch);
                CliPrintf(hCli,
                          "\t%s.alpha.pitch    = %s (%x)\r\n",
                          i == SENSOR_0 ? "sensor0" : "sensor1",
                          printBuffer,
                          p_imu->euler.data.alphaPitch);
                snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", fheading);
                CliPrintf(hCli,
                          "\t%s.alpha.heading  = %s (%x)\r\n",
                          i == 0 ? "sensor0" : "sensor1",
                          printBuffer,
                          p_imu->euler.data.alphaHeading);

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->euler.data.accel[coorIdx]);
                    snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", fread);
                    CliPrintf(hCli,
                              "\t%s.accel.%c       = %s (%x)\r\n",
                              i == SENSOR_0 ? "sensor0" : "sensor1",
                              coorIdx + 'x',
                              printBuffer,
                              p_imu->euler.data.accel[coorIdx]);
                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->euler.data.gyro[coorIdx]);
                    snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", fread);
                    CliPrintf(hCli,
                              "\t%s.gyro.%c        = %s (%x)\r\n",
                              i == SENSOR_0 ? "sensor0" : "sensor1",
                              coorIdx + 'x',
                              printBuffer,
                              p_imu->euler.data.gyro[coorIdx]);
                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    uint16_t mag = htob16(p_imu->euler.data.magnet[coorIdx]);
                    snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", MAGNET_VALUE(mag));
                    CliPrintf(hCli,
                              "\t%s.magnet.%c      = %s (%x)\r\n",
                              i == SENSOR_0 ? "sensor0" : "sensor1",
                              coorIdx + 'x',
                              printBuffer,
                              p_imu->euler.data.magnet[coorIdx]);
                }
                uint16_t temp = htob16(p_imu->euler.data.temperature);
                snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", TEMPERATURE_VALUE(temp));
                CliPrintf(hCli,
                          "\t%s.temperature  = %s (%x)\r\n",
                          i == SENSOR_0 ? "sensor0" : "sensor1",
                          printBuffer,
                          p_imu->euler.data.temperature);

            } else {

                for (int coorIdx = 0; coorIdx < QUATERNION_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->quat.quat[coorIdx]);
                    snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", fread);
                    CliPrintf(hCli,
                              "\t%s.quat.%c        = %s\r\n",
                              i == SENSOR_0 ? "sensor0" : "sensor1",
                              coorIdx + '0',
                              printBuffer);
                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->quat.accel[coorIdx]);
                    snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", fread);
                    CliPrintf(hCli,
                              "\t%s.accel.%c       = %s\r\n",
                              i == SENSOR_0 ? "sensor0" : "sensor1",
                              coorIdx + 'x',
                              printBuffer);
                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->quat.gyro[coorIdx]);
                    snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", fread);
                    CliPrintf(hCli,
                              "\t%s.gyro.%c        = %s\r\n",
                              i == SENSOR_0 ? "sensor0" : "sensor1",
                              coorIdx + 'x',
                              printBuffer);
                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    uint16_t mag = htob16(p_imu->quat.magnet[coorIdx]);
                    snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", MAGNET_VALUE(mag));
                    CliPrintf(hCli,
                              "\t%s.magnet.%c      = %s\r\n",
                              i == SENSOR_0 ? "sensor0" : "sensor1",
                              coorIdx + 'x',
                              printBuffer);
                }
                uint16_t temp = htob16(p_imu->quat.temperature);
                snprintf(printBuffer, MAX_PRINT_BUFFER_SZ, "%.02f", TEMPERATURE_VALUE(temp));
                CliPrintf(hCli, "\t%s.temperature    = %s\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", printBuffer);
            }
        }
        break;
    default:
        CliPrintf(hCli, "Board is not configured in slot mapping\r\n");
        break;
    }
}

uint32_t printStreamDataToBuffer(char *buf, uint32_t bufSz, uint32_t dbId) {
    char *nxt = buf;
    int tmp;
    switch (sensorBoardDataLocation[dbId].configBoardType) {
    case BOARDTYPE_MCG: {
        sensorMCGBoardReadings_tp p_readings = sensorBoardDataLocation[dbId].dataLocation[streamData.streamDataIdx][SENSOR_0].p_MCGsensors;
        for (int i = SENSOR_0; i <= SENSOR_1; i++) {
            tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\t%s.C reading     = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->readings[(i * NUMBER_OF_ADC_READINGS_PER_SENSOR) + MCG_READING_COIL_C].value32bit);
            SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

            tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\t%s.B reading     = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->readings[(i * NUMBER_OF_ADC_READINGS_PER_SENSOR) +MCG_READING_COIL_B].value32bit);
            SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

            tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\t%s.A reading     = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->readings[(i * NUMBER_OF_ADC_READINGS_PER_SENSOR) +MCG_READING_COIL_A].value32bit);
            SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

            tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\t%s.TEMP reading  = %lu\r\n", i == SENSOR_0 ? "sensor0" : "sensor1", p_readings->readings[(i * NUMBER_OF_ADC_READINGS_PER_SENSOR) +MCG_READING_TEMPERATURE].value32bit);
            SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););
        }
    }
    break;
    case BOARDTYPE_ECG: {
        sensorECGBoardReadings_tp p_readings = sensorBoardDataLocation[dbId].dataLocation[streamData.streamDataIdx][SENSOR_0].p_ECGsensors;

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tAUX.0A reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX0A].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tAUX.0B reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX0B].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tAUX.0C reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX0C].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tAUX.1A reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX1A].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tAUX.1B reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX1B].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tAUX.1C reading     = %lu\r\n", p_readings->readings[ECG_READING_AUX1C].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG reading        = %lu\r\n", p_readings->readings[ECG_READING_ECG].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tAWG reading        = %lu\r\n", p_readings->readings[ECG_READING_AWG].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););
    }
    break;
    case BOARDTYPE_12ECG: {
        sensorECGBoardReadings_tp p_readings = sensorBoardDataLocation[dbId].dataLocation[streamData.streamDataIdx][SENSOR_0].p_ECGsensors;
        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG V6 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V6].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG LA reading     = %lu\r\n", p_readings->readings[ECG12_READING_LA].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG LL reading     = %lu\r\n", p_readings->readings[ECG12_READING_LL].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG V2 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V2].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG V3 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V3].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG V4 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V4].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG V5 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V5].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "\tECG V1 reading     = %lu\r\n", p_readings->readings[ECG12_READING_V1].value32bit);
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););
    }
    break;
    case BOARDTYPE_IMU_COIL:
        imuReadings_tp p_imu;

        for (int i = SENSOR_0; i <= SENSOR_1; i++) {
            p_imu = &g_imuData[i];
            if (p_imu->euler.blank == 0) {
                float froll, fpitch, fheading;
                u32BE_to_floatLE(froll, p_imu->euler.data.alphaRoll);
                u32BE_to_floatLE(fpitch, p_imu->euler.data.alphaPitch);
                u32BE_to_floatLE(fheading, p_imu->euler.data.alphaHeading);

                tmp = snprintf((char *)nxt,
                                bufSz - (nxt - buf),
                                "\t%s.alpha.roll     = %.02f (%lx)\r\n",
                                i == SENSOR_0 ? "sensor0" : "sensor1",
                                froll,
                                p_imu->euler.data.alphaRoll);
                SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                tmp = snprintf((char *)nxt,
                                bufSz - (nxt - buf),
                                "\t%s.alpha.pitch    = %.02f (%lx)\r\n",
                                i == SENSOR_0 ? "sensor0" : "sensor1",
                                fpitch,
                                p_imu->euler.data.alphaPitch);
                SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                tmp = snprintf((char *)nxt,
                                bufSz - (nxt - buf),
                                "\t%s.alpha.heading  = %.02f (%lx)\r\n",
                                i == 0 ? "sensor0" : "sensor1",
                                fheading,
                                p_imu->euler.data.alphaHeading);

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->euler.data.accel[coorIdx]);

                    tmp = snprintf((char *)nxt,
                                    bufSz - (nxt - buf),
                                    "\t%s.accel.%c       = %.02f (%lx)\r\n",
                                    i == SENSOR_0 ? "sensor0" : "sensor1",
                                    coorIdx + 'x',
                                    fread,
                                    p_imu->euler.data.accel[coorIdx]);
                    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->euler.data.gyro[coorIdx]);

                    tmp = snprintf((char *)nxt,
                                    bufSz - (nxt - buf),
                                    "\t%s.gyro.%c        = %.02f (%lx)\r\n",
                                    i == SENSOR_0 ? "sensor0" : "sensor1",
                                    coorIdx + 'x',
                                    fread,
                                    p_imu->euler.data.gyro[coorIdx]);
                    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    uint16_t mag = htob16(p_imu->euler.data.magnet[coorIdx]);

                    tmp = snprintf((char *)nxt,
                                    bufSz - (nxt - buf),
                                    "\t%s.magnet.%c      = %.02f (%x)\r\n",
                                    i == SENSOR_0 ? "sensor0" : "sensor1",
                                    coorIdx + 'x',
                                    MAGNET_VALUE(mag),
                                    p_imu->euler.data.magnet[coorIdx]);
                    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                }
                uint16_t temp = htob16(p_imu->euler.data.temperature);

                tmp = snprintf((char *)nxt,
                                bufSz - (nxt - buf),
                                "\t%s.temperature  = %.02f (%x)\r\n",
                                i == SENSOR_0 ? "sensor0" : "sensor1",
                                TEMPERATURE_VALUE(temp),
                                p_imu->euler.data.temperature);
                SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););


            } else {

                for (int coorIdx = 0; coorIdx < QUATERNION_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->quat.quat[coorIdx]);

                    tmp = snprintf((char *)nxt,
                                    bufSz - (nxt - buf),
                                    "\t%s.quat.%c        = %.02f\r\n",
                                    i == SENSOR_0 ? "sensor0" : "sensor1",
                                    coorIdx + '0',
                                    fread);
                    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->quat.accel[coorIdx]);

                    tmp = snprintf((char *)nxt,
                                    bufSz - (nxt - buf),
                                    "\t%s.accel.%c       = %.02f\r\n",
                                    i == SENSOR_0 ? "sensor0" : "sensor1",
                                    coorIdx + 'x',
                                    fread);
                    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    float fread = 0.0;
                    u32BE_to_floatLE(fread, p_imu->quat.gyro[coorIdx]);

                    tmp = snprintf((char *)nxt,
                                    bufSz - (nxt - buf),
                                    "\t%s.gyro.%c        = %.02f\r\n",
                                    i == SENSOR_0 ? "sensor0" : "sensor1",
                                    coorIdx + 'x',
                                    fread);
                    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                }

                for (int coorIdx = 0; coorIdx < CARTESSIAN_COOR; coorIdx++) {
                    uint16_t mag = htob16(p_imu->quat.magnet[coorIdx]);

                    tmp = snprintf((char *)nxt,
                                    bufSz - (nxt - buf),
                                    "\t%s.magnet.%c      = %.02f\r\n",
                                    i == SENSOR_0 ? "sensor0" : "sensor1",
                                    coorIdx + 'x',
                                    MAGNET_VALUE(mag));
                    SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

                }
                uint16_t temp = htob16(p_imu->quat.temperature);
                tmp = snprintf((char *)nxt,
                                bufSz - (nxt - buf),
                                "\t%s.temperature    = %.02f\r\n",
                                i == SENSOR_0 ? "sensor0" : "sensor1",
                                TEMPERATURE_VALUE(temp));
                SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

            }
        }
        break;
    default:
        tmp = snprintf((char *)nxt, bufSz - (nxt - buf), "Board is not configured in slot mapping\r\n");
        SNPRINTF_TEST_AND_ADD(tmp,nxt, return (nxt - buf););

        break;
    }
    return (nxt - buf);
}

#pragma GCC diagnostic pop
/*
 * Debug function. It will fill the next ethernet stream of data with known values so that
 * the structure can be easily parsed and verified.
 *
 */
void testSensorFill(bool quatImuType) {
    dbCommThreadInfo_t threadInfo;
    static uint32_t fill = 0x00;
    uint32_t readings[SENSORS_PER_BOARD][MAX_ADC_READING];
    imuReadings_t imuReadings;
    for (int sbId = 0; sbId < MAX_CS_ID; sbId++) {
        threadInfo.daughterBoardId = sbId;
        switch (sensorBoardDataLocation[sbId].configBoardType) {
        case BOARDTYPE_MCG:
            // fall through
        case BOARDTYPE_ECG:
            // fall through
        case BOARDTYPE_12ECG:
            for (int sensor = SENSOR_0; sensor <= SENSOR_1; sensor++) {
                readings[sensor][0] = sbId;
                for (int read = 1; read < MAX_ADC_READING; read++) {
                    readings[sensor][read] = fill++;
                }
            }
            updateSensorData(&threadInfo, (uint8_t *)&readings[0][0], sizeof(readings));
            break;
        case BOARDTYPE_IMU_COIL:
            for (int sensor = SENSOR_0; sensor <= SENSOR_1; sensor++) {
                if (quatImuType) {
                    for (int axis = QUAT_0; axis < QUAT_MAX; axis++) {
                        imuReadings.quat.quat[axis] = fill++;
                    }
                    for (int axis = AXIS_X; axis < AXIS_MAX; axis++) {
                        imuReadings.quat.accel[axis] = fill++;
                    }
                    for (int axis = AXIS_X; axis < AXIS_MAX; axis++) {
                        imuReadings.quat.gyro[axis] = fill++;
                    }
                    for (int axis = AXIS_X; axis < AXIS_MAX; axis++) {
                        imuReadings.quat.magnet[axis] = fill++;
                    }
                    imuReadings.quat.temperature = sbId;
                    updateSensorData(&threadInfo, &imuReadings.u8[0], sizeof(quaternionData_t));
                } else {

                    imuReadings.euler.blank = 0;
                    imuReadings.euler.data.alphaRoll = fill++;
                    imuReadings.euler.data.alphaPitch = fill++;
                    imuReadings.euler.data.alphaHeading = fill++;

                    for (int axis = AXIS_X; axis < AXIS_MAX; axis++) {
                        imuReadings.euler.data.accel[axis] = fill++;
                    }
                    for (int axis = AXIS_X; axis < AXIS_MAX; axis++) {
                        imuReadings.euler.data.gyro[axis] = fill++;
                    }
                    for (int axis = AXIS_X; axis < AXIS_MAX; axis++) {
                        imuReadings.euler.data.magnet[axis] = fill++;
                    }
                    imuReadings.euler.data.temperature = sbId;
                    updateSensorData(&threadInfo, &imuReadings.u8[0], sizeof(euler_t));
                }
            }
            break;
        default:

            break;
        }
    }
}

void correctTargetToEmpty(uint32_t target) {
    correctTargetToNewValue(target, BOARDTYPE_EMPTY);
}

void correctTargetToNewValue(uint32_t target, uint32_t newValue) {
    uint32_t boardType;
    uint32_t boardTypeEmpty = newValue;
    // search each register and any that match target set to 7 (empty)
    eepromOpen(osWaitForever);
    for (int i = EEPROM_SENSOR_BOARD_0; i <= EEPROM_SENSOR_BOARD_23; i++) {
        eepromReadRegister(i, (uint8_t *)&boardType, sizeof(boardType));
        if (boardType == target) {
            eepromWriteRegister(i, (uint8_t *)&boardTypeEmpty, sizeof(boardTypeEmpty));
        }
    }
    eepromClose();
}
