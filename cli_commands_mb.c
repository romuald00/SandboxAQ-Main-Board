/*
 * cli_commands_db.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 27, 2023
 *      Author: jzhang
 */

#include "MB_gatherTask.h"
#include "cli/cli.h"
#include "cli/cli_print.h"
#include "cli_task.h"
#include "debugPrint.h"
#include "freeRTOSConfig.h"
#include "lwip.h"
#include "lwip/apps/lwiperf.h"
#include "lwip/stats.h"
#include "macros.h"
#include "peripherals/MB_handleLogs.h"
#include "pwm.h"
#include "pwmPinConfig.h"
#include "ramlog.h"
#include "registerParams.h"
#include "taskWatchdog.h"
#include "watchDog.h"
#include "webCliMisc.h"
#include <cli_commands_board.h>
#include <stdint.h>
#include <string.h>

#define SUCCESS_GOOD 1
#define SUCCESS_BAD 0

#ifdef STM32F411xE
#define FREERTOS_STATS_BUFFER_BYTES 2048
#else
#define FREERTOS_STATS_BUFFER_BYTES 4096
#endif

void *iperfServerHandle = NULL;

#define CMD_IDX 1
#define NO_PARAMS (CMD_IDX + 1)
#define SUBCMD_IDX 2
#define DATA_IDX 3
#define IP_DATA_IDX(x) (x + 2)
#define IP_NUM_DATA_PARAMS(x) (IP_DATA_IDX(0) + x)

#define CMD_DATA_IP_ADDR_IDX 2
#define CMD_DATA_NET_MASK_IDX 3
#define CMD_DATA_GATEWAY_IDX 4
#define PWM_DUTY_100PERCENT 100
#define PWM_DUTY_50PERCENT 50

#define MAX_BUFFER_SIZE_BYTES 40
#define IP4_ADDR_SIZE_BYTES 4

int16_t nameCommand(CLI *hCli, int argc, char *argv[]) {
    CliWriteString(hCli, "MB\n\r");
    return CLI_RESULT_SUCCESS;
}

BEGIN_NEEDS_LARGE_STACK
int16_t testCommand(CLI *hCli, int argc, char *argv[]) {
    uint8_t success = SUCCESS_BAD;
    int16_t subCmdRet;

    if (argc > CMD_IDX) {
        if ((argc == (SUBCMD_IDX + 1)) && (strcmp(argv[CMD_IDX], "note") == 0)) {
            /* write argv[SUBCMD_IDX] into the log */
            DPRINTF_INFO("NOTE: %s\r\n", argv[SUBCMD_IDX]);
            success = SUCCESS_GOOD;
        } else if ((argc == NO_PARAMS) && (strcmp(argv[CMD_IDX], "wdog") == 0)) {
            for (uint8_t i = 0; i < WDT_NUM_TASKS; i++) {
                CliPrintf(hCli,
                          "%2d - %-16s %5d %5d\r\n",
                          i,
                          watchdogGetTaskName(i),
                          watchdogGetHighWatermark(i),
                          watchdogGetPeriod(i));
            }
            success = SUCCESS_GOOD;
        } else if ((argc == NO_PARAMS) && (strcmp(argv[CMD_IDX], "lwipstats") == 0)) {
            CliWriteString(hCli, "Dumping LwIP stats...\r\n");
            LINK_STATS_DISPLAY();
            osDelay(100);
            ETHARP_STATS_DISPLAY();
            osDelay(100);
            IPFRAG_STATS_DISPLAY();
            osDelay(100);
            IP_STATS_DISPLAY();
            osDelay(100);
            ICMP_STATS_DISPLAY();
            osDelay(100);
            UDP_STATS_DISPLAY();
            osDelay(100);
            TCP_STATS_DISPLAY();
            osDelay(100);
            MEM_STATS_DISPLAY();
            for (uint16_t i = 0; i < MEMP_MAX; i++) {
                osDelay(100);
                MEMP_STATS_DISPLAY(i);
            }
            osDelay(100);
            SYS_STATS_DISPLAY();
            success = SUCCESS_GOOD;
        } else if ((argc == NO_PARAMS) && (strcmp(argv[CMD_IDX], "taskstats") == 0)) {
            CliWriteString(hCli, "Dumping task stats...\r\n");
            char *taskBuff = pvPortMalloc(FREERTOS_STATS_BUFFER_BYTES); // FreeRTOS task statistics printing area
            if (taskBuff == NULL) {
                CliWriteString(hCli, "Malloc Failure...\r\n");
                return CLI_RESULT_ERROR;
            }
            vTaskGetCustomRunTimeStats(taskBuff);
            CliWriteString(hCli, taskBuff);
            CliWriteString(hCli, "\r\n");
            vPortFree(taskBuff);
            success = SUCCESS_GOOD;
        } else if ((argc == 2) && (strcmp(argv[CMD_IDX], "stacks") == 0)) {
            TaskStatus_t *taskStatus;
            volatile UBaseType_t numTasks = uxTaskGetNumberOfTasks();
            taskStatus = pvPortMalloc(numTasks * sizeof(TaskStatus_t));
            numTasks = uxTaskGetSystemState(taskStatus, numTasks, NULL);

            if (taskStatus != NULL) {
                for (uint8_t i = 0; i < numTasks; i++) {
                    CliPrintf(hCli, "%-16s - %6d\r\n", taskStatus[i].pcTaskName, taskStatus[i].usStackHighWaterMark);
                }

                vPortFree(taskStatus);
            } else {
                CliWriteString(hCli, "Error allocating status memory!\r\n");
            }

            success = SUCCESS_GOOD;
        } else if ((argc == NO_PARAMS) && (strcmp(argv[CMD_IDX], "ticks") == 0)) {
            CliPrintf(hCli, "%d\r\n", HAL_GetTick());

            success = SUCCESS_GOOD;

        } else if ((argc == (SUBCMD_IDX + 1)) && (strcmp(argv[CMD_IDX], "iperf") == 0)) {
            if (strcmp(argv[SUBCMD_IDX], "start") == 0) {
                if (iperfServerHandle == NULL) {
                    CliWriteString(hCli, "Starting iperf server on port 5001...\r\n");
                    iperfServerHandle = lwiperf_start_tcp_server_default(NULL, NULL);
                    if (iperfServerHandle == NULL) {
                        CliWriteString(hCli, "Error starting iperf server\r\n");
                    }
                } else {
                    CliWriteString(hCli, "iperf server already running\r\n");
                }
                success = SUCCESS_GOOD;
            }
        } else if ((argc == NO_PARAMS) && (strcmp(argv[CMD_IDX], "dumplog") == 0)) {
            subCmdRet = testdumplogCommand(hCli, argc, argv);
            if (subCmdRet == CLI_RESULT_SUCCESS) {
                success = SUCCESS_GOOD;
            } else if (subCmdRet != CLI_RESULT_BAD_ARGUMENT) {
                return subCmdRet;
            }
        } else if ((argc == (SUBCMD_IDX + 1)) && (strcmp(argv[CMD_IDX], "sensorlog")) == 0) {
            uint32_t destination = atoi(argv[SUBCMD_IDX]);
            if (destination >= MAX_CS_ID) {
                CliPrintf(hCli, "%d is out of range 0-%d\r\n", destination, MAX_CS_ID - 1);
                return CLI_RESULT_BAD_ARGUMENT;
            }
            success = cliSensorLogGet(hCli, destination);

        } else if ((argc == (SUBCMD_IDX + 1)) && (strcmp(argv[CMD_IDX], "clearsensorlog")) == 0) {
            uint32_t destination = atoi(argv[SUBCMD_IDX]);
            if (destination >= MAX_CS_ID) {
                CliPrintf(hCli, "%d is out of range 0-%d\r\n", destination, MAX_CS_ID - 1);
                return CLI_RESULT_BAD_ARGUMENT;
            }
            success = cliClearSensorLog(hCli, destination);

        } else if (strcmp(argv[CMD_IDX], "fillramlog") == 0) {

            uint32_t start = HAL_GetTick();
            uint32_t end;
            const char *testString = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n";
            uint8_t testStringLen = strlen(testString);
            // Write the full log plus some extra to ensure we wrap around
            for (uint32_t i = 0; i < RAM_LOG_SIZE_BYTES + 1; i += testStringLen) {
                if (!dprintfLock(DEBUG_PRINT_WAIT)) {
                    CliWriteString(hCli, "Failed to get RAM log lock\r\n");
                    return CLI_RESULT_ERROR;
                }
                ramLogWrite(0, testString);
                dprintfUnlock();
            }
            end = HAL_GetTick();
            CliWriteString(hCli, "RAM log filled\r\n");
            CliPrintf(hCli, "Time to fill: %d\r\n", end - start);
            success = SUCCESS_GOOD;
        } else if (strcmp(argv[CMD_IDX], "clearramlog") == 0) {
            if (ramLogEmpty() == 0) {
                CliWriteString(hCli, "RAM log cleared\r\n");
            } else {
                CliWriteString(hCli, "Failed to clear RAM log\r\n");
            }
            success = SUCCESS_GOOD;
        } else if (strcmp(argv[CMD_IDX], "ramloglength") == 0) {
            uint32_t len = usableRamLogLength();
            CliPrintf(hCli, "%d\r\n", len);
            success = SUCCESS_GOOD;
        } else if ((argc == NO_PARAMS) && (strcmp(argv[CMD_IDX], "assert") == 0)) {
            assert(0);
        } else if ((argc == NO_PARAMS) && (strcmp(argv[CMD_IDX], "stopwdt") == 0)) {
            while (1) {
                // NOP
            };
        } else if ((argc == NO_PARAMS) && (strcmp(argv[CMD_IDX], "heap") == 0)) {
            CliPrintf(hCli, "Free Heap = 0x%x (%lu) Bytes\r\n", xPortGetFreeHeapSize());
            HeapStats_t heapStats;
            vPortGetHeapStats(&heapStats);
            CliPrintf(hCli,
                      "Available Heap        = 0x%x (%lu) Bytes\r\n",
                      heapStats.xAvailableHeapSpaceInBytes,
                      heapStats.xAvailableHeapSpaceInBytes);
            CliPrintf(hCli,
                      "Largest Free Block    = 0x%x (%lu) Bytes\r\n",
                      heapStats.xSizeOfLargestFreeBlockInBytes,
                      heapStats.xSizeOfLargestFreeBlockInBytes);
            CliPrintf(hCli,
                      "Smallest Free Block   = 0x%x (%lu) Bytes\r\n",
                      heapStats.xSizeOfSmallestFreeBlockInBytes,
                      heapStats.xSizeOfSmallestFreeBlockInBytes);
            CliPrintf(hCli,
                      "Number of Free Block  = 0x%x (%lu) Bytes\r\n",
                      heapStats.xNumberOfFreeBlocks,
                      heapStats.xNumberOfFreeBlocks);
            CliPrintf(hCli,
                      "Min Free Bytes        = 0x%x (%lu) Bytes\r\n",
                      heapStats.xMinimumEverFreeBytesRemaining,
                      heapStats.xMinimumEverFreeBytesRemaining);
            CliPrintf(hCli,
                      "Number of Allocations = 0x%x (%lu)\r\n",
                      heapStats.xNumberOfSuccessfulAllocations,
                      heapStats.xNumberOfSuccessfulAllocations);
            CliPrintf(hCli,
                      "Number of Frees       = 0x%x (%lu)\r\n",
                      heapStats.xNumberOfSuccessfulFrees,
                      heapStats.xNumberOfSuccessfulFrees);
            CliPrintf(hCli,
                      "Alloc-Free            = 0x%x\r\n",
                      heapStats.xNumberOfSuccessfulAllocations - heapStats.xNumberOfSuccessfulFrees);
            success = SUCCESS_GOOD;
        } else if ((argc == IP_NUM_DATA_PARAMS(1)) && (strcmp(argv[CMD_IDX], "dumprxbuf") == 0)) {
            uint32_t count = atoi(argv[SUBCMD_IDX]);
            dumpRxBuffer(hCli, count);
            success = SUCCESS_GOOD;
        } else if (argc == IP_NUM_DATA_PARAMS(1) && strcmp(argv[CMD_IDX], "correctEmptyTo7")) {
            uint32_t target = atoi(argv[SUBCMD_IDX]);
            correctTargetToNewValue(target, BOARDTYPE_EMPTY);
            success = SUCCESS_GOOD;
        } else if (argc == IP_NUM_DATA_PARAMS(2) && strcmp(argv[CMD_IDX], "setTargetToNewValue")) {
            uint32_t target = atoi(argv[SUBCMD_IDX]);
            uint32_t newValue = atoi(argv[SUBCMD_IDX + 1]);
            correctTargetToNewValue(target, newValue);
            success = SUCCESS_GOOD;
        }
    }

    if (!success) {
        CliWriteString(hCli, "Error! Malformed input. See 'help test'\r\n");
        return CLI_RESULT_ERROR;
    }
    return CLI_RESULT_SUCCESS;
}
END_NEEDS_LARGE_STACK

int16_t ipCliCommand(CLI *hCli, int argc, char *argv[]) {
#define MAX_BUFFER 70
    uint32_t cnt = 0;
    int16_t success = SUCCESS_BAD;
    int rc;
    char buffer[MAX_BUFFER];
    if (argc == IP_NUM_DATA_PARAMS(0)) {

        if (strcmp(argv[CMD_IDX], "get_ip") == 0) {
            uint8_t *ipV;

            registerInfo_t regInfo = {.mbId = IP_ADDR, .type = DATA_UINT};
            rc = registerRead(&regInfo);
            assert(rc == 0);
            ipV = (uint8_t *)&regInfo.u.dataUint;
            cnt += snprintf(&buffer[cnt], MAX_BUFFER - cnt, "ip %u.%u.%u.%u", ipV[0], ipV[1], ipV[2], ipV[3]);
            CliPrintf(hCli, "\r\n      ip %u.%u.%u.%u\r\n", ipV[0], ipV[1], ipV[2], ipV[3]);

            regInfo.mbId = NETMASK;
            rc = registerRead(&regInfo);
            assert(rc == 0);
            ipV = (uint8_t *)&regInfo.u.dataUint;
            CliPrintf(hCli, " netmask %u.%u.%u.%u\r\n", ipV[0], ipV[1], ipV[2], ipV[3]);

            regInfo.mbId = GATEWAY;
            rc = registerRead(&regInfo);
            assert(rc == 0);
            ipV = (uint8_t *)&regInfo.u.dataUint;
            CliPrintf(hCli, " gateway %u.%u.%u.%u\r\n", ipV[0], ipV[1], ipV[2], ipV[3]);
            success = SUCCESS_GOOD;
        } else if (strcmp(argv[CMD_IDX], "get_http") == 0) {
            registerInfo_t regInfo = {.mbId = HTTP_PORT, .type = DATA_UINT};
            rc = registerRead(&regInfo);
            assert(rc == 0);
            CliPrintf(hCli, "HTTP PORT %u\r\n", regInfo.u.dataUint);
            success = SUCCESS_GOOD;
        } else if (strcmp(argv[CMD_IDX], "get_udpclient") == 0) {
            registerInfo_t regInfo = {.mbId = UDP_SERVER_IP, .type = DATA_UINT};
            rc = registerRead(&regInfo);
            assert(rc == 0);
            uint32_t serverIp = regInfo.u.dataUint;
            regInfo.mbId = UDP_SERVER_PORT;
            rc = registerRead(&regInfo);
            assert(rc == 0);

            uint8_t *ipV = (uint8_t *)&serverIp;
            CliPrintf(hCli, "UDP Server %u.%u.%u.%u:%u\r\n", ipV[0], ipV[1], ipV[2], ipV[3], regInfo.u.dataUint);
            success = SUCCESS_GOOD;
        } else if (strcmp(argv[CMD_IDX], "get_udptxport") == 0) {
            registerInfo_t regInfo = {.mbId = UDP_TX_PORT, .type = DATA_UINT};
            rc = registerRead(&regInfo);
            assert(rc == 0);
            CliPrintf(hCli, "UDP TX PORT %u\r\n", regInfo.u.dataUint);
        } else if (strcmp(argv[CMD_IDX], "get_ip_tx_type") == 0) {
            registerInfo_t regInfo = {.mbId = IP_TX_DATA_TYPE, .type = DATA_UINT};
            rc = registerRead(&regInfo);

            assert(rc == 0);
            if (regInfo.u.dataUint < IPTYPE_MAX) {
                CliPrintf(
                    hCli, "IP TX DATA TYPE %s (%d)\r\n", IPTYPE_e_Strings[regInfo.u.dataUint], regInfo.u.dataUint);
            } else {
                CliPrintf(hCli, "IP TX DATA TYPE (%d) unknown using UDP\r\n", regInfo.u.dataUint);
            }
            success = SUCCESS_GOOD;
        }

    } else if (argc == IP_NUM_DATA_PARAMS(1)) {
        if (strcmp(argv[CMD_IDX], "set_http") == 0) {
            uint32_t httpPort = atoi(argv[IP_DATA_IDX(0)]);

            if (httpPort < HTTP_PORT_MIN || httpPort > HTTP_PORT_MAX) {
                CliPrintf(hCli, "HTTP PORT %u out of range [%u - %u]\r\n", httpPort, HTTP_PORT_MIN, HTTP_PORT_MAX);
                return 0;
            }
            registerInfo_t regInfo = {.mbId = HTTP_PORT, .type = DATA_UINT, .u.dataUint = httpPort};
            rc = registerWrite(&regInfo);
            assert(rc == 0);
            success = SUCCESS_GOOD;
        } else if (strcmp(argv[CMD_IDX], "set_udptxport") == 0) {
            uint32_t udpTxPort = atoi(argv[IP_DATA_IDX(0)]);
            if (udpTxPort < UDP_TX_PORT_MIN || udpTxPort > UDP_TX_PORT_MAX) {
                CliPrintf(
                    hCli, "UDP TX PORT %u out of range [%u - %u]\r\n", udpTxPort, UDP_TX_PORT_MIN, UDP_TX_PORT_MAX);
                return 0;
            }
            registerInfo_t regInfo = {.mbId = UDP_TX_PORT, .type = DATA_UINT, .u.dataUint = udpTxPort};
            rc = registerWrite(&regInfo);
            assert(rc == 0);
            CliPrintf(hCli, "UDP TX PORT %u\r\n", regInfo.u.dataUint);
        } else if (strcmp(argv[CMD_IDX], "set_ip_tx_type") == 0) {
            uint32_t txType = atoi(argv[IP_DATA_IDX(0)]);
            registerInfo_t regInfo = {.mbId = IP_TX_DATA_TYPE, .type = DATA_UINT, .u.dataUint = txType};

            if (txType == IPTYPE_UDP || txType == IPTYPE_TCP) {
                registerWrite(&regInfo);
                success = SUCCESS_GOOD;
            } else {
                DPRINTF_ERROR("txType out of range %d\r\n", txType);
                success = SUCCESS_BAD;
            }
        }

    } else if (argc == IP_NUM_DATA_PARAMS(2)) {
        if (strcmp(argv[CMD_IDX], "set_udpclient") == 0) {
            if (!validIpStr(argv[IP_DATA_IDX(0)])) {
                CliPrintf(hCli, "Value %s is not a valid ip address format\r\n", argv[SUBCMD_IDX]);
                return 0;
            }

            char buffer[MAX_BUFFER_SIZE_BYTES];
            int cnt = 0;
            uint8_t serverIp[IP4_ADDR_SIZE_BYTES] = {0};
            strlcpy(buffer, argv[IP_DATA_IDX(0)], sizeof(buffer) - 1);
            char *token = strtok(buffer, ".");
            while (token != NULL && cnt < IP4_ADDR_SIZE_BYTES) {
                serverIp[cnt] = atoi(token);
                cnt++;
                token = strtok(NULL, ".");
            }
            DPRINTF_INFO("set udpServer %u.%u.%u.%u\r\n", serverIp[0], serverIp[1], serverIp[2], serverIp[3]);
            uint32_t v = *(uint32_t *)serverIp;
            ip4_addr_t testIp;
            IP4_ADDR(&testIp, serverIp[0], serverIp[1], serverIp[2], serverIp[3]);
            assert(v == testIp.addr);
            registerInfo_t regInfo = {.mbId = UDP_SERVER_IP, .type = DATA_UINT, .u.dataUint = v};
            rc = registerWrite(&regInfo);
            assert(rc == 0);

            regInfo.mbId = UDP_SERVER_PORT;
            regInfo.u.dataUint = atoi(argv[IP_DATA_IDX(1)]);
            rc = registerWrite(&regInfo);
            assert(rc == 0);
            success = SUCCESS_GOOD;
        }

    } else if (argc == IP_NUM_DATA_PARAMS(3)) {

        registerInfo_t regInfo = {.type = DATA_UINT};

        if (strcmp(argv[CMD_IDX], "set_ip") == 0) {
            for (int j = 2; j < 5; j++) {
                if (!validIpStr(argv[j])) {
                    CliPrintf(hCli, "Value %s is not a valid ip address format\r\n", argv[j]);
                    return 0;
                }
            }

            for (int i = CMD_DATA_IP_ADDR_IDX; i <= CMD_DATA_GATEWAY_IDX; i++) {
                char buffer[MAX_BUFFER_SIZE_BYTES];
                int cnt = 0;
                uint8_t dotAddress[IP4_ADDR_SIZE_BYTES] = {0};
                strlcpy(buffer, argv[i], sizeof(buffer) - 1);
                char *token = strtok(buffer, ".");
                while (token != NULL) {
                    dotAddress[cnt] = atoi(token);
                    cnt++;
                    token = strtok(NULL, ".");
                }
                ip4_addr_t testIp;
                IP4_ADDR(&testIp, dotAddress[0], dotAddress[1], dotAddress[2], dotAddress[3]);

                regInfo.u.dataUint = testIp.addr;
                if (i == CMD_DATA_IP_ADDR_IDX) {
                    regInfo.mbId = IP_ADDR;
                } else if (i == CMD_DATA_NET_MASK_IDX) {
                    regInfo.mbId = NETMASK;
                } else if (i == CMD_DATA_GATEWAY_IDX) {
                    regInfo.mbId = GATEWAY;
                }
                DPRINTF_INFO("Write regInfo.mbId = %d\r\n", regInfo.mbId);
                rc = registerWrite(&regInfo);
                assert(rc == 0);
            }
        }
    }
    if (success == 1) {
        CliPrintf(hCli, "Success\r\n");
        return CLI_RESULT_SUCCESS;
    }
    return CLI_RESULT_ERROR;
}

int16_t resetCmd(CLI *hCli, int argc, char *argv[]) {
    int16_t cliResult = CLI_RESULT_ERROR;

    if (argc > CMD_IDX && strcmp("write", argv[CMD_IDX]) == 0) {
        if (argc > SUBCMD_IDX && strcmp("delay", argv[SUBCMD_IDX]) == 0) {
            uint32_t value = atoi(argv[DATA_IDX]);
            registerInfo_t registerInfo = {.mbId = REBOOT_DELAY_MS, .type = DATA_UINT, .u.dataUint = value};
            cliResult = registerWrite(&registerInfo);
            if (cliResult == 0) {
                CliWriteString(hCli, "Success\n\r");
            }
        } else if (argc > SUBCMD_IDX && strcmp("trigger", argv[SUBCMD_IDX]) == 0) {
            registerInfo_t registerInfo = {.mbId = REBOOT_FLAG, .type = DATA_UINT, .u.dataUint = 1};
            cliResult = registerWrite(&registerInfo);
            if (cliResult == 0) {
                CliWriteString(hCli, "Success\n\r");
            }
        } else {
            CliWriteString(hCli, "Error! Malformed input\n\r");
            return CLI_RESULT_ERROR;
        }
    } else if (argc > CMD_IDX && strcmp("read", argv[CMD_IDX]) == 0) {
        if (argc > SUBCMD_IDX && strcmp("delay", argv[SUBCMD_IDX]) == 0) {
            registerInfo_t registerInfo = {.mbId = REBOOT_DELAY_MS, .type = DATA_UINT};
            cliResult = registerRead(&registerInfo);
            if (cliResult == 0) {
                CliPrintf(hCli, "Register read: %d\n\r", registerInfo.u.dataUint);
            }
        } else if (argc > SUBCMD_IDX && strcmp("trigger", argv[SUBCMD_IDX]) == 0) {
            registerInfo_t registerInfo = {.mbId = REBOOT_FLAG, .type = DATA_UINT};
            cliResult = registerRead(&registerInfo);
            if (cliResult == 0) {
                CliPrintf(hCli, "Register read: %d\n\r", registerInfo.u.dataUint);
            }
        } else {
            CliWriteString(hCli, "Error! Malformed input\n\r");
            return CLI_RESULT_ERROR;
        }
    } else {
        CliWriteString(hCli, "Error! Malformed input\n\r");
        return CLI_RESULT_ERROR;
    }
    return CLI_RESULT_SUCCESS;
}

int16_t adcCommand(CLI *hCli, int argc, char *argv[]) {
    /* arguments should be PWM pin, command, value to write */
    if (argc >= CMD_IDX) {
        /* get port */
        pwmPort_e port = ADC_PIN;

        /* get command */
        if (argc > DATA_IDX && strcmp(argv[CMD_IDX], "set_duty") == 0) {
            uint32_t duty = atoi(argv[DATA_IDX]);
            CliPrintf(hCli, "Duty: %d\n\r", duty);
            pwmPinStates[port].dutyInfo.u.dataUint = duty;
            if (registerWrite(&pwmPinStates[port].dutyInfo) != 0) {
                CliWriteString(hCli, "Failure\n\r");
                return CLI_RESULT_ERROR;
            }
            CliPrintf(hCli, "Success: %d\n\r", pwmPinStates[port].dutyInfo.u.dataUint);
        } else if (argc > DATA_IDX && strcmp(argv[CMD_IDX], "set_frequency") == 0) {
            uint32_t freq = atoi(argv[DATA_IDX]);
            CliPrintf(hCli, "Frequency: %d\n\r", freq);
            pwmPinStates[port].freqInfo.u.dataUint = freq * 100; // scale factor
            if (registerWrite(&pwmPinStates[port].freqInfo) != 0) {
                CliWriteString(hCli, "Failure\n\r");
                return CLI_RESULT_ERROR;
            }
            CliPrintf(hCli, "Success: %d\n\r", pwmPinStates[port].freqInfo.u.dataUint / 100);
        } else if (argc == (CMD_IDX + 1) && strcmp(argv[CMD_IDX], "get_duty") == 0) {
            if (registerRead(&pwmPinStates[port].dutyInfo) != 0) {
                CliWriteString(hCli, "Failure\n\r");
                return CLI_RESULT_ERROR;
            }
            CliPrintf(hCli, "Duty: %d\n\r", pwmPinStates[port].dutyInfo.u.dataUint);
        } else if (argc == (CMD_IDX + 1) && strcmp(argv[CMD_IDX], "get_frequency") == 0) {
            if (registerRead(&pwmPinStates[port].freqInfo) != 0) {
                CliWriteString(hCli, "Failure\n\r");
                return CLI_RESULT_ERROR;
            }
            CliPrintf(hCli, "Frequency: %d hz\n\r", pwmPinStates[port].freqInfo.u.dataUint / 100);
        } else if (strcmp(argv[CMD_IDX], "start") == 0) {
            int ret = pwmStart(port);
            if (g_adcModeSwitch == ADC_MODE_CONTINUOUS) {
                pwmPinStates[port].dutyInfo.u.dataUint = PWM_DUTY_100PERCENT;
            } else {
                pwmPinStates[port].dutyInfo.u.dataUint = PWM_DUTY_50PERCENT;
            }
            registerWrite(&pwmPinStates[port].dutyInfo);
            CliPrintf(hCli, "Success: %d\n\r", ret);
        } else if (strcmp(argv[CMD_IDX], "stop") == 0) {
            int ret = pwmStop(port);
            CliPrintf(hCli, "Success: %d\n\r", ret);
        }

    } else {
        CliWriteString(hCli, "Error! Malformed input\n\r");
        return CLI_RESULT_ERROR;
    }
    return CLI_RESULT_SUCCESS;
}
