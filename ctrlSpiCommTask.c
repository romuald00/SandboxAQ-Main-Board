
/*
 * ctrlCommTask.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 31, 2023
 *      Author: rlegault
 */

// will generate task for each spi bus.

#define __DBCOMM_CREATE__
#include "ctrlSpiCommTask.h"
#include "cli/cli_print.h"
#include "cli_task.h"
#include "cmdAndCtrl.h"
#include "cmsis_os.h"
#include "dbCommTask.h"
#include "debugPrint.h"
#include "gpioMB.h"
#include "largeBuffer.h"
#include "perseioTrace.h"
#include "saqTarget.h"
#include "stmTarget.h"
#include "taskWatchdog.h"

#include <stdlib.h>
#include <string.h>

static uint32_t mappingBoard = MAX_CS_ID;

typedef struct os_pool_cb {
    void *pool;
    uint8_t *markers;
    uint32_t pool_sz;
    uint32_t item_sz;
    uint32_t currentIndex;
} os_pool_cb_t;

#undef __DBCOMM_CREATE__

// Initializes the spiComm task.
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hspi4;

// extern SPI_HandleTypeDef hspi5; Unused
extern CRC_HandleTypeDef hcrc;

#define NOT_A_STUCK_MISO_DATA 0xFE


/* Note that the required delays are command specific for debug/db/stats
 * this is the time required for the daughter board to fill the medium size buffer
 * before starting the transmission.
 * watchdog 2ms
 * task     7ms
 * stack    6ms
 * heap     2ms
 * system   2ms
 * reglist  3ms
 *
 * ramLog   2ms
 * db/getlog only requires 2ms because it is not writing to the buffer it is just
 * reading from the log ram.
 */
#define SB_DATA_PROCESSING_DELAY_MS 10

#define SB_BUFFER_CHANGE_DELAY_MS 1

#define SPI_DBMB_PKT_SIZE sizeof(spiDbMbPacket_t)

#define MAX_CS_ID (MAX_CS_PER_SPI * MAX_SPI)

// Allow
#define SPI_MSG_DEPTH 2 * (MAX_CS_PER_SPI + 3)

// Tried 25% (5/4) above MAX_CS_ID but that was insufficient
// 30 was not enough, at 60 we saw no errors.
#define SPI_MSG_POOL_DEPTH (MAX_CS_ID * 10 / 4)

// CLI command argv access indexes
#define CMD_ARG_IDX 1
#define CMD_PARAM_IDX(x) (x + CMD_ARG_IDX + 1)
#define CMD_PARAM_CNT(x) (CMD_PARAM_IDX(x) + 1)

// USING DBG_6 F4
#define RAISE_CS(id)                                                                                                   \
    HAL_GPIO_WritePin(csId[id].port, csId[id].pin, 1);                                                                 \
    if (id == mappingBoard) {                                                                                          \
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_4, 1);                                                                       \
    } // DBG_6
#define LOWER_CS(id)                                                                                                   \
    HAL_GPIO_WritePin(csId[id].port, csId[id].pin, 0);                                                                 \
    if (id == mappingBoard) {                                                                                          \
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_4, 0);                                                                       \
    } // DBG_6
#define SPI_TXRX_NOTIFY_TIMEOUT_MS 2

/* 8=bits per character,
 * 1000 translate to msec
 * 12500000 spi bus speed
 * 2 buffer
 */
#define SPI_RX_LARGEBUFFER_TIMEOUT_MS ((LARGE_BUFFER_SIZE_BYTES * 8 * 1000 / 12500000) + 2)

__DTCMRAM__ StaticQueue_t spiQMsgCtrl[MAX_SPI];
__DTCMRAM__ uint8_t spiMsgQ[MAX_SPI][SPI_MSG_DEPTH * sizeof(cncInfo_tp)];

#define spiMessageQCreateStatic(ID, v, threadId)                                                                       \
    {                                                                                                                  \
        osMessageQStaticDef(spi##ID##MsgQ, SPI_MSG_DEPTH, cncInfo_tp, spiMsgQ[ID], &spiQMsgCtrl[ID]);                  \
        v = osMessageCreate(osMessageQ(spi##ID##MsgQ), threadId);                                                      \
        assert(v != NULL);                                                                                             \
    }

#define spiMessageQCreate(ID, v)                                                                                       \
    {                                                                                                                  \
        osMessageQDef(spi##ID##MsgQ, SPI_MSG_DEPTH, cncInfo_tp);                                                       \
        v = osMessageCreate(osMessageQ(spi##ID##MsgQ), NULL);                                                          \
        assert(v != NULL);                                                                                             \
    }

typedef struct {
    GPIO_TypeDef *port;
    int pin;
} spiCs_t, *spiCs_tp;

static uint32_t noSpiMsgPoolMemoryCnt = 0;
typedef struct {
    uint32_t crcError;
    uint32_t txPktCnt;
    uint32_t rxPktCnt;
    uint32_t qFullCnt;
    uint32_t enableCnt;
    uint32_t lastTxPktCnt;
    uint32_t txPktRatePerSec;
    uint32_t msgPending;
} spiStats_t, *spiStats_tp;

typedef struct {
    spiStats_t spiStats;
    uint16_t nxtTxId;
    uint32_t sendCrcErrors;
    uint32_t dest;
    bool ready;
} spiStateInfo_t, *spiStateInfo_tp;

typedef struct {
    SPI_HandleTypeDef *hspi;
    uint32_t spiBusId;
    osMessageQId msgQId;
    osThreadId threadId;
    WatchdogTask_e watchDogId;
    int csStartIdx;
    spiDbMbPacket_tp p_txBuffer;
    spiDbMbPacket_tp p_rxBuffer;
    spiStateInfo_t state;
} ctrlCommThreadInfo_t, *ctrlCommThreadInfo_tp;

// Pool is shared among all 4 spi busses
osPoolId spiMsgPool = NULL;

__attribute__((section(".spiRxDmaSection"))) static uint8_t RX_DATA[MAX_SPI][sizeof(spiDbMbPacket_t)];
__attribute__((section(".spiTxDmaSection"))) static uint8_t TX_DATA[MAX_SPI][sizeof(spiDbMbPacket_t)];

#define csMap(PORT, PIN)                                                                                               \
    { GPIO##PORT, GPIO_PIN_##PIN }
static const spiCs_t csId[MAX_CS_ID] = {
    /* following defines create rows of
     * csMap(C,9), -> {GPIOC,GPIO_PIN_9},
     */
    /* SPI 1 */
    csMap(B, 0),  /* CS0 */
    csMap(B, 1),  /* CS1 */
    csMap(F, 11), /* CS2 */
    csMap(F, 12), /* CS3 */
    csMap(F, 13), /* CS4 */
    csMap(F, 14), /* CS5 */
    csMap(F, 15), /* CS6 */
    csMap(G, 0),  /* CS7 */
    /* SPI 2 */
    csMap(G, 1),  /* CS8 */
    csMap(E, 7),  /* CS9 */
    csMap(E, 8),  /* CS10 */
    csMap(E, 9),  /* CS11 */
    csMap(E, 10), /* CS12 */
    csMap(E, 11), /* CS13 */
    csMap(E, 12), /* CS14 */
    csMap(E, 13), /* CS15 */
    /* SPI 3 */
    csMap(E, 14), /* CS16 */
    csMap(E, 15), /* CS17 */
    csMap(D, 14), /* CS18 */
    csMap(D, 15), /* CS19 */
    csMap(G, 2),  /* CS20 */
    csMap(G, 3),  /* CS21 */
    csMap(G, 4),  /* CS22 */
    csMap(G, 5),  /* CS23 */

};
char csBuffer[10];
char *csName(uint32_t csIdx) {

    if (csIdx >= MAX_CS_ID)
        return NULL;

    uint32_t pin = 0;
    uint32_t p = csId[csIdx].pin;
    do {
        pin++;
        p = p >> 1;
    } while (p != 0);
    pin--;
    if (csId[csIdx].port == GPIOA) {
        snprintf(csBuffer, 10, "PA%lu", pin);
    } else if (csId[csIdx].port == GPIOB) {
        snprintf(csBuffer, 10, "PB%lu", pin);
    } else if (csId[csIdx].port == GPIOC) {
        snprintf(csBuffer, 10, "PC%lu", pin);
    } else if (csId[csIdx].port == GPIOD) {
        snprintf(csBuffer, 10, "PD%lu", pin);
    } else if (csId[csIdx].port == GPIOE) {
        snprintf(csBuffer, 10, "PE%lu", pin);
    } else if (csId[csIdx].port == GPIOF) {
        snprintf(csBuffer, 10, "PF%lu", pin);
    } else if (csId[csIdx].port == GPIOG) {
        snprintf(csBuffer, 10, "PG%lu", pin);
    } else {
        return NULL;
    }

    return csBuffer;
}
#define spiCommThreadInfoCreate(SPI_COMM_THREAD_INFO, BUS, IDX)                                                        \
    SPI_COMM_THREAD_INFO.hspi = &hspi##BUS;                                                                            \
    SPI_COMM_THREAD_INFO.spiBusId = IDX;                                                                               \
    SPI_COMM_THREAD_INFO.msgQId = NULL;                                                                                \
    SPI_COMM_THREAD_INFO.threadId = NULL;                                                                              \
    SPI_COMM_THREAD_INFO.watchDogId = WDT_TASK_CTRLCOMM_SPI##BUS;                                                      \
    SPI_COMM_THREAD_INFO.csStartIdx = MAX_CS_PER_SPI * IDX;                                                            \
    SPI_COMM_THREAD_INFO.p_txBuffer = (spiDbMbPacket_tp)TX_DATA[IDX];                                                  \
    SPI_COMM_THREAD_INFO.p_rxBuffer = (spiDbMbPacket_tp)RX_DATA[IDX];

__DTCMRAM__ static ctrlCommThreadInfo_t spiCommThreadInfo[MAX_SPI];

static void ctrlCommTaskThread(void const *argument);
static HAL_StatusTypeDef handleSpiMsg(ctrlCommThreadInfo_tp p_threadInfo, cncInfo_tp p_data);
static HAL_StatusTypeDef handleRxMsg(ctrlCommThreadInfo_tp p_threadInfo, cncInfo_tp p_cncInfo, spiDbMbPacket_tp p_data);
static osStatus ctrlSpiCommCliCallback(spiDbMbCmd_e spiDbMbCmd,
                                       uint32_t cbId,
                                       uint8_t xInfo,
                                       spiDbMbPacketCmdResponse_t cmdResponse,
                                       cncPayload_tp p_cncMsg);

__DTCMRAM__ StaticTask_t spiTaskCtrlBlock[MAX_SPI];
__DTCMRAM__ StackType_t spiStack[MAX_SPI][SPI_STACK_WORDS];

#define spiCreateStaticTask(NAME, IDX, MDMA)                                                                           \
    osThreadStaticDef(NAME, ctrlCommTaskThread, priority, 0, stackSize, spiStack[IDX], &spiTaskCtrlBlock[IDX]);        \
    spiCommThreadInfo[IDX].threadId = osThreadCreate(osThread(NAME), &spiCommThreadInfo[IDX]);                         \
    assert(spiCommThreadInfo[IDX].threadId != NULL);

#define spiCreateTask(NAME, IDX, MDMA)                                                                                 \
    osThreadDef(NAME, ctrlCommTaskThread, priority, 0, stackSize);                                                     \
    spiCommThreadInfo[IDX].threadId = osThreadCreate(osThread(NAME), &spiCommThreadInfo[IDX]);                         \
    assert(spiCommThreadInfo[IDX].threadId != NULL);

#if DEBUG_ONE_BOARD_COMPILER_FLAG
#warning "SPI interaction with only one board"
#endif

extern bool printSpi;

bool SPIIsReady(uint32_t destination) {
    uint8_t spiDest = destination / MAX_CS_PER_SPI;
    assert(spiDest < MAX_SPI);
    return (spiCommThreadInfo[spiDest].state.ready);
}

void ctrlSpiCommTaskInit(int priority, int stackSize) {
    DPRINTF_CCOMM("Ctrl Comm task initializing all spi tasks\r\n");

    assert(stackSize == SPI_STACK_WORDS);

    memset(spiCommThreadInfo, 0, sizeof(spiCommThreadInfo));

    spiCommThreadInfoCreate(spiCommThreadInfo[0], 1, 0);
    spiCommThreadInfoCreate(spiCommThreadInfo[1], 2, 1);
    spiCommThreadInfoCreate(spiCommThreadInfo[2], 3, 2);

    spiMessageQCreateStatic(0, spiCommThreadInfo[0].msgQId, NULL);
    spiMessageQCreateStatic(1, spiCommThreadInfo[1].msgQId, NULL);
    spiMessageQCreateStatic(2, spiCommThreadInfo[2].msgQId, NULL);

    osPoolDef(spiMsgPool, SPI_MSG_POOL_DEPTH, cncInfo_t);
    spiMsgPool = osPoolCreate(osPool(spiMsgPool));
    assert(spiMsgPool != NULL);

    spiCreateTask(SPI1_COM, 0, hmdma_mdma_channel0_sw_0);
    spiCreateTask(SPI2_COM, 1, hmdma_mdma_channel1_sw_0);
    spiCreateTask(SPI3_COM, 2, hmdma_mdma_channel2_sw_0);

    spiCommThreadInfo[0].state.spiStats.msgPending = 0;
    spiCommThreadInfo[1].state.spiStats.msgPending = 0;
    spiCommThreadInfo[2].state.spiStats.msgPending = 0;
}

void updateSPIEnableCount(uint8_t destination, bool enable) {
    uint8_t spiDest = destination / MAX_CS_PER_SPI;
    assert(spiDest < MAX_SPI);

    if (enable) {
        spiCommThreadInfo[spiDest].state.spiStats.enableCnt++;
    } else {
        spiCommThreadInfo[spiDest].state.spiStats.enableCnt--;
    }
    DPRINTF("Destination = %d, spiDest=%d, enable =%d, cnt=%d\r\n",
            destination,
            spiDest,
            enable,
            spiCommThreadInfo[spiDest].state.spiStats.enableCnt);
}

DMA_HandleTypeDef *spiSendDma = NULL;
__ITCMRAM__ osStatus spiSendMsg(uint8_t destination,
                                spiDbMbCmd_e cmd,
                                uint8_t xInfo,
                                uint16_t cmdUid,
                                cncMsgPayload_tp payload,
                                cncCbFnPtr cbFnPtr,
                                uint32_t cbId,
                                uint32_t timeout_ms) {

    cncInfo_tp p_cncInfo = osPoolAlloc(spiMsgPool);
    if (p_cncInfo == NULL) {
        noSpiMsgPoolMemoryCnt++;
        if (noSpiMsgPoolMemoryCnt == 1 || (noSpiMsgPoolMemoryCnt % 100) == 0) {
            DPRINTF_ERROR("spiSendMsg pool is empty\r\n");
        }
        return osErrorNoMemory;
    }
    p_cncInfo->cbFnPtr = cbFnPtr;
    p_cncInfo->cbId = cbId;
    p_cncInfo->cmd = cmd;
    p_cncInfo->destination = destination;
    p_cncInfo->xInfo = xInfo;
    p_cncInfo->cmdResponse.cmdResponse = UNKNOWN_COMMAND;
    p_cncInfo->cmdResponse.cmdUid = cmdUid;
    p_cncInfo->cmdResponse.flags = 0;

    // Multiple dbProc will call this function making it difficult to use MDMA
    memcpy(&p_cncInfo->payload, payload, sizeof(cncMsgPayload_t));
    uint8_t spiDest = destination / MAX_CS_PER_SPI;
    if (spiDest > MAX_SPI) {
        return osErrorValue;
    }
    spiCommThreadInfo[spiDest].state.spiStats.msgPending++;

    xTracePrintCompactF2(urlLogTxMsg, "spiSendMsg dest=%d per=%d", destination, payload->cncMsgPayloadHeader.peripheral);
    osStatus status = osMessagePut(spiCommThreadInfo[spiDest].msgQId, (uint32_t)p_cncInfo, timeout_ms);
    if (status != osOK) {
        spiCommThreadInfo[spiDest].state.spiStats.msgPending--;
        spiCommThreadInfo[spiDest].state.spiStats.qFullCnt++;
        DPRINTF_ERROR("SPI%d msgQ is full\r\n", spiDest + 1);
    }
    return status;
}

__ITCMRAM__ void ctrlCommTaskThread(void const *argument) {
    ctrlCommThreadInfo_tp p_ctrlCommInfo = (ctrlCommThreadInfo_tp)argument;
    osEvent evt;

    DPRINTF_CCOMM("Ctrl Comm task for SPI%d starting\r\n", p_ctrlCommInfo->spiBusId);

    watchdogAssignToCurrentTask(p_ctrlCommInfo->watchDogId);
    watchdogSetTaskEnabled(p_ctrlCommInfo->watchDogId, 1);

    for (int i = 0; i < MAX_CS_PER_SPI; i++) {
        RAISE_CS(i + p_ctrlCommInfo->csStartIdx);
    }
    p_ctrlCommInfo->state.ready = true;
    while (1) {
        watchdogKickFromTask(p_ctrlCommInfo->watchDogId);
        evt = osMessageGet(p_ctrlCommInfo->msgQId, 500);
        if (evt.status == osEventMessage) {

            spiCommThreadInfo[p_ctrlCommInfo->spiBusId].state.spiStats.msgPending--;
            cncInfo_tp p_data = (cncInfo_tp)evt.value.p;
            handleSpiMsg(p_ctrlCommInfo, p_data);
#ifdef TRACEALYZER
            {
                uint32_t index = (uint32_t)p_data - (uint32_t)(spiMsgPool->pool);
                index = index / spiMsgPool->item_sz;
                xTracePrintCompactF2(spiCommTrace[p_ctrlCommInfo->spiBusId],
                                     "Rx Msg, pending %lu poolIdx=%lu",
                                     spiCommThreadInfo[p_ctrlCommInfo->spiBusId].state.spiStats.msgPending,
                                     index);
            }
#endif
            osPoolFree(spiMsgPool, p_data);
        }
    }
}
#define SPI_TRACE_BUFFER_SIZE 256

#define PRINT_BYTE_CNT 8
#if ENABLE_SPI_TRACE_BUFFER
static __DTCMRAM__ char spiTraceBuffer[SPI_TRACE_BUFFER_SIZE][MAX_SPI];
#endif

__ITCMRAM__ HAL_StatusTypeDef handleSpiMsg(ctrlCommThreadInfo_tp p_threadInfo, cncInfo_tp p_data) {
    assert(p_threadInfo != NULL);
    assert(p_data != NULL);
    HAL_StatusTypeDef halResult = HAL_OK;
    uint32_t result = 0;
    spiDbMbPacket_t rxLargeBufferInfo = {.header.cmd = SPICMD_RESP_CNC,
                                         .header.xInfo = p_data->xInfo,
                                         .header.cmdResponse.cmdUid = p_data->cmdResponse.cmdUid,
                                         .header.cmdResponse.cmdResponse = NO_ERROR,
                                         .header.cmdResponse.flags = p_data->cmdResponse.flags};

    p_threadInfo->state.dest = p_data->destination;
    if (p_threadInfo->state.dest < p_threadInfo->csStartIdx ||
        p_threadInfo->state.dest > p_threadInfo->csStartIdx + MAX_CS_PER_SPI) {
        DPRINTF_ERROR("dest %d out of range for SPI %d\r\n", p_threadInfo->state.dest, p_threadInfo->spiBusId);
        return HAL_ERROR;
    }

    spiDbMbPacket_tp p_rx = p_threadInfo->p_rxBuffer;
    spiDbMbPacket_tp p_tx = p_threadInfo->p_txBuffer;

    p_tx->header.pktId = p_threadInfo->state.nxtTxId++;
    p_tx->header.cmd = p_data->cmd;
    p_tx->header.xInfo = p_data->xInfo;
    p_tx->header.cmdResponse.cmdUid = p_data->cmdResponse.cmdUid;
    p_tx->header.cmdResponse.cmdResponse = p_data->cmdResponse.cmdResponse;

    // memcpy (Destination, src, size)
    memcpy(p_tx->spiDBMBPacket_payload, &(p_data->payload), sizeof(cncMsgPayload_t));

    if (p_threadInfo->state.sendCrcErrors) {
        p_tx->crc = p_threadInfo->state.sendCrcErrors;
        p_threadInfo->state.sendCrcErrors--;
    } else {
        p_tx->crc = HAL_CRC_Calculate(&hcrc, p_tx->u32, (SPI_DBMB_PKT_SIZE - sizeof(uint32_t)) / sizeof(uint32_t));
    }
#if ENABLE_SPI_TRACE_BUFFER || PRINT_FULL_SPI_PACKET || PRINT_SPI_CRC_ERROR
    uint32_t spiBufferIndex;

#endif

#if TRACE_ANALYZE_SPI
    spiDbMbPacket_tp p_spiPkt = p_tx;
    int cnt = SPI_TRACE_BUFFER_SIZE;
    char *nxtBuffer = spiTraceBuffer[p_threadInfo->spiBusId];
    int wrote = snprintf(nxtBuffer,
                         cnt,
                         "SPI %ld dest %ld TX %u  ",
                         p_threadInfo->spiBusId,
                         p_threadInfo->state.dest,
                         *(uint16_t *)p_tx);
    nxtBuffer += wrote;
    cnt -= wrote;
    for (spiBufferIndex = 0; spiBufferIndex < sizeof(spiDbMbPacket_t) / PRINT_BYTE_CNT; spiBufferIndex++) {
        wrote = snprintf(nxtBuffer,
                         cnt,
                         "%02x %02x %02x %02x %02x %02x %02x %02x ",
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 4],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 5],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 6],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 7]);
    }
    if ((sizeof(spiDbMbPacket_t) % PRINT_BYTE_CNT == PRINT_BYTE_CNT / 2)) {
        wrote = snprintf(nxtBuffer,
                         cnt,
                         "%02x %02x %02x %02x",
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3]);
    }
    nxtBuffer += wrote;
    cnt -= wrote;

    vTracePrint(dbCommTrace[p_threadInfo->state.dest], spiTraceBuffer[p_threadInfo->spiBusId]);
#endif

#if PRINT_FULL_SPI_PACKET
    if (p_threadInfo->hspi == &hspi1) {
        spiDbMbPacket_tp p_spiPkt = p_tx;
        DPRINTF_RAW("MB SENDING %d\r\n", p_threadInfo->state.dest);

        for (spiBufferIndex = 0; spiBufferIndex < sizeof(spiDbMbPacket_t) / PRINT_BYTE_CNT; spiBufferIndex++) {
            DPRINTF_RAW("%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 4],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 5],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 6],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 7]);
        }
        if ((sizeof(spiDbMbPacket_t) % PRINT_BYTE_CNT == PRINT_BYTE_CNT / 2)) {
            DPRINTF_RAW("%02x %02x %02x %02x\r\n",
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3]);
        }
        DPRINTF_RAW("\r\n");
    }
#endif
    LOWER_CS(p_threadInfo->state.dest);

    uint32_t timeout_ms = SPI_TXRX_NOTIFY_TIMEOUT_MS;
    dbCommThreadInfo_tp p_dbCommThread = (dbCommThreadInfo_tp)p_data->cbId;
    xTracePrintCompactF3(urlLogTxMsg, "handleSpiMsg dest=%d per=%d lgBuf=%x", p_threadInfo->state.dest, p_data->payload.cmd.cncMsgPayloadHeader.peripheral, (uint32_t)p_dbCommThread->dbCommState.largeBuffer);
    xTracePrintCompactF2(urlLogTxMsg, "p_tx=%p p_rx=%p", (uint32_t)p_tx, (uint32_t)p_rx);

    if(p_data->payload.cmd.cncMsgPayloadHeader.action>=CNC_ACTION_READ_SMALL_BUFFER) {
        const int PERIPHERAL_IDX_IN_PACKET = 8;
        xTracePrintCompactF2(
            urlLogTxMsg, "p_tx[%d]=0x%x", PERIPHERAL_IDX_IN_PACKET, p_tx->u8[PERIPHERAL_IDX_IN_PACKET]);
        halResult =
            HAL_SPI_TransmitReceive_DMA(p_threadInfo->hspi, (uint8_t *)p_tx, (uint8_t *)p_rx, SPI_DBMB_PKT_SIZE);
        if (halResult != HAL_OK) {
            DPRINTF_ERROR("%s SPI%d TXRX failure %d\r\n", __func__, p_threadInfo->spiBusId, halResult);
            goto handleSpiMsgEnd;
        }

        result = ulTaskNotifyTake(true, timeout_ms);
        if (timeout_ms == SPI_RX_LARGEBUFFER_TIMEOUT_MS) {
            p_threadInfo->state.spiStats.txPktCnt++;
        }
        p_threadInfo->state.spiStats.txPktCnt++;
        if (result != TASK_NOTIFY_OK) {
            DPRINTF_ERROR("%s SPI%d SEMA failure %d\r\n", __func__, p_threadInfo->spiBusId, result);
            halResult = HAL_ERROR;
            goto handleSpiMsgEnd;
        }
        RAISE_CS(p_threadInfo->state.dest);
        osDelay(SB_DATA_PROCESSING_DELAY_MS); // give time for Daughter card to process last message
        LOWER_CS(p_threadInfo->state.dest);

        uint8_t *p_rxLargeBuffer = p_data->payload.cmd.largeBufferAddr;
        uint32_t sz = p_data->payload.cmd.cncMsgPayloadHeader.cncActionData.largeBufferSz;

        xTracePrintCompactF3(
            urlLogTxMsg, "Lbuf SPI%d of sz=%d p_rx=%x", p_threadInfo->spiBusId, sz, (uint32_t)p_rxLargeBuffer);
        rxLargeBufferInfo.largeBufferPtr = p_rxLargeBuffer;
        p_rx = &rxLargeBufferInfo;
        // 0xFE is used as it cannot be a stuck MISO line at 0 or 1.

        memset(p_rxLargeBuffer, NOT_A_STUCK_MISO_DATA, sz);
        halResult = HAL_SPI_Receive_DMA(p_threadInfo->hspi, p_rxLargeBuffer, sz);

        p_data->cmd = SPICMD_RESP_CNC;
        p_data->xInfo = p_tx->header.xInfo;
        p_data->cmdResponse.cmdUid = p_tx->header.cmdResponse.cmdUid;
        p_data->cmdResponse.cmdResponse = INTERNAL_COMMAND_ERROR;
        p_data->cmdResponse.flags = FLAG_LARGEBUFFER;

        timeout_ms = SPI_RX_LARGEBUFFER_TIMEOUT_MS;
        p_dbCommThread->dbCommState.largeBuffer = NULL;
    } else {
        timeout_ms = SPI_TXRX_NOTIFY_TIMEOUT_MS;
        halResult =
            HAL_SPI_TransmitReceive_DMA(p_threadInfo->hspi, (uint8_t *)p_tx, (uint8_t *)p_rx, SPI_DBMB_PKT_SIZE);
    }
    if (halResult != HAL_OK) {
        DPRINTF_ERROR("%s SPI%d TXRX failure %d\r\n", __func__, p_threadInfo->spiBusId, halResult);
        goto handleSpiMsgEnd;
    }

    result = ulTaskNotifyTake(true, timeout_ms);
    if (timeout_ms == SPI_RX_LARGEBUFFER_TIMEOUT_MS) {
        p_threadInfo->state.spiStats.txPktCnt++;
    }
    p_threadInfo->state.spiStats.txPktCnt++;
    if (result != TASK_NOTIFY_OK) {
        DPRINTF_ERROR("%s SPI%d SEMA failure %d\r\n", __func__, p_threadInfo->spiBusId, result);
        halResult = HAL_ERROR;
        goto handleSpiMsgEnd;
    }
    RAISE_CS(p_threadInfo->state.dest);

    if (timeout_ms == SPI_RX_LARGEBUFFER_TIMEOUT_MS) {
        p_data->cmdResponse.cmdResponse = NO_ERROR;
        osDelay(SB_BUFFER_CHANGE_DELAY_MS); // give time for the daughter board to reset its circular buffer
    }
    return handleRxMsg(p_threadInfo, p_data, p_rx);

handleSpiMsgEnd:
    if (timeout_ms == SPI_RX_LARGEBUFFER_TIMEOUT_MS) {
        osDelay(SB_BUFFER_CHANGE_DELAY_MS); // give time for the daughter board to reset its circular buffer
    }
    RAISE_CS(p_threadInfo->state.dest);
    return result;
}

__ITCMRAM__ HAL_StatusTypeDef handleResponseLargeBuffer(ctrlCommThreadInfo_tp p_threadInfo,
                                                        cncInfo_tp p_cncInfo,
                                                        spiDbMbPacket_tp p_spiPkt) {
    if (p_cncInfo->cbFnPtr != NULL) {
        p_threadInfo->state.spiStats.rxPktCnt++;
        if (p_cncInfo->cmdResponse.flags == FLAG_LARGEBUFFER) {
            cncPayload_t payload = {.cmd.largeBufferAddr=p_spiPkt->largeBufferPtr, .cmd.cncMsgPayloadHeader.peripheral=PER_LOG,
                    .cmd.cncMsgPayloadHeader.action=CNC_ACTION_READ, .cmd.cncMsgPayloadHeader.cncActionData.result=NO_ERROR, .cmd.cncMsgPayloadHeader.addr=0 };
            p_spiPkt->header.cmdResponse.flags = FLAG_LARGEBUFFER;
            p_cncInfo->cbFnPtr(
                p_spiPkt->header.cmd, p_cncInfo->cbId, p_spiPkt->header.xInfo, p_spiPkt->header.cmdResponse, &payload);
            return HAL_OK;
        }
        assert(1 == 0); // for this function to be called FLAG_LARGEBUFFER must be set so should never get here.
    }
    return HAL_ERROR;
}

__ITCMRAM__ HAL_StatusTypeDef handleRxMsg(ctrlCommThreadInfo_tp p_threadInfo,
                                          cncInfo_tp p_cncInfo,
                                          spiDbMbPacket_tp p_spiPkt) {
    // The first byte because of the DB DMA FIFO may not be what we set it to be so just fix it to a known value.
#ifdef SET_FIRST_BYTE_0XA5
    RX_DataCtrl[0] = 0xA5;
#endif

#if TRACE_ANALYZE_SPI || PRINT_FULL_SPI_PACKET || PRINT_SPI_CRC_ERROR
    uint32_t spiBufferIndex = 0;
#endif
    if (p_cncInfo->cmdResponse.flags == FLAG_LARGEBUFFER) {
        xTracePrintCompactF1(urlLogTxMsg, "hspi=%x Large buffer RX", (uint32_t)p_threadInfo->hspi);
        return handleResponseLargeBuffer(p_threadInfo, p_cncInfo, p_spiPkt);
    }
    uint32_t crcCalc =
        HAL_CRC_Calculate(&hcrc, p_spiPkt->u32, (SPI_DBMB_PKT_SIZE - sizeof(uint32_t)) / sizeof(uint32_t));

    uint32_t crcRead = p_spiPkt->crc;
    static uint32_t pktSinceLastReportedError = 0;
    pktSinceLastReportedError++;
#if TRACE_ANALZYE_SPI
    int cnt = SPI_TRACE_BUFFER_SIZE;
    char *nxtBuffer = spiTraceBuffer[p_threadInfo->spiBusId];
    int wrote = snprintf(nxtBuffer, cnt, " SPI %d RX %u  ", p_threadInfo->spiBusId, *(uint16_t *)p_spiPkt);
    nxtBuffer += wrote;
    cnt -= wrote;

    for (spiBufferIndex = 0; spiBufferIndex < sizeof(spiDbMbPacket_t) / PRINT_BYTE_CNT; spiBufferIndex++) {
        wrote = snprintf(nxtBuffer,
                         cnt,
                         "%02x %02x %02x %02x %02x %02x %02x %02x ",
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 4],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 5],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 6],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 7]);
        nxtBuffer += wrote;
        cnt -= wrote;
    }
    if ((sizeof(spiDbMbPacket_t) % PRINT_BYTE_CNT == PRINT_BYTE_CNT / 2)) {
        wrote = snprintf(nxtBuffer,
                         cnt,
                         "%02x %02x %02x %02x ",
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                         p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3]);
    }
    nxtBuffer += wrote;
    cnt -= wrote;

    vTracePrint(dbCommTrace[p_threadInfo->state.dest], spiTraceBuffer[p_threadInfo->spiBusId]);
#endif
    if (crcCalc == crcRead) {

#if PRINT_FULL_SPI_PACKET
        DPRINTF_RAW("*** SPI %d MB RECEIVED from %d\r\n", p_threadInfo->spiBusId, p_threadInfo->state.dest);

        for (spiBufferIndex = 0; spiBufferIndex < sizeof(spiDbMbPacket_t) / PRINT_BYTE_CNT; spiBufferIndex++) {
            DPRINTF_RAW("%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 4],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 5],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 6],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 7]);
        }
        if ((sizeof(spiDbMbPacket_t) % PRINT_BYTE_CNT == PRINT_BYTE_CNT / 2)) {
            DPRINTF_RAW("%02x %02x %02x %02x \r\n",
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3]);
        }
        DPRINTF_RAW("\r\n");

#endif
        if (printSpi) {
            DPRINTF("cmd=%x cmdUid = %d\r\n", p_spiPkt->header.cmd, p_spiPkt->header.cmdResponse.cmdUid);
        }
        if (p_cncInfo->cbFnPtr != NULL) {
            p_threadInfo->state.spiStats.rxPktCnt++;
            p_cncInfo->cbFnPtr(p_spiPkt->header.cmd,
                               p_cncInfo->cbId,
                               p_spiPkt->header.xInfo,
                               p_spiPkt->header.cmdResponse,
                               (cncPayload_tp)(p_spiPkt->spiDBMBPacket_payload));
        }
    } else if (crcCalc == 0x6d5aec34 || crcCalc == 0x473b38fe) {
        // all 0x00 or 0xFF packet, No daughter board responding
        return HAL_ERROR;
    } else {
        p_threadInfo->state.spiStats.crcError++;
        updateDBCrcErrorOccurred(p_threadInfo->state.dest);
        xTracePrintCompactF2(dbCommTrace[p_threadInfo->state.dest], "CRC ERROR, Calc=%08x Read=%08x", crcCalc, crcRead);
        if (pktSinceLastReportedError > PRINT_1_OUT_OF_(30000)) {
            DPRINTF_ERROR("SPI %d dest %d CRC error calc=%x read=%x, totalCRCErr=%lu\r\n",
                          p_threadInfo->spiBusId,
                          p_threadInfo->state.dest,
                          crcCalc,
                          crcRead,
                          p_threadInfo->state.spiStats.crcError);

            pktSinceLastReportedError = 0;
        }

#if PRINT_SPI_CRC_ERROR || PRINT_FULL_SPI_PACKET
        for (spiBufferIndex = 0; spiBufferIndex < sizeof(spiDbMbPacket_t) / PRINT_BYTE_CNT; spiBufferIndex++) {
            DPRINTF_RAW("%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 4],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 5],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 6],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 7]);
        }
        if ((sizeof(spiDbMbPacket_t) % PRINT_BYTE_CNT == PRINT_BYTE_CNT / 2)) {
            DPRINTF_RAW("%02x %02x %02x %02x\r\n",
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 0],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 1],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 2],
                        p_spiPkt->u8[PRINT_BYTE_CNT * spiBufferIndex + 3]);
        }
        DPRINTF_RAW("\r\n");
#endif
        return HAL_ERROR;
    }
    return HAL_OK;
}

__ITCMRAM__ void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    volatile SPI_HandleTypeDef *t = hspi;
    if (hspi == &hspi1) {
        vTaskNotifyGiveFromISR(spiCommThreadInfo[0].threadId, &xHigherPriorityTaskWoken);
    } else if (hspi == &hspi2) {
        vTaskNotifyGiveFromISR(spiCommThreadInfo[1].threadId, &xHigherPriorityTaskWoken);
    } else if (hspi == &hspi3) {
        vTaskNotifyGiveFromISR(spiCommThreadInfo[2].threadId, &xHigherPriorityTaskWoken);
    } else {
        DPRINTF_ERROR("hspi %p not inuse \r\n", t);

        return;
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

__ITCMRAM__ void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    HAL_SPI_TxRxCpltCallback(hspi);
}

static uint16_t cliResult;

void appendSpiStatsToBuffer(char *buf, uint32_t bufSz, uint32_t spiId) {
    char *nxt = buf;

    nxt += snprintf((char *)nxt, bufSz - (nxt - buf), "SPI%ld:\r\n", spiId + 1);
    nxt += snprintf(
        (char *)nxt, bufSz - (nxt - buf), "\tTx Cnt     = %lu\r\n", spiCommThreadInfo[spiId].state.spiStats.txPktCnt);
    nxt += snprintf(
        (char *)nxt, bufSz - (nxt - buf), "\tRx Cnt     = %lu\r\n", spiCommThreadInfo[spiId].state.spiStats.rxPktCnt);
    nxt += snprintf(
        (char *)nxt, bufSz - (nxt - buf), "\tCRC Errors = %lu\r\n", spiCommThreadInfo[spiId].state.spiStats.crcError);
    nxt += snprintf(
        (char *)nxt, bufSz - (nxt - buf), "\tFull Q Cnt = %lu\r\n", spiCommThreadInfo[spiId].state.spiStats.qFullCnt);
    nxt += snprintf(
        (char *)nxt, bufSz - (nxt - buf), "\tEnabled DBs= %lu\r\n", spiCommThreadInfo[spiId].state.spiStats.enableCnt);
    nxt += snprintf((char *)nxt,
                    bufSz - (nxt - buf),
                    "\tPkt Rate   = %lu\r\n",
                    spiCommThreadInfo[spiId].state.spiStats.txPktRatePerSec);
    nxt += snprintf(
        (char *)nxt, bufSz - (nxt - buf), "\tMsg Pending= %lu\r\n", spiCommThreadInfo[spiId].state.spiStats.msgPending);
}

int16_t spiCliCmd(CLI *hCli, int argc, char *argv[]) {
    uint16_t success = 0;
    uint32_t min = 0;
    uint32_t max = 1;
    if (argc > CMD_ARG_IDX) {
        if (argc == CMD_PARAM_CNT(0) && strcmp(argv[CMD_ARG_IDX], "stats") == 0) {
            if (strcmp(argv[CMD_PARAM_IDX(0)], "all") == 0) {
                min = 0;
                max = MAX_SPI;
            } else {
                uint32_t idx = atoi(argv[CMD_PARAM_IDX(0)]);
                if (idx >= MAX_SPI) {
                    CliPrintf(hCli, "value %d is out of range 0-3 or all\r\n", idx);
                    return success;
                }
                min = idx;
                max = idx + 1;
            }
            success = 1;
            CliPrintf(hCli, "System:\r\n");
            CliPrintf(hCli, "\tSpi Msg Pool Memory Exhaustion = %lu\r\n", noSpiMsgPoolMemoryCnt);
            for (int i = min; i < max; i++) {
                CliPrintf(hCli, "SPI%d:\r\n", i + 1);
                CliPrintf(hCli, "\tTx Cnt     = %lu\r\n", spiCommThreadInfo[i].state.spiStats.txPktCnt);
                CliPrintf(hCli, "\tRx Cnt     = %lu\r\n", spiCommThreadInfo[i].state.spiStats.rxPktCnt);
                CliPrintf(hCli, "\tCRC Errors = %lu\r\n", spiCommThreadInfo[i].state.spiStats.crcError);
                CliPrintf(hCli, "\tFull Q Cnt = %lu\r\n", spiCommThreadInfo[i].state.spiStats.qFullCnt);
                CliPrintf(hCli, "\tEnabled DBs= %lu\r\n", spiCommThreadInfo[i].state.spiStats.enableCnt);
                CliPrintf(hCli, "\tPkt Rate   = %lu\r\n", spiCommThreadInfo[i].state.spiStats.txPktRatePerSec);
                CliPrintf(hCli, "\tMsg Pending= %lu\r\n", spiCommThreadInfo[i].state.spiStats.msgPending);
            }
        } else if (argc == CMD_PARAM_CNT(0) && strcmp(argv[CMD_ARG_IDX], "clear") == 0) {
            if (strcmp(argv[CMD_PARAM_IDX(0)], "all") == 0) {
                min = 0;
                max = MAX_SPI;
                noSpiMsgPoolMemoryCnt = 0;
            } else {
                uint32_t idx = atoi(argv[CMD_PARAM_IDX(0)]);
                if (idx >= MAX_SPI) {
                    CliPrintf(hCli, "value %d is out of range 0-3 or all\r\n", idx);
                    return success;
                }
                min = idx;
                max = idx + 1;
            }
            success = 1;
            for (int i = min; i < max; i++) {
                spiCommThreadInfo[i].state.spiStats.txPktCnt = 0;
                spiCommThreadInfo[i].state.spiStats.rxPktCnt = 0;
                spiCommThreadInfo[i].state.spiStats.crcError = 0;
                spiCommThreadInfo[i].state.spiStats.qFullCnt = 0;
            }
        } else if (argc == CMD_PARAM_CNT(1) && strcmp(argv[CMD_ARG_IDX], "send") == 0) {
            int destination = atoi(argv[CMD_PARAM_IDX(0)]);
            int addr = atoi(argv[CMD_PARAM_IDX(1)]);
            // send test packet to Device at destination
            static uint8_t xInfo = 0x34;
            xInfo++;
            cncMsgPayload_t payload = {.cncMsgPayloadHeader.peripheral = PER_ADC, .cncMsgPayloadHeader.action = 0, .cncMsgPayloadHeader.addr = addr};
            DPRINTF("peripheral=%d, xinfo=%d, addr=%d xInfo=%d\r\n",
                    payload.cncMsgPayloadHeader.peripheral,
                    payload.cncMsgPayloadHeader.action,
                    payload.cncMsgPayloadHeader.addr,
                    xInfo);
            spiSendMsg(
                destination, SPICMD_CNC, xInfo, CMD_UID_DONT_CARE, &payload, ctrlSpiCommCliCallback, (uint32_t)hCli, 0);
            osStatus status = stallCliUntilComplete(1000);
            if (status == osOK) {
                cliResult = 1;
            } else {
                DPRINTF_ERROR("adc cli command timeout\r\n");
                cliResult = 0;
            }
        } else if (argc == CMD_PARAM_CNT(1) && strcmp(argv[CMD_ARG_IDX], "txcrcerror") == 0) {
            int destination = atoi(argv[CMD_PARAM_IDX(0)]);
            int crcCnt = atoi(argv[CMD_PARAM_IDX(1)]);
            spiCommThreadInfo[destination].state.sendCrcErrors = crcCnt;
        } else if ((argc == CMD_PARAM_CNT(0) && strcmp(argv[CMD_ARG_IDX], "mapBoardToDbg6") == 0)) {
            // This allows us to use saleae and see the CS state of any board with DBG6 on the main board
            mappingBoard = atoi(argv[CMD_PARAM_IDX(0)]);
            if (mappingBoard > MAX_CS_ID) {
                mappingBoard = MAX_CS_ID;
            }
        }
    }
    return success;
}

osStatus ctrlSpiCommCliCallback(spiDbMbCmd_e spiDbMbCmd,
                                uint32_t cbId,
                                uint8_t xInfo,
                                spiDbMbPacketCmdResponse_t cmdResponse,
                                cncPayload_tp p_cncMsg) {
    CLI *hcli = (CLI *)cbId;
    if (p_cncMsg->cmd.cncMsgPayloadHeader.cncActionData.result == osOK) {
        CliPrintf(hcli, "Spi command returned successful\r\n");
        CliPrintf(hcli, "Xinfo=%d, addr=%d, value=%d\r\n", xInfo, p_cncMsg->cmd.cncMsgPayloadHeader.addr, p_cncMsg->cmd.value);
    } else {
        CliPrintf(hcli, "Write command failed %d\r\n", p_cncMsg->cmd.cncMsgPayloadHeader.cncActionData.result);
    }
    osStatus status = cliCommandComplete();
    if (status != osOK) {
        DPRINTF_ERROR("cliCommand Complete failed with %d\r\n", status);
    }
    return status;
}

void updateSpiStats(void) {
    for (int i = 0; i < MAX_SPI; i++) {
        spiCommThreadInfo[i].state.spiStats.txPktRatePerSec =
            spiCommThreadInfo[i].state.spiStats.txPktCnt - spiCommThreadInfo[i].state.spiStats.lastTxPktCnt;
        spiCommThreadInfo[i].state.spiStats.lastTxPktCnt = spiCommThreadInfo[i].state.spiStats.txPktCnt;
    }
}
