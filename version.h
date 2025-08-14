/*
 * version.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 5, 2023
 *      Author: rlegault
 */

#ifndef APP_INC_VERSION_H_
#define APP_INC_VERSION_H_

#define MACRO_FW_VER_MAJ 2
#define MACRO_FW_VER_MIN 5
#define MACRO_FW_VER_MAINT 1
#define MACRO_FW_VER_BUILD 0

// 2.1.0.0 includes a change to MB-DB interface, some commands send back a short update PASS/FAIL/TIMEOUT that
// doesn't result in a loss of data packet

// 2.2.0.0 altered SPI bus interaction to allow for large buffers to be sent over spi

// 2.2.0.2 the value of BOARDTYPE_EMPTY has changed from 3 to 7. Need to update all empty values to 7.
// run url  /dbg/setTargetToEmpty  json={"uid":"111", "target":"3"}

// 2.3.0.0 A new register was added on sensor board that protects the eeprom from being overwritten accidentally

// 2.3.0.1 Increased wait time for SB preparation of stat information from 7 to 10msec
// added ECG12 web commands and support

// 2.4.0.0 Fixed CNC_ACTION_e were conflated with CNC_CMD_e and they were out of sync

// 2.5.0.0 Enlarged SPI packet to support retrieving coil settings information
//         Altered and Enlarged Ethernet packet to support reporting onf coil setting information

#endif /* APP_INC_VERSION_H_ */
