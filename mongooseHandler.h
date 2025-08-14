/*
 * mongooseHandler.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 4, 2023
 *      Author: rlegault
 */

#ifndef WEBSERVER_INC_MONGOOSEHANDLER_H_
#define WEBSERVER_INC_MONGOOSEHANDLER_H_

#define DESTINATION_ALL -1

#define TRANSIENT_TASK_NOTIFY_TIMEOUT_MS 5000

#include "cmsis_os.h"
#include "json.h"
extern osPoolId webResponsePoolId;

typedef struct {
    int httpCode;
    json_object *jsonResponse;
} webResponse_t, *webResponse_tp;

#endif /* WEBSERVER_INC_MONGOOSEHANDLER_H_ */
