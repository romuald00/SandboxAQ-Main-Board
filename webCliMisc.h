/*
 * webCliMisc.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 5, 2023
 *      Author: rlegault
 */

#ifndef LIB_INC_WEBCLIMISC_H_
#define LIB_INC_WEBCLIMISC_H_

#include "mongooseHandler.h"
#include "webCmdHandler.h"
#include <stdbool.h>

#define UDP_TX_PORT_MIN 1024
#define UDP_TX_PORT_MAX 19999

#define HTTP_PORT_MIN 1024
#define HTTP_PORT_MAX 19999

/** use this to set keyString to a variable of the same name
 * @param type = int, bool, etc
 * @param keyString key value in the jsonObject
 * @jsonObject json tokenized object
 * @getObjectType, json call to pull out the json value,
 *                 ie. json_object_get_int
 **/
#define GET_REQ_KEY_VALUE(VAR_TYPE, KEY_STRING, JSON_OBJECT, JSON_OBJECT_GET_TYPE)                                     \
    VAR_TYPE KEY_STRING;                                                                                               \
    {                                                                                                                  \
        json_object *tmp;                                                                                              \
        if (!json_object_object_get_ex(JSON_OBJECT, #KEY_STRING, &tmp)) {                                              \
            p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;                                                          \
            struct json_object *jsonResult = json_object_new_string("missing param error");                            \
            json_object *jsonParam = json_object_new_string(#KEY_STRING);                                              \
            json_object_object_add_ex(                                                                                 \
                p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);                     \
            json_object_object_add_ex(p_webResponse->jsonResponse, "param", jsonParam, JSON_C_OBJECT_KEY_IS_CONSTANT); \
                                                                                                                       \
            WEB_CMD_PARAM_CLEANUP                                                                                      \
            json_object_put(tmp);                                                                                      \
            return p_webResponse;                                                                                      \
        }                                                                                                              \
        KEY_STRING = JSON_OBJECT_GET_TYPE(tmp);                                                                        \
    }

/**
 * Use this to test that VALUENAME falls between MINV and MAXV inclusive
 * return True or False, if false it also fills out a HTTP Json object return structure
 *
 * @requires that p_webResponse be present in code before call
 *
 * @param VALUENAME value to test
 * @param MINV minimum value VALUENAME can have
 * @param MAXV maximum value VALUENAME can have
 *
 **/
#define TEST_VALUE(VALUENAME, MINV, MAXV)                                                                              \
    if (VALUENAME < MINV || VALUENAME >= MAXV) {                                                                       \
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;                                                              \
        char buf[HTTP_BUFFER_SIZE];                                                                                    \
        json_object *jsonResult = json_object_new_string(#VALUENAME " value error");                                   \
        snprintf(buf, sizeof(buf), "Value %d out of range [%d:%d]", VALUENAME, MINV, MAXV - 1);                        \
        json_object *jsonDestError = json_object_new_string(buf);                                                      \
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);   \
        json_object_object_add_ex(                                                                                     \
            p_webResponse->jsonResponse, "detail", jsonDestError, JSON_C_OBJECT_KEY_IS_CONSTANT);                      \
        return false;                                                                                                  \
    }                                                                                                                  \
    return true;

/**
 * If jsonResult is not NULL then an error occured.
 * Create HTTP error response in p_webResponse
 * return True or False, if false it also fills out a HTTP Json object return structure
 *
 * Call this at the end of handler to check for errors.
 * Uses string in jsonResult to populate error response
 *
 * @requires that p_webResponse be present in code before call
 **/
#define CHECK_ERRORS                                                                                                   \
    if (jsonResult != NULL) {                                                                                          \
        p_webResponse->httpCode = HTTP_ERROR_BAD_REQUEST;                                                              \
        snprintf(buf, sizeof(buf), "ErrorCode %ld", webCncCbId.p_payload->cmd.cncMsgPayloadHeader.cncActionData.result);                   \
        json_object *jsonError = json_object_new_string(buf);                                                          \
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);   \
        json_object_object_add_ex(p_webResponse->jsonResponse, "detail", jsonError, JSON_C_OBJECT_KEY_IS_CONSTANT);    \
    } else {                                                                                                           \
        p_webResponse->httpCode = HTTP_OK;                                                                             \
        json_object *jsonResult = json_object_new_string("success");                                                   \
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);   \
    }

/* get destination and check for index or "all", populates result in KEY_STRING. Uses -1 to represent all boards,
 * otherwise stores the board index
 */
#define GET_DESTINATION(KEY_STRING)                                                                                    \
    GET_REQ_KEY_VALUE(const char *, destination, obj, json_object_get_string);                                         \
    if (strcmp(destination, "all") == 0) {                                                                             \
        KEY_STRING = DESTINATION_ALL;                                                                                  \
    } else {                                                                                                           \
        GET_REQ_KEY_VALUE(int, destination, obj, json_object_get_int);                                                 \
        KEY_STRING = destination;                                                                                      \
        testDaughterBoardId(p_webResponse, KEY_STRING);                                                                \
    }

/**
 * @fn
 *
 * @brief Check that the string contains a properly formated IP4 address
 *
 * @param str: point to the string with the IP address
 * @return TRUE if string format is IP4 compliant
 **/
bool validIpStr(const char *str);

/**
 * @fn
 *
 * @brief Test that dbId is a valid sensor board address
 *
 * @param[in] webResponse_tp p_webResponse: Web response structure
 * @param address of the sensor board
 * @return TRUE if dbId is within bounds
 **/
bool testDaughterBoardId(webResponse_tp p_webResponse, int dbId);

#endif /* LIB_INC_WEBCLIMISC_H_ */
