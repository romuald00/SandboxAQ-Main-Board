/**
 * @file
 * This file defines the debug printing library.
 *
 * Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef __DEBUG_PRINT_H__
#define __DEBUG_PRINT_H__

#include "debugPrintSettings.h"
#include <stdbool.h>
#include <stdio.h>
// clang-format off
#define DEBUG_PRINT_WAIT 2000

#if DPRINTF_USE_RAMLOG == 1
    #include "ramLog.h"
#endif

// VTrace

#define SPI_VTRACEPRINTF_VERBOSE 1

// The following series of defines are used to separate printing into separate groups.
// Each group can have a unique prefix and can be turned on and off individually.
// To enable a group. Add "#define DEBUG_<TYPE>_PRINTING == 1" to "debugPrintSettings.h"

#define DPRINTF(...) DPRINTF_TYPE("DEBUG", __VA_ARGS__)
#define DPRINTF_INFO(...) DPRINTF_FULL_TYPE("INFO", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DPRINTF_WARN(...) DPRINTF_FULL_TYPE("WARNING", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DPRINTF_ERROR(...) DPRINTF_FULL_TYPE("ERROR", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DPRINTF_VAR_INT(var) DPRINTF("%s = %d\r\n", #var, var)
#define DPRINTF_VAR_FLOAT(var) DPRINTF("%s = %04f\r\n", #var, var)
#define DPRINTF_VAR_STRING(var) DPRINTF("%s = %s\r\n", #var, var)


#if DEBUG_SPI_PRINTING == 1
    #define DPRINTF_SPI(...) DPRINTF_TYPE("SPI", __VA_ARGS__)
#else
    #define DPRINTF_SPI(...) {}
#endif

#if DEBUG_SETTINGS_PRINTING == 1
    #define DPRINTF_SETTINGS(...) DPRINTF_TYPE("SET", __VA_ARGS__)
#else
    #define DPRINTF_SETTINGS(...) {}
#endif


#if DEBUG_UART_PRINTING == 1
    #define DPRINTF_UART(...) DPRINTF_TYPE("UART", __VA_ARGS__)
#else
    #define DPRINTF_UART(...) {}
#endif


#if DEBUG_WEB_PRINTING == 1
    #define DPRINTF_WEB(...) DPRINTF_TYPE("WEB", __VA_ARGS__)
#else
    #define DPRINTF_WEB(...) {}
#endif

#if DEBUG_JSON_PRINTING == 1
    #define DPRINTF_JSON(...) DPRINTF_TYPE("JSON", __VA_ARGS__)
#else
    #define DPRINTF_JSON(...) {}
#endif


#if DEBUG_LED_PRINTING == 1
    #define DPRINTF_LED(...) DPRINTF_TYPE("LED", __VA_ARGS__)
#else
    #define DPRINTF_LED(...) {}
#endif


#if DEBUG_MAIN_PRINTING == 1
    #define DPRINTF_MAIN(...) DPRINTF_TYPE("MAIN", __VA_ARGS__)
#else
    #define DPRINTF_MAIN(...) {}
#endif

#if DEBUG_CLI_PRINTING == 1
    #define DPRINTF_CLI(...) DPRINTF_TYPE("CLI", __VA_ARGS__)
#else
    #define DPRINTF_CLI(...) {}
#endif


#if DEBUG_ADC_CAPTURE_PRINTING == 1
    #define DPRINTF_ADC_CAPTURE(...) DPRINTF_TYPE("ADC_CAPTURE", __VA_ARGS__)
#else
    #define DPRINTF_ADC_CAPTURE(...) {}
#endif

#if DEBUG_ADC_PRINTING == 1
    #define DPRINTF_ADC(...) DPRINTF_TYPE("ADC", __VA_ARGS__)
#else
    #define DPRINTF_ADC(...) {}
#endif

#if DEBUG_MAX57_PRINTING == 1
    #define DPRINTF_MAX57(...) DPRINTF_TYPE("M57", __VA_ARGS__)
#else
    #define DPRINTF_MAX57(...) {}
#endif

#if DEBUG_CNC_PRINTING == 1
    #define DPRINTF_CNC(...) DPRINTF_TYPE("CNC", __VA_ARGS__)
#else
    #define DPRINTF_CNC(...) {}
#endif

#if DEBUG_STACKQ_PRINTING == 1
    #define DPRINTF_STACKQ(...) DPRINTF_TYPE("STK_Q", __VA_ARGS__)
#else
    #define DPRINTF_STACKQ(...) {}
#endif

#if DEBUG_WORKER_COMM_PRINTING == 1
    #define DPRINTF_WCOMM(...) DPRINTF_TYPE("W_COMM", __VA_ARGS__)
#else
    #define DPRINTF_WCOMM(...) {}
#endif

#if DEBUG_CTRL_COMM_PRINTING == 1
    #define DPRINTF_CCOMM(...) DPRINTF_TYPE("C_COMM", __VA_ARGS__)
#else
    #define DPRINTF_CCOMM(...) {}
#endif

#if DEBUG_CMD_STREAM_PRINTING == 1
    #define DPRINTF_CMD_STREAM(...) DPRINTF_TYPE("CMD_STRM", __VA_ARGS__)
#else
    #define DPRINTF_CMD_STREAM(...) {}
#endif

#if DEBUG_DBCOMM_VERBOSE_PRINTING == 1
    #define DPRINTF_DBCOMM_VERBOSE(...) DPRINTF_TYPE("DBCOMM_VERB", __VA_ARGS__)
#else
    #define DPRINTF_DBCOMM_VERBOSE(...) {}
#endif

#if DEBUG_DBCOMM_PRINTING == 1
    #define DPRINTF_DBCOMM(...) DPRINTF_TYPE("DBCOMM", __VA_ARGS__)
#else
    #define DPRINTF_DBCOMM(...) {}
#endif

#if DEBUG_HTTPSERVER_PRINTING == 1
    #define DPRINTF_HTTP(...) DPRINTF_TYPE("HTTP", __VA_ARGS__)
#else
    #define DPRINTF_HTTP(...) {}
#endif

#if DEBUG_HTTP_POST_PRINTING == 1
    #define DPRINTF_POST(...) DPRINTF_TYPE("POST", __VA_ARGS__)
#else
    #define DPRINTF_POST(...) {}
#endif

#if DEBUG_GATH_PRINTING == 1
    #define DPRINTF_GATH(...) DPRINTF_TYPE("POST", __VA_ARGS__)
#else
    #define DPRINTF_GATH(...) {}
#endif

#if DEBUG_CMD_STREAM_VERBOSE_PRINTING == 1
    #define DPRINTF_CMD_STREAM_VERBOSE(...) DPRINTF_TYPE("CNC_V", __VA_ARGS__)
#else
    #define DPRINTF_CMD_STREAM_VERBOSE(...) {}
#endif

#if DEBUG_IMU_PRINTING == 1
    #define DPRINTF_IMU(...) DPRINTF_TYPE("IMU", __VA_ARGS__)
#else
    #define DPRINTF_IMU(...) {}
#endif

#if DEBUG_IMU_VERBOSE_PRINTING == 1
    #define DPRINTF_IMU_VERBOSE(...) DPRINTF_TYPE("IMU", __VA_ARGS__)
#else
    #define DPRINTF_IMU_VERBOSE(...) {}
#endif

#if DEBUG_IMU_CMD_VERBOSE_PRINTING == 1
    #define DPRINTF_IMU_CMD_VERBOSE(...) DPRINTF_TYPE("IMUCMD", __VA_ARGS__)
#else
    #define DPRINTF_IMU_CMD_VERBOSE(...) {}
#endif


#if DEBUG_IMU_RX_STATE_VERBOSE_PRINTING == 1
    #define DPRINTF_IMU_RX_STATE_VERBOSE(...) DPRINTF_TYPE("IMU_RX_STATE", __VA_ARGS__)
#else
    #define DPRINTF_IMU_RX_STATE_VERBOSE(...) {}
#endif



#if DPRINTF_RTOS == 1
    void debugPrintingInit(void); // Initialize the debug printing interface
    int32_t debugPrintingIsInit(void);
#endif

void minimalWrite(const char *ptr);
void safePrint(const char *str, ...);
void safePrintVA(const char *str, va_list args);

#if DEBUG_PRINTING == 1
    #if DPRINTF_RTOS == 1
        void dprintfType(const char *type, const char *str, ...);
        void dprintfRaw(const char *str, ...);
        void dprintfRawVA(const char *str, va_list args);
        void dprintfNolock(const char *str);
        void dprintfNolockArg(const char *str, ...);

        bool dprintfLock(int timeout);
        void dprintfUnlock(void);

        #define DPRINTF_TYPE(type, ...) dprintfType(type, __VA_ARGS__)
        #define DPRINTF_FULL_TYPE(type, file, line, func, ...) { \
            DPRINTF_TYPE(type, __VA_ARGS__); \
        }
        #define DPRINTF_RAW(...) dprintfRaw(__VA_ARGS__)
        #define DPRINTF_NOLOCK(str) dprintfNolock(str)
        #define DPRINTF_NOLOCK_ARG(...) dprintfNolockArg(__VA_ARGS__)

        #if DPRINTF_USE_RAMLOG == 1
            #define DPRINTF_EMERG(str) { \
                ramLogWrite(SWO_EN, str); \
            }
            #define DPRINTF_EMERG_ARG(str, ...) { \
                ramLogWrite(SWO_EN, str, __VA_ARGS__); \
            }
        #else
            #define DPRINTF_EMERG(str) { \
                minimalWrite(str); \
            }
            #define DPRINTF_EMERG_ARG(str, ...) { \
                safePrint(str, __VA_ARGS__); \
            }
        #endif
    #else
        #if DPRINTF_USE_RAMLOG == 1
            #error "DPRINTF_USE_RAMLOG not available without DPRINTF_RTOS"
        #endif
        #define DPRINTF_TYPE(type, ...) { \
            safePrint("%8s: ", type); \
            safePrint(__VA_ARGS__); \
        }
        #define DPRINTF_FULL_TYPE(type, file, line, func, ...) { \
            DPRINTF_TYPE(type, __VA_ARGS__) \
        }
        #define DPRINTF_RAW(...) { \
            safePrint(__VA_ARGS__); \
        }
        #define DPRINTF_NOLOCK(str) { \
            minimalWrite(str); \
        }
        #define DPRINTF_NOLOCK_ARG(...) { \
            DPRINTF_RAW(__VA_ARGS__); \
        }
        #define DPRINTF_EMERG(str) { \
            minimalWrite(str); \
        }
        #define DPRINTF_EMERG_ARG(str, ...) { \
            DPRINTF_RAW(str, __VA_ARGS__); \
        }
    #endif

#else
    #define DPRINTF_TYPE(...) {}
    #define DPRINTF_FULL_TYPE(type, file, line, func, ...) {}
    #define DPRINTF_NOLOCK(str) {}
    #define DPRINTF_NOLOCK_ARG(...) {}
    #define DPRINTF_RAW(...) {}
#endif

#endif /* __DEBUG_PRINT_H__ */
