/**
 * @file
 *
 * Definition for simulated configuration functions.
 *
 * Copyright Nuvation Research Corporation 2014-2018. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_SIM_CONFIG_H_
#define NUVC_SIM_CONFIG_H_

/**
 * Callback when a CLI option, argument combo is parsed.
 *
 * @param[in] c     option character that triggered the callback
 * @param[in] arg   argument supplied to the option
 * @return          must be zero for now
 */
typedef int (*SimArgCallback)(int c, const char *arg);

/**
 * Stores argc and argv from main for later use.
 *
 * These are stored so that simulated drivers can parse the arguments at their
 * own leisure.
 *
 * @param[in] argc  the argc value to remember
 * @param[in] argv  the argv value to remember
 */
extern void SimStoreArgs(int argc, const char *const *argv);

/**
 * Retrieve the stored arguments.
 *
 * This function basically returns the values previous set through the
 * SimStoreArgs function.
 *
 * @param[out] argc     the argc value previously stored
 * @param[out] argv     the argv value previously stored
 */
extern void SimGetArgs(int *argc, const char *const **argv);

/**
 * Parses arguments previously supplied.
 *
 * The options are parsed using the standard getopt function. It is
 * recommended that you start your string with a colon to avoid printing
 * unnecessary errors.
 *
 * @param[in] callback  points to a function to accept option and argument
 * @param[in] options   short options to look for
 */
extern void SimParseArgs(SimArgCallback callback, const char *options);

#endif /* NUVC_SIM_CONFIG_H_ */
