/*
 * webCmdHandler.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 10, 2023
 *      Author: rlegault
 */

#ifndef WEBSERVER_INC_WEBCMDHANDLER_H_
#define WEBSERVER_INC_WEBCMDHANDLER_H_

#include "mongooseHandler.h"
#include "registerParams.h"

#define HTTP_OK 200
#define HTTP_ERROR_BAD_REQUEST 400
#define HTTP_ERROR_INTERNAL_SERVER 500
#define HTTP_ERROR_PRECONDITION_FAILED 412

#define HTTP_BUFFER_SIZE 110

/* Creates
   webResponse_tp p_webResponse
   json_object *jsonResponse
   struct json_tokener *jsonTok
   struct json_object *obj;
*/
#define WEB_CMD_PARAM_SETUP(jsonStr, strLen)                                                                           \
    int cnt = 0;                                                                                                       \
    webResponse_tp p_webResponse = NULL;                                                                               \
    while (cnt++ < 10) {                                                                                               \
        p_webResponse = osPoolAlloc(webResponsePoolId);                                                                \
        if (p_webResponse != NULL) {                                                                                   \
            break;                                                                                                     \
        }                                                                                                              \
        osDelay(10);                                                                                                   \
    }                                                                                                                  \
    if (p_webResponse == NULL) {                                                                                       \
        return NULL;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    json_object *jsonResponse = json_object_new_object();                                                              \
    if (jsonResponse == NULL) {                                                                                        \
        osPoolFree(webResponsePoolId, p_webResponse);                                                                  \
        return NULL;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    p_webResponse->jsonResponse = jsonResponse;                                                                        \
                                                                                                                       \
    struct json_tokener *jsonTok = json_tokener_new();                                                                 \
    struct json_object *obj;                                                                                           \
    obj = json_tokener_parse_ex(jsonTok, jsonStr, strLen);                                                             \
    if (obj == NULL) {                                                                                                 \
        json_tokener_free(jsonTok);                                                                                    \
        struct json_object *jsonResult = json_object_new_string("No Json Structure in body");                          \
        json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);   \
        return p_webResponse;                                                                                          \
    }

#define WEB_CMD_PARAM_CLEANUP                                                                                          \
    json_object_put(obj);                                                                                              \
    json_tokener_free(jsonTok);

#define WEB_CMD_NO_PARAM_SETUP(jsonStr, strLen)                                                                        \
    webResponse_tp p_webResponse = NULL;                                                                               \
    int cnt = 0;                                                                                                       \
    while (cnt++ < 10) {                                                                                               \
        p_webResponse = osPoolAlloc(webResponsePoolId);                                                                \
        if (p_webResponse != NULL) {                                                                                   \
            break;                                                                                                     \
        }                                                                                                              \
        osDelay(10);                                                                                                   \
    }                                                                                                                  \
    if (p_webResponse == NULL) {                                                                                       \
        return NULL;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    json_object *jsonResponse = json_object_new_object();                                                              \
    if (jsonResponse == NULL) {                                                                                        \
        osPoolFree(webResponsePoolId, p_webResponse);                                                                  \
        return NULL;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    p_webResponse->jsonResponse = jsonResponse;                                                                        \
                                                                                                                       \
    p_webResponse->httpCode = HTTP_OK;                                                                                 \
    json_object *jsonResult = json_object_new_string("success");                                                       \
    json_object_object_add_ex(p_webResponse->jsonResponse, "result", jsonResult, JSON_C_OBJECT_KEY_IS_CONSTANT);

/**
 * @Documentation See Document SAQ01_SDD_Main_Board
 * for URI json body descriptions for the following
 * commands
 */

/**
 * @fn
 *
 * @brief get the IP address and prepare JSON structure
 *
 * @param jsonStr: http body, for this command it is empty
 * @param strLen: len should be 0
 * @return http structure with JSON body with result.
 * */
webResponse_tp webGetIp(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief set the IP address and prepare JSON structure
 *
 * @param jsonStr: http body containing JSON parameters
 *              for setting the ip address
 * @param strLen: length of http body
 * @return http structure with JSON body with result.
 * */
webResponse_tp webSetIp(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief get the UDP Port address and prepare JSON structure
 *
 * @param jsonStr: http body, for this command it is empty
 * @param strLen: len should be 0
 * @return http structure with JSON body with result.
 * */
webResponse_tp webTxUdpPortGet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief set the UDP port and prepare JSON structure
 *
 * @param jsonStr: http body containing JSON parameters
 *              for setting the udp port
 * @param strLen: length of http body
 * @return http structure with JSON body with result.
 * */
webResponse_tp webTxUdpPortSet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief get the Server IP address and port and prepare JSON structure
 *
 * @param jsonStr: http body, for this command it is empty
 * @param strLen: len should be 0
 * @return http structure with JSON body with result.
 * */
webResponse_tp webUdpServerIpGet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief set the Server IP address and port and prepare JSON structure
 *
 * @param jsonStr: http body with ip setting
 * @param strLen: length of http body
 * @return http structure with JSON body with result.
 * */
webResponse_tp webUdpServerIpSet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief get the TCP Listening Port and prepare JSON structure
 *
 * @param jsonStr: http body with json parameters
 * @param strLen: length of http body
 * @return http structure with JSON body with result.
 * */
webResponse_tp webTcpClientPortGet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief set the TCP Listening port and prepare JSON structure
 *
 * @param jsonStr: http body containing JSON parameters
 *              for setting the TCP CLIENT LISTENING port
 * @param strLen: len of jsonStr
 * @return http structure with JSON body with result.
 * */
webResponse_tp webTcpClientPortSet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief get the DaughterBoard process loopback status
 *        and prepare JSON structure
 *
 * @param jsonStr: http body, for this command it is empty
 * @param strLen: len should be 0
 * @return http structure with JSON body with result.
 * */
webResponse_tp webDbProcLoopbackGet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief set the DaughterBoard process loopback status
 *        and prepare JSON structure
 *
 * @param jsonStr: http body, for this command it is empty
 * @param strLen: len should be 0
 * @return http structure with JSON body with result.
 * */
webResponse_tp webDbProcLoopbackSet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief get the SPI commands interval to daughter board
 *        and prepare JSON structure
 *
 * @param jsonStr: http body, for this command it is empty
 * @param strLen: len should be 0
 * @return http structure with JSON body with result.
 * */
webResponse_tp webDbProcSpiIntervalGet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief set the SPI commands interval to daughter board
 *        and prepare JSON structure
 *
 * @param jsonStr: http body with json parameters
 * @param strLen: length of http body
 * @return http structure with JSON body with result.
 * */
webResponse_tp webDbProcSpiIntervalSet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief get the UDP stream packet interval
 *        and prepare JSON structure
 *
 * @param jsonStr: http body, for this command it is empty
 * @param strLen: len should be 0
 * @return http structure with JSON body with result.
 * */
webResponse_tp webUdpStreamIntervalGet(const char *jsonStr, const int strLen);

/**
 * @fn
 *
 * @brief set the UDP stream packet interval
 *        and prepare JSON structure
 *
 * @param jsonStr: http body with json parameters
 * @param strLen: lengthn of http body
 * @return http structure with JSON body with result.
 * */
webResponse_tp webUdpStreamIntervalSet(const char *jsonStr, const int strLen);

/**
 * @fn webMfgWriteEnGet
 *
 * @brief Handler for web API to get the Manufacture Write Enable setting
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webMfgWriteEnGet(const char *jsonStr, int strLen);

/**
 * @fn webMfgWriteEnSet
 *
 * @brief Handler for web API to set the Manufacture Write Enable setting
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webMfgWriteEnSet(const char *jsonStr, int strLen);

/**
 * @fn webClientTxIpDataTypeSet
 *
 * @brief Handler for web API to set the TX ip data type TCP or UDP
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webClientTxIpDataTypeSet(const char *jsonStr, const int strLen);

/**
 * @fn webClientTxIpDataTypeGet
 *
 * @brief Handler for web API to get the TX ip data type TCP or UDP
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webClientTxIpDataTypeGet(const char *jsonStr, const int strLen);

/**
 * @fn webTxClientIpSet
 *
 * @brief Handler for web API to set the TX TCP Client ip address
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webTxClientIpSet(const char *jsonStr, const int strLen);

/**
 * @fn webTxClientIpGet
 *
 * @brief Handler for web API to get the TX TCP Client ip address
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webTxClientIpGet(const char *jsonStr, const int strLen);

/**
 * @fn webSensorConfigureSet
 *
 * @brief Handler for web API to configure the sensor board type mfg data
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webSensorConfigureSet(const char *jsonStr, const int strLen);

/**
 * @fn webSensorConfigureGet
 *
 * @brief Handler for web API to configure the sensor board type mfg data
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webSensorConfigureGet(const char *jsonStr, const int strLen);

/**
 * @fn webSystemStatusGet
 *
 * @brief Handler for web API to get the system status state
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webSystemStatusGet(const char *jsonStr, const int strLen);

/**
 * @fn webSensorConfigureStatusGet
 *
 * @brief Handler for web API to get the sensor boards status
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webSensorConfigureStatusGet(const char *jsonStr, const int strLen);

/**
 * @fn webCorrectTargetToEmpty
 *
 * @brief Handler for web API to alter all the target values in the
 *        eeprom register SENSOR_BOARD_x to BOARDTYPE_EMPTY (8)
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCorrectTargetToEmpty(const char *jsonStr, int strLen);

/**
 * @fn webCorrectTargetToEmpty
 *
 * @brief Handler for web API to alter all the target values in the
 *        eeprom register SENSOR_BOARD_x to newvalue
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 *
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCorrectTargetToNewValue(const char *jsonStr, int strLen);

#endif /* WEBSERVER_INC_WEBCMDHANDLER_H_ */
