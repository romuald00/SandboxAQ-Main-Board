/**
 * @file
 * Log utility functions.
 *
 * A logging function that currently wraps Linux syslog functionality.
 * This component is intended to be used and called by one other
 * component.  Multiple components (processes) will have their own
 * instance of this logger to use.
 *
 * Copyright Nuvation Research Corporation 2013-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_LOG_H_
#define NUVC_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/**
 * The logging severity.
 * The mapping between logging facility priorities and syslog priorities
 * arranged from highest to lowest priority.
 * Note that these definitions map directly to syslog priorities.
 * All logs are sent to /var/log/syslog as well as what is noted below.
 */
typedef enum {
    /** Emergency - System is unusable sent to /var/log/user.log */
    LOG_LEVEL_EMERG,
    /** Alert - Not an error, but action must be taken immediately sent to
     * /var/log/user.log. */
    LOG_LEVEL_ALERT,
    /** Critical - Critical conditions, these are severe errors that could lead
     * to emergencies sent to /var/log/user.log. */
    LOG_LEVEL_CRIT,
    /** Error - Error conditions, something bad happened but the system is
     * still working sent to /var/log/user.log. */
    LOG_LEVEL_ERR,
    /** Warning - Warning conditions, abnormal conditions that could lead to
     * errors or be an error under certain conditions sent to
     * /var/log/messages. */
    LOG_LEVEL_WARNING,
    /** Notice - A normal but significant condition sent to /var/log/messages.
     */
    LOG_LEVEL_NOTICE,
    /** Information - such as the system booted sent to /var/log/messages. */
    LOG_LEVEL_INFO,
    /** Debug - Debug-level message sent to /var/log/debug. */
    LOG_LEVEL_DEBUG,
    /** Used internally. */
    LOG_LEVEL_MAX
} LOG_LEVEL_t;

/* Macros for logging that should generally be used instead of LOG_Write(). */
#ifdef NDEBUG
#define LOG_EMERG_MSG(...)
#define LOG_ALERT_MSG(...)
#define LOG_CRIT_MSG(...)
#define LOG_ERR_MSG(...)
#define LOG_WARNING_MSG(...)
#define LOG_NOTICE_MSG(...)
#define LOG_INFO_MSG(...)
#define LOG_DBG_MSG(...)

#define LOG_CRIT(f, ...)
#define LOG_ERR(f, ...)
#define LOG_WARN(f, ...)
#define LOG_INFO(f, ...)
#define LOG_DBG(f, ...)
#else
#define LOG_EMERG_MSG(f, ...) LOG_Write(LOG_LEVEL_EMERG, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_ALERT_MSG(f, ...) LOG_Write(LOG_LEVEL_ALERT, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_CRIT_MSG(f, ...) LOG_Write(LOG_LEVEL_CRIT, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_ERR_MSG(f, ...) LOG_Write(LOG_LEVEL_ERR, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_WARNING_MSG(f, ...) LOG_Write(LOG_LEVEL_WARNING, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_NOTICE_MSG(f, ...) LOG_Write(LOG_LEVEL_NOTICE, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_INFO_MSG(f, ...) LOG_Write(LOG_LEVEL_INFO, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_DBG_MSG(f, ...) LOG_Write(LOG_LEVEL_DEBUG, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)

#define LOG_CRIT(f, ...) LOG_Write(LOG_LEVEL_CRIT, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_ERR(f, ...) LOG_Write(LOG_LEVEL_ERR, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(f, ...) LOG_Write(LOG_LEVEL_WARNING, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(f, ...) LOG_Write(LOG_LEVEL_INFO, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#define LOG_DBG(f, ...) LOG_Write(LOG_LEVEL_DEBUG, __FILE__ "(%d): " f, __LINE__, ##__VA_ARGS__)
#endif /* ifdef NDEBUG */

void LOG_Init(const char *ident);
void LOG_Write(LOG_LEVEL_t, const char *format, ...) __attribute__((format(printf, 2, 3)));
void LOG_SetLevelLimit(LOG_LEVEL_t);
void LOG_Close();
void LOG_PutString(const char *s);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* NUVC_LOG_H_ */
