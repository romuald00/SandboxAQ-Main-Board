/*
 * eeprom.c
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 1, 2023
 *      Author: jzhang
 */

#include "eeprom.h"
#include "debugPrint.h"
#include "eeprma2_m24.h"
#include "raiseIssue.h"
#include <stdio.h>
#include <string.h>

#define ASSERT_DELAY_MS 2
#define BUFFER_MAX 16
// Hook to handle eeprom not present
// will use a shadow register that will not be saved over reboot
static bool g_eepromPresent = false;

typedef struct {
    uint32_t key;
    uint32_t ipAddr;
    uint32_t httpPort;
    uint32_t udpTxPort;
    uint32_t gateway;
    uint32_t netmask;
    uint32_t serverIp;
    uint32_t serverUdpPort;
    uint8_t serial[16];
    uint32_t hwType;
    uint32_t hwVersion;
} eepromStruct_t, *eepromStruct_tp;

/* Eeprom register lookup table, must be same order as EepromRegisters enum */
#ifdef STM32H743xx

#define START_EEPROM_ADDR 0x50

#define END_EEPROM_ADDR(idx) START_EEPROM_ADDR + idx * sizeof(uint32_t)
#define EEPROMSENSORBOARD(idx)                                                                                         \
    [EEPROM_SENSOR_BOARD_##idx] = {                                                                                    \
        "sensorBoard" #idx, START_EEPROM_ADDR + idx * sizeof(uint32_t), sizeof(uint32_t), "uint32_t", NULL, NULL}

const EepromRegistersInfo_t eepromRegisterLookupTable[NUM_EEPROM_REGISTERS] = {
    [EEPROM_INIT_KEY] = {"init_key", 0x00, 4, "uint32_t", NULL, NULL},
    [EEPROM_SERIAL_NUMBER] = {"serialNumber", 0x4, 16, "string", NULL, NULL},
    [EEPROM_HW_TYPE] = {"hardwareType", 0x14, 4, "uint32_t", NULL, NULL},
    [EEPROM_HW_VERSION] = {"hardwareVersion", 0x18, 4, "uint32_t", NULL, NULL},
    [EEPROM_IP_ADDR] = {"ip_address", 0x1C, 4, "dotteddec", NULL, NULL},
    [EEPROM_HTTP_PORT] = {"http_port", 0x20, 4, "uint32_t", NULL, NULL},
    [EEPROM_UDP_TX_PORT] = {"udp_tx_port", 0x24, 4, "uint32_t", NULL, NULL},
    [EEPROM_GATEWAY] = {"gateway", 0x28, 4, "dotteddec", NULL, NULL},
    [EEPROM_NETMASK] = {"netmask", 0x2C, 4, "dotteddec", NULL, NULL},
    [EEPROM_SERVER_UDP_IP] = {"server_UDP_ip", 0x30, 4, "dotteddec", NULL, NULL},
    [EEPROM_SERVER_UPD_PORT] = {"server_UDP_port", 0x34, 4, "uint32_t", NULL, NULL},
    [EEPROM_CLIENT_TCP_IP] = {"client_TCP_ip", 0x38, 4, "dotteddec", NULL, NULL},
    [EEPROM_CLIENT_TCP_PORT] = {"cient_TCP_port", 0x3C, 4, "uint32_t", NULL, NULL},
    [EEPROM_IP_TX_DATA_TYPE] = {"tx_data_type", 0x40, 4, "uint32_t", NULL, NULL},
    [EEPROM_UNIQUE_ID] = {"uniqueID", 0x48, 4, "uint32_t", NULL, NULL},
    [EEPROM_ADC_MODE] = {"adcMode", 0x4C, 4, "uint32_t", NULL, NULL},
    EEPROMSENSORBOARD(0),
    EEPROMSENSORBOARD(1),
    EEPROMSENSORBOARD(2),
    EEPROMSENSORBOARD(3),
    EEPROMSENSORBOARD(4),
    EEPROMSENSORBOARD(5),
    EEPROMSENSORBOARD(6),
    EEPROMSENSORBOARD(7),
    EEPROMSENSORBOARD(8),
    EEPROMSENSORBOARD(9),
    EEPROMSENSORBOARD(10),
    EEPROMSENSORBOARD(11),
    EEPROMSENSORBOARD(12),
    EEPROMSENSORBOARD(13),
    EEPROMSENSORBOARD(14),
    EEPROMSENSORBOARD(15),
    EEPROMSENSORBOARD(16),
    EEPROMSENSORBOARD(17),
    EEPROMSENSORBOARD(18),
    EEPROMSENSORBOARD(19),
    EEPROMSENSORBOARD(20),
    EEPROMSENSORBOARD(21),
    EEPROMSENSORBOARD(22),
    EEPROMSENSORBOARD(23), // 0x50+23*4 = 0xAC (172)
    [EEPROM_FAN_POP] = {"fanPop", 0xB0, 4, "uint32_t", NULL, NULL},
};

// clang-format off
#define EEPROM_KEY_VALUE 0x5b5aa5a5
// Factory default settings.
uint8_t EEPROM_FACTORY_SETTINGS[END_EEPROM_ADDR(MAX_CS_ID)] = {
    0xa5, 0xa5, 0x5a, 0x5b, // EEPROM KEY VALUE
    's', 'e', 'r', 'i', /********************************/
    'a', 'l', ' ', 'n', /* serial number 16 characters  */
    'u', 'm', 'b', 'e', /*                              */
    'r', 0,  0,  0,     /********************************/
    0, 0, 0, 0x80, // Hardware Type
    2, 0, 0, 0, // Hardware Version
    192, 168, 99, 11, // local ip address
    0x41, 0x1F, 0, 0, // 8001 web server http port
    0xc3, 0x3c, 0, 0, // 15555 udp TX port
    192, 168, 99, 1, // gateway addr
    255, 255, 255, 0, // netmask
    192, 168, 99, 1, // udp server ip address
    0x8D, 0x13, 0, 0, // 5005 udp server port
    192, 168, 99, 1, // TCP server ip address
    0x8D, 0x14, 0, 0, // 5006 TCP server port
    0, 0, 0, 0, // IP_TX_DATA TYPE 0=UDP, 1=TCP
    0xFF,0xFF,0xFF,0xFF, // Unique Id
    0,0,0,0, // ADC MODE Single shot=0, Continuous=1
    0,0,0,0, // EEPROM 0 MCG
    0,0,0,0, // EEPROM 1 MCG
    0,0,0,0, // EEPROM 2 MCG
    0,0,0,0, // EEPROM 3 MCG
    0,0,0,0, // EEPROM 4 MCG
    0,0,0,0, // EEPROM 5 MCG
    0,0,0,0, // EEPROM 6 MCG
    0,0,0,0, // EEPROM 7 MCG
    0,0,0,0, // EEPROM 8 MCG
    0,0,0,0, // EEPROM 9 MCG
    0,0,0,0, // EEPROM 10 MCG
    2,0,0,0, // EEPROM 11 COIL
    1,0,0,0, // EEPROM 12 ECG
    0,0,0,0, // EEPROM 13 MCG
    0,0,0,0, // EEPROM 14 MCG
    0,0,0,0, // EEPROM 15 MCG
    0,0,0,0, // EEPROM 16 MCG
    0,0,0,0, // EEPROM 17 MCG
    0,0,0,0, // EEPROM 18 MCG
    0,0,0,0, // EEPROM 19 MCG
    0,0,0,0, // EEPROM 20 MCG
    0,0,0,0, // EEPROM 21 MCG
    0,0,0,0, // EEPROM 22 MCG
    3,0,0,0, // EEPROM 23 EMPTY
    1,0,0,0, // FAN1 is populated, FAN2 is not

};

// clang-format on

#else
const EepromRegistersInfo_t eepromRegisterLookupTable[NUM_EEPROM_REGISTERS] = {
    [EEPROM_INIT_KEY] = {"init_key", 0x00, 4, "uint32_t", NULL, NULL},
    [EEPROM_SERIAL_NUMBER] = {"serialNumber", 0x4, 16, "string", NULL, NULL},
    [EEPROM_HW_TYPE] = {"hardwareType", 0x14, 4, "uint32_t", NULL, NULL},
    [EEPROM_HW_VERSION] = {"hardwareVersion", 0x18, 4, "uint32_t", NULL, NULL},
    [EEPROM_ADC_MODE] = {"adcMode", 0x1C, 4, "uint32_t", NULL, NULL},
    [EEPROM_IMU0_PRESENT] = {"imu0Present", 0x20, 4, "uint32_t", NULL, NULL},
    [EEPROM_IMU1_PRESENT] = {"imu1Present", 0x24, 4, "uint32_t", NULL, NULL},
    [EEPROM_IMU_BAUD] = {"imuBaud", 0x28, 4, "uint32_t", NULL, NULL},
};

// clang-format off
#define EEPROM_KEY_VALUE 0x5a5aa5a5
// Factory default settings.
uint8_t EEPROM_FACTORY_SETTINGS[sizeof(eepromRegisterLookupTable)] = {
    0xa5, 0xa5, 0x5a, 0x5a, // EEPROM KEY VALUE
    'b', 'l', ' ', 'n', // serial number 16 characters
    'u', 'm', 'b', 'e',
    'r', 0,  0,  0,
    0, 0, 0, 0, // end of 16 characters
    0, 0, 0, 0, // Hardware Type
    2, 0, 0, 0, // Hardware Version
    0, 0, 0, 0, // ADC MODE Single shot=0 Continuous=1
    1, 0, 0, 0, // IMU 0 Present
    1, 0, 0, 0, // IMU 1 Present
    0, 1, 0xc2, 0, // IMU BAUD 0x0007 0800 = 460800, 0001 C200 = 115200
};

// clang-format on

#endif
static osMutexId eepromMutexId = NULL;

/**
 * @fn testEepromMappings
 *
 * @brief Test that no eeprom locations overlap with one another.
 * @note test will assert if this is true.
 *
 **/
static void testEepromMappings(void) {

    for (int i = 0; i < NUM_EEPROM_REGISTERS; i++) {
        for (int j = 0; j < NUM_EEPROM_REGISTERS; j++) {
            uint32_t addrStart = eepromRegisterLookupTable[i].addr;
            uint32_t addrEnd = addrStart + eepromRegisterLookupTable[i].size - 1;

            if (i != j) {
                if (eepromRegisterLookupTable[j].addr >= addrStart && eepromRegisterLookupTable[j].addr <= addrEnd) {
                    DPRINTF_ERROR("element %s overlaps element %s\r\n",
                                  eepromRegisterLookupTable[j].name,
                                  eepromRegisterLookupTable[i].name);
                    // Give time for print to get written before asserting
                    osDelay(ASSERT_DELAY_MS);
                    assert(false);
                }
            }
        }
    }
    return;
}

void eepromInit(void) {
    uint8_t buffer[BUFFER_MAX];
    testEepromMappings();
    osMutexDef(eepromMutex);
    eepromMutexId = osMutexCreate(osMutex(eepromMutex));
    assert(eepromMutexId != NULL);

    if (EEPRMA2_M24_Init(EEPRMA2_M24M01_0) != BSP_ERROR_NONE) {
        assert(false);
    }
    if (EEPRMA2_M24_IsDeviceReady(EEPRMA2_M24M01_0, 1) != BSP_ERROR_NONE) {
        DPRINTF_ERROR("EEPROM is not available\r\n");
        g_eepromPresent = false;
        hardwareFailure(PER_EEPROM);
        return;
    }
    g_eepromPresent = true;
    eepromOpen(osWaitForever);
    eepromReadRegister(EEPROM_INIT_KEY, buffer, sizeof(buffer));
    uint32_t *p32 = (uint32_t *)buffer;
    if (*p32 != EEPROM_KEY_VALUE) {
        DPRINTF_INFO("Writing INIT EEPROM Structure to eeprom\r\n");
        if (EEPRMA2_M24_WriteData(EEPRMA2_M24M01_0, EEPROM_FACTORY_SETTINGS, 0, sizeof(EEPROM_FACTORY_SETTINGS)) != 0) {
            DPRINTF_ERROR("ERR: failed to write init data structure to eeprom\r\n");
        }
    }
    eepromClose();
}

uint32_t ipStringToInt(char *str) {
    uint32_t ipAsInt = 0;
    unsigned int byte3 = 0;
    unsigned int byte2 = 0;
    unsigned int byte1 = 0;
    unsigned int byte0 = 0;

    if (sscanf(str, "%u.%u.%u.%u", &byte3, &byte2, &byte1, &byte0) == 4) {
        if ((byte3 < 256) && (byte2 < 256) && (byte1 < 256) && (byte0 < 256)) {
            ipAsInt = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + byte0;
        }
    }

    return ipAsInt;
}

int32_t getRegisterFromName(char *name) {
    int32_t registerNotFound = -1;
    for (int i = 0; i < NUM_EEPROM_REGISTERS; i++) {
        if (strcmp(name, eepromRegisterLookupTable[i].name) == 0) {
            return i;
        }
    }
    return registerNotFound;
}

osStatus eepromOpen(uint32_t wait_ms) {
    return osMutexWait(eepromMutexId, wait_ms);
}
osStatus eepromClose(void) {
    return osMutexRelease(eepromMutexId);
}

osStatus eepromReadRegister(EepromRegisters_e idx, uint8_t *const pData, size_t pDataSz) {
    assert(pData != NULL);
    if (idx > NUM_EEPROM_REGISTERS) {
        DPRINTF_ERROR("Eeprom Register idx %d out of range\r\n", idx);
        return osErrorValue;
    }
    if (pDataSz < eepromRegisterLookupTable[idx].size) {
        DPRINTF_ERROR("pData size %d is too small for idx %d\r\n", pDataSz, idx);
        return osErrorValue;
    }

    if (!g_eepromPresent) {
        uint8_t *ptr = (uint8_t *)EEPROM_FACTORY_SETTINGS + eepromRegisterLookupTable[idx].addr;
        size_t sz = eepromRegisterLookupTable[idx].size;
        memcpy(pData, ptr, sz);
        return osOK;
    }
    osStatus rc;
    if ((rc = EEPRMA2_M24_ReadData(
             EEPRMA2_M24M01_0, pData, eepromRegisterLookupTable[idx].addr, eepromRegisterLookupTable[idx].size)) !=
        osOK) {
        DPRINTF_ERROR(
            "EEPROM READ idx %d at addr %x failed with rc=%d\r\n", idx, eepromRegisterLookupTable[idx].addr, rc);
        return osErrorOS;
    }
    return osOK;
}

osStatus eepromWriteRegister(EepromRegisters_e idx, uint8_t *const pData, size_t pDataSz) {
    assert(pData != NULL);
    if (idx > NUM_EEPROM_REGISTERS) {
        DPRINTF_ERROR("Eeprom Register idx %d out of range\r\n", idx);
        return osErrorValue;
    }
    if (pDataSz > eepromRegisterLookupTable[idx].size) {
        DPRINTF_ERROR("pData size %d is too large for idx %d\r\n", pDataSz, idx);
        return osErrorValue;
    }

    if (!g_eepromPresent) {
        uint8_t *ptr = (uint8_t *)&EEPROM_FACTORY_SETTINGS + eepromRegisterLookupTable[idx].addr;
        size_t sz = eepromRegisterLookupTable[idx].size;
        memcpy(ptr, pData, sz);
        return osOK;
    }

    if (EEPRMA2_M24_WriteData(
            EEPRMA2_M24M01_0, pData, eepromRegisterLookupTable[idx].addr, eepromRegisterLookupTable[idx].size) != 0) {
        return osErrorOS;
    }

    return osOK;
}
