/**
 *  @file
 *  Portable Command Line Interface Library
 *
 *  Header file for the portable command line interface's uart interface.
 *
 * Copyright Nuvation Research Corporation 2013-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_CLI_UART_H_
#define NUVC_CLI_UART_H_

#include "nuv_fsm.h"
#include <stdint.h>

extern FSM_STATE_MACHINE fsmCLI;

/**
 * Defines the prototype of the function used to send an arbitrary buffer of
 * data over the UART used to implement the CLI.
 *
 * Must be implemented in a seperate module with a target specific UART driver.
 *
 * @param[in] channel   over which UART the data is to be sent
 * @param[in] data      points to the data to be sent
 * @param[in] size      holds the size of the data in bytes
 * @return              number of bytes written
 */
int CliWriteOnUart(int channel, const void *data, size_t size);

/**
 * Defines the prototype of the function used to send a single byte of data
 * over the UART used to implement the CLI.
 *
 * Must be implemented in a seperate module with a target specific UART driver.
 *
 * @param[in] channel   over which UART the data is to be sent
 * @param[in] data      holds to the byte to be sent
 * @return              number of bytes written
 */
int CliWriteByteOnUart(int channel, uint_least8_t data);

#endif /* NUVC_CLI_UART_H_ */
