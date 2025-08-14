/**
 *  @file
 *  Portable Command Line Interface Library
 *
 *  Header file for the portable command line interface's printing helper
 *  functions.
 *
 *  Copyright Nuvation Research Corporation 2006-2018. All Rights Reserved.
 *  www.nuvation.com
 */

#ifndef NUVC_CLI_PRINT_H_
#define NUVC_CLI_PRINT_H_

#include "cli/cli.h"
#include <inttypes.h>

typedef struct {
    const char *key;
    const char *value;
} CliKeyValuePairs;

extern const char returnCodeName[];
extern const char bytesRead[];
extern const char bytesWritten[];
extern const char bytesTransferred[];
extern const char outputBytes[];

/**
 * Writes the formatted string to the CLI's registered output function.
 *
 * @param[in] hCli
 *      the current CLI instance
 *
 * @param[in] format
 *      A printf-like format string.
 *
 * @param[in] ...
 *      Additional arguments used in rendering the format string.
 *
 * @return
 *      The number of characters output.
 */
void CliPrintf(CLI *hCli, const char *format, ...);

/**
 * Output a list of key value pairs to the CLI
 *
 * @param[in] hCli
 *      the current CLI instance
 *
 * @param[in]  pairs   List of value pairs to output.  Last entry should have
 *                     NULL key to terminate the list.
 */
void CliPrintKeyValuePairs(CLI *hCli, CliKeyValuePairs *pairs);

/**
 * Print a name-value pair in robot-parseable format.
 * @param[in] hCli
 *      the current CLI instance
 * @param[in] name      the name to print
 * @param[in] number    the value to print
 */
void CliPrintNameAndNumber(CLI *hCli, const char *name, int number);

/**
 * Print a name_index-value pair in robot-parseable format.
 * @param[in] hCli
 *      the current CLI instance
 * @param[in] name      the name to print
 * @param[in] index     the index to print with the name
 * @param[in] number    the value to print
 */
void CliPrintNameIndexAndNumber(CLI *hCli, const char *name, int index, int number);

/**
 * Print a name-value pair in robot-parseable format.
 *
 * @param[in] hCli
 *      the current CLI instance
 * @param[in] name      the name to print
 * @param[in] number    the 32-bit value to print
 */
void CliPrintNameAnd32BitNumber(CLI *hCli, const char *name, uint32_t number);

/**
 * Print a name_index-value pair in robot-parseable format.
 *
 * @param[in] hCli
 *      the current CLI instance
 * @param[in] name      the name to print
 * @param[in] index     the index to print with the name
 * @param[in] number    the 32-bit value to print
 */
void CliPrintIndexNameAnd32BitNumber(CLI *hCli, const char *name, int index, uint32_t number);

/**
 * Print a name-value pair in robot-parseable format.
 *
 * @param[in] hCli
 *      the current CLI instance
 * @param[in] name      the name to print
 * @param[in] number    the value to print
 */
void CliPrintNameAndString(CLI *hCli, const char *name, const char *format, ...);

/**
 * Prints a special tag in robot-parseable format.
 *
 * @param[in] hCli
 *      the current CLI instance
 * @param[in] name  the name to print
 */
void CliPrintSpecialName(CLI *hCli, const char *name);

#endif /* NUVC_CLI_PRINT_H_ */
