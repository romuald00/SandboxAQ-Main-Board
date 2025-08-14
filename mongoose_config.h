/*
 * mongoose_custom.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 21, 2023
 *      Author: rlegault
 */

#ifndef WEBSERVER_MONGOOSE_SRC_MONGOOSE_CUSTOM_H_
#define WEBSERVER_MONGOOSE_SRC_MONGOOSE_CUSTOM_H_

#include <stdarg.h>
#include <stdbool.h>

//#define MG_ARCH MG_ARCH_CMSIS_RTOS1
#define MG_ARCH MG_ARCH_FREERTOS
#define MG_ENABLE_FILE 0
#define MG_IO_SIZE 256
#define MG_ENABLE_LWIP 1

#endif /* WEBSERVER_MONGOOSE_SRC_MONGOOSE_CUSTOM_H_ */
