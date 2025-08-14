/*
 * imuLib.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Aug 17, 2024
 *      Author: rlegault
 */

#ifndef LIB_INC_IMULIB_H_
#define LIB_INC_IMULIB_H_

/**
 * @fn displaySentBinaryData
 *
 * @brief Translate and print the IMU packet
 * @Note the data comes in as Big Endian format.
 *
 * @param[in] rxBuffer, pointer to the IMU data to print  first bytes should be the alpha or quanternion data array
 * @param[in] imu data type.
 **/
void displaySentBinaryData(uint8_t *rxBuffer, BINARY_DATA_FIELD_e dataType);

#endif /* LIB_INC_IMULIB_H_ */
