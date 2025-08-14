/*
 * gpioMB.c
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 15, 2024
 *      Author: rlegault
 *
 */

#define CREATE_GPIOMB_STRING_LOOKUP_TABLE
#include "gpioMB.h"
#undef CREATE_GPIOMB_STRING_LOOKUP_TABLE

#include "cli/cli.h"
#include "cli/cli_print.h"
#include "ctrlSpiCommTask.h"
#include "debugPrint.h"
#include <stdlib.h>
#include <string.h>

#define GPIO_CMD_IDX 1
#define GPIO_CMD_PORTPIN_IDX 2
#define GPIO_PORT_CHAR_IDX 0
#define GPIO_CMD_PIN_STATE_IDX 3
#define MAX_PIN_VALUE 0xF

#define GPIO_CMD_PINNAME_IDX 2
#define GPIO_CMD_PINNAME_STATE_IDX 3

// Struct for pin config mapping
typedef struct {
    gpioMBPin_e pin;
    gpioPort_e port;
    uint16_t pinNumber;
    gpioType_e gpioType;
} gpioPinConfig_t;

const gpioPinConfig_t mbPinMap[] = {
    // MCU_PIN Board_Rev
    {.pin = DBG_0, .port = GPIO_PORT_B_ENUM, .pinNumber = GPIO_PIN_8, .gpioType = GPIO_MODE_OUTPUT_PP}, // B0 R2
    {.pin = DBG_1, .port = GPIO_PORT_B_ENUM, .pinNumber = GPIO_PIN_9, .gpioType = GPIO_MODE_OUTPUT_PP}, // B1 R2
    {.pin = DBG_2, .port = GPIO_PORT_E_ENUM, .pinNumber = GPIO_PIN_0, .gpioType = GPIO_MODE_OUTPUT_PP}, // E0 R2
    {.pin = DBG_3, .port = GPIO_PORT_E_ENUM, .pinNumber = GPIO_PIN_1, .gpioType = GPIO_MODE_OUTPUT_PP}, // E1 R2
    {.pin = DBG_4, .port = GPIO_PORT_F_ENUM, .pinNumber = GPIO_PIN_2, .gpioType = GPIO_MODE_OUTPUT_PP}, // F2 R2
    {.pin = DBG_5, .port = GPIO_PORT_F_ENUM, .pinNumber = GPIO_PIN_3, .gpioType = GPIO_MODE_OUTPUT_PP}, // F3 R2
    {.pin = DBG_6, .port = GPIO_PORT_F_ENUM, .pinNumber = GPIO_PIN_4, .gpioType = GPIO_MODE_OUTPUT_PP}, // F4 R2

    {.pin = DBG_7_DAC, .port = GPIO_PORT_A_ENUM, .pinNumber = GPIO_PIN_4, .gpioType = GPIO_MODE_OUTPUT_PP},  // A4 R2
    {.pin = PWR_SW_EN, .port = GPIO_PORT_D_ENUM, .pinNumber = GPIO_PIN_10, .gpioType = GPIO_MODE_OUTPUT_PP}, // D10 R3
    {.pin = PWR_SW_DIA_EN, .port = GPIO_PORT_D_ENUM, .pinNumber = GPIO_PIN_0, .gpioType = GPIO_MODE_OUTPUT_PP}, // D0 R3
    {.pin = PWR_SW_SEL2, .port = GPIO_PORT_D_ENUM, .pinNumber = GPIO_PIN_1, .gpioType = GPIO_MODE_OUTPUT_PP},   // D1 R3
    {.pin = PWR_SW_SEL1, .port = GPIO_PORT_D_ENUM, .pinNumber = GPIO_PIN_2, .gpioType = GPIO_MODE_OUTPUT_PP},   // D2 R3
    {.pin = PWR_SW_LATCH, .port = GPIO_PORT_D_ENUM, .pinNumber = GPIO_PIN_8, .gpioType = GPIO_MODE_OUTPUT_PP},  // D8 R3
    {.pin = PWR_SW_STATUS, .port = GPIO_PORT_D_ENUM, .pinNumber = GPIO_PIN_9, .gpioType = GPIO_MODE_INPUT},     // D9 R3
    {.pin = REF_CLK_EN, .port = GPIO_PORT_F_ENUM, .pinNumber = GPIO_PIN_9, .gpioType = GPIO_MODE_OUTPUT_PP},    // F9 R3
    {.pin = SPARE1, .port = GPIO_PORT_D_ENUM, .pinNumber = GPIO_PIN_12, .gpioType = GPIO_MODE_OUTPUT_PP}, // D12 R3
};

/**
 * @fn _gpioGetPort
 *
 * @brief Get GPIO port given GPIO port enum
 *
 * @param[in] gpioPort_e gpioPort: GPIO pin enum
 *
 * @return GPIO_TypeDef
 **/
GPIO_TypeDef *_gpioGetPort(gpioPort_e gpioPort) {

    switch (gpioPort) {
    case GPIO_PORT_A_ENUM:
        return GPIOA;
    case GPIO_PORT_B_ENUM:
        return GPIOB;
    case GPIO_PORT_C_ENUM:
        return GPIOC;
    case GPIO_PORT_D_ENUM:
        return GPIOD;
    case GPIO_PORT_E_ENUM:
        return GPIOE;
    case GPIO_PORT_F_ENUM:
        return GPIOF;
    case GPIO_PORT_G_ENUM:
        return GPIOG;
    case GPIO_PORT_H_ENUM:
        return GPIOH;
    default:
        return NULL;
    }

    return NULL;
}

/**
 * @fn _gpioGetPortByPin
 *
 * @brief Get GPIO port given pin enum
 *
 * @param[in] gpioPin_t gpioPin: GPIO pin enum
 *
 * @return GPIO_TypeDef
 **/
GPIO_TypeDef *_gpioGetPortByPin(gpioMBPin_e gpioPin) {
    if (mbPinMap[gpioPin].pin != gpioPin) {
        return NULL;
    }
    return _gpioGetPort(mbPinMap[gpioPin].port);
}

int16_t gpioCommand(CLI *hCli, int argc, char *argv[]) {
    // gpio read a 1
    if ((argc == (GPIO_CMD_PORTPIN_IDX + 1)) && (strcmp(argv[GPIO_CMD_IDX], "getPin") == 0)) {
        GPIO_TypeDef *gpioPort;
        char portC = argv[GPIO_CMD_PORTPIN_IDX][GPIO_PORT_CHAR_IDX];
        switch (portC) {
        case 'A':
        case 'a':
            gpioPort = _gpioGetPort(GPIO_PORT_A_ENUM);
            break;
        case 'B':
        case 'b':
            gpioPort = _gpioGetPort(GPIO_PORT_B_ENUM);
            break;
        case 'C':
        case 'c':
            gpioPort = _gpioGetPort(GPIO_PORT_C_ENUM);
            break;
        case 'D':
        case 'd':
            gpioPort = _gpioGetPort(GPIO_PORT_D_ENUM);
            break;
        case 'E':
        case 'e':
            gpioPort = _gpioGetPort(GPIO_PORT_E_ENUM);
            break;

        case 'F':
        case 'f':
            gpioPort = _gpioGetPort(GPIO_PORT_F_ENUM);
            break;

        case 'G':
        case 'g':
            gpioPort = _gpioGetPort(GPIO_PORT_G_ENUM);
            break;

        case 'H':
        case 'h':
            gpioPort = _gpioGetPort(GPIO_PORT_H_ENUM);
            break;

        default:
            return CLI_RESULT_ERROR;
        }
        int pinV = atoi(argv[GPIO_CMD_PORTPIN_IDX] + 1); // move past the alpha value
        if (pinV > MAX_PIN_VALUE) {
            return CLI_RESULT_ERROR;
        }

        int state = HAL_GPIO_ReadPin(gpioPort, 1 << pinV);
        CliPrintf(hCli, "P%c%d=%d\r\n", portC, pinV, state);
        return CLI_RESULT_SUCCESS;
    } else if ((argc == (GPIO_CMD_PIN_STATE_IDX + 1)) && (strcmp(argv[GPIO_CMD_IDX], "setPin") == 0)) {
        // gpio write a 1 1
        GPIO_TypeDef *gpioPort;
        char portC = argv[GPIO_CMD_PORTPIN_IDX][GPIO_PORT_CHAR_IDX];
        switch (portC) {
        case 'A':
        case 'a':
            gpioPort = _gpioGetPort(GPIO_PORT_A_ENUM);
            break;
        case 'B':
        case 'b':
            gpioPort = _gpioGetPort(GPIO_PORT_B_ENUM);
            break;
        case 'C':
        case 'c':
            gpioPort = _gpioGetPort(GPIO_PORT_C_ENUM);
            break;
        case 'D':
        case 'd':
            gpioPort = _gpioGetPort(GPIO_PORT_D_ENUM);
            break;
        case 'E':
        case 'e':
            gpioPort = _gpioGetPort(GPIO_PORT_E_ENUM);
            break;
        case 'F':
        case 'f':
            gpioPort = _gpioGetPort(GPIO_PORT_F_ENUM);
            break;
        case 'G':
        case 'g':
            gpioPort = _gpioGetPort(GPIO_PORT_G_ENUM);
            break;
        case 'H':
        case 'h':
            gpioPort = _gpioGetPort(GPIO_PORT_H_ENUM);
            break;

        default:
            return CLI_RESULT_ERROR;
        }
        int pinV = atoi(argv[GPIO_CMD_PORTPIN_IDX] + 1); // 1 to skip the port alpha value

        if (pinV > MAX_PIN_VALUE) {
            return CLI_RESULT_ERROR;
        }

        gpioState_e pinState = GPIO_STATE_HIGH;
        if (atoi(argv[GPIO_CMD_PIN_STATE_IDX]) == 0) {
            pinState = GPIO_STATE_LOW;
        }

        HAL_GPIO_WritePin(gpioPort, 1 << pinV, pinState);
        CliPrintf(hCli, "P%c%d=%d\r\n", portC, pinV, pinState);
        return CLI_RESULT_SUCCESS;

    } else if ((argc == (GPIO_CMD_IDX + 1)) && (strcmp(argv[GPIO_CMD_IDX], "list") == 0)) {
        // Chip Select list
        CliPrintf(hCli, "Pin List\r\n");
        for (int spi = 0; spi < MAX_SPI; spi++) {
            for (int cardBySpi = 0; cardBySpi < MAX_CS_PER_SPI; cardBySpi++) {
                uint32_t idx = cardBySpi + (spi * MAX_CS_PER_SPI);
                CliPrintf(hCli, "SENSOR %d ChipSelect %s\r\n", idx, csName(idx));
            }
        }
        return CLI_RESULT_SUCCESS;
    }
    return CLI_RESULT_SUCCESS;
}

osStatus gpioGetInput(gpioMBPin_e gpioPin, gpioState_e *pinValue) {
    if (pinValue == NULL) {
        DPRINTF_LED("NULL value received/n/r");
        return osErrorParameter;
    }

    GPIO_TypeDef *port = _gpioGetPortByPin(gpioPin);

    if (port == NULL) {
        DPRINTF_LED("Could not get GPIO port/n/r");
        return osErrorParameter;
    }

    if (HAL_GPIO_ReadPin(port, mbPinMap[gpioPin].pinNumber) == GPIO_PIN_SET) {
        *pinValue = GPIO_STATE_HIGH;
    } else {
        *pinValue = GPIO_STATE_LOW;
    }

    return osOK;
}

osStatus gpioSetOutput(gpioMBPin_e gpioPin, gpioState_e pinState) {
    GPIO_TypeDef *port = _gpioGetPortByPin(gpioPin);
    if (port == NULL) {
        DPRINTF_LED("Could not get GPIO port/n/r");
        return osErrorParameter;
    }

    HAL_GPIO_WritePin(port, mbPinMap[gpioPin].pinNumber, pinState);

    return osOK;
}

osStatus gpioToggle(gpioMBPin_e gpioPin) {
    GPIO_TypeDef *port = _gpioGetPortByPin(gpioPin);
    if (port == NULL) {
        DPRINTF_LED("Could not get GPIO port/n/r");
        return osErrorParameter;
    }
    HAL_GPIO_TogglePin(port, mbPinMap[gpioPin].pinNumber);

    return osOK;
}
