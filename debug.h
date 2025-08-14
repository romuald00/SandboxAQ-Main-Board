/**
 * @file
 * Definition of the debug API.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_DEBUG_H_
#define NUVC_DEBUG_H_

typedef enum { ERR = 0, WARN, LOG, NUM_DEBUG_LEVELS } DebugLevel;

/**
 * Outputs a debug log message to stdout (*see below) along with the file and
 * line
 * number identifying the location of the message in the source code. Called as
 * if it were a function.
 *
 * Example usage: DBG(LOG, "myInt has value: %d", myInt);
 *
 * *In the case of the simulator output will be redirected to a FILE stream if
 *  it is invoked with the appropriate -f argument.
 *
 * Implementation Notes:
 *  - The if(DEBUG_ENABLED) is a test against a constant known at compile time.
 *    This means the code will be optimized out when DEBUG_ENABLED = 0, but
 *    with the advantage that the code is still evaluated by the compiler.
 *
 * @param[in] LVL       The debug level of the log message. When the
 * DEBUG_LEVEL
 *                      definition is set to a level below that of the message,
 *                      the message will not be printed.
 * @param[in] STRING    The debug log message to print
 *                      (uses printf() formatting)
 * @param[in] ...       Optional formatting arguments for printf(). The
 *                      available options depend on the implementation of
 *                      printf() on the target platform.
 */
#define DBG(LVL, ...)                                                                                                  \
    do {                                                                                                               \
        if (DEBUG_ENABLED) {                                                                                           \
            if (LVL <= DEBUG_LEVEL) {                                                                                  \
                PrintDebug(LVL, __FILE__, __LINE__, __VA_ARGS__);                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/**
 * Platform specific configuration:
 * toggle this to 0 to remove debug logging from the code.
 */
#define DEBUG_ENABLED 1
/**
 * Platform specific configuration:
 * controls the verbosity of the debug output (if it is enabled).
 */
#define DEBUG_LEVEL LOG

/**
 * Prints a debug message to the appropriate destination.
 */
extern void PrintDebug(DebugLevel lvl, const char *file, int line, const char *format, ...);

#endif /* NUVC_DEBUG_H_ */
