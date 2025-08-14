/**
 * @file
 * Definition of the Xmodem protocol API.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_XMODEM_H_
#define NUVC_XMODEM_H_

#include <stdint.h>

#define XMODEM_STATUS_OFFSET (0)

typedef enum {
    XMODEM_OK = 0,
    XMODEM_CANCELLED = XMODEM_STATUS_OFFSET - 1,
    XMODEM_PROTOCOL_ERROR = XMODEM_STATUS_OFFSET - 2,
    XMODEM_TIMEOUT = XMODEM_STATUS_OFFSET - 3,
    XMODEM_LAST_ERROR = XMODEM_STATUS_OFFSET - 4,
} XmodemStatus;

/**
 * Callback prototype for storing the next XMODEM packet data.
 *
 * This function is called every time XMODEM finishes transferring a new
 * packet. The data and its size is passed to this function.
 *
 * The storage argument points to a buffer that can be used by the function
 * how it wishes. It is up to the user to co-ordinate these callbacks with the
 * exact specifications of the pointer target.
 *
 * @param[in] data      points to the data of the most recent packet
 * @param[in] size      holds the size of the data
 * @param[in] storage   points to a space to be used by the function
 */
typedef void (*XmodemStoreNextPacket)(const uint8_t *data, uint16_t size, void *storage);

/**
 * Callback prototype for retrieving the data for the next XMODEM packet.
 *
 * This function is called before the next XMODEM packet gets sent out. The
 * caller specifies how large the data buffer is, but not all bytes need to be
 * filled in. When there is no more data to be sent out, the return value
 * should indicate a number less than 'size'.
 *
 * The storage argument points to a buffer that can be used by the function
 * how it wishes. It is up to the user to co-ordinate these callbacks with the
 * exact specifications of the pointer target.
 *
 * @param[in] data      points to the data buffer for the next packet
 * @param[in] size      holds the size of the data buffer
 * @param[in] storage   points to a space to be used by the function
 * @return              the number of bytes filled into data
 */
typedef int16_t (*XmodemGetNextPacket)(uint8_t *data, uint16_t size, void *storage);

/**
 * Callback prototype for XMODEM to use as stdin.
 *
 * @param[in] timeoutMs     timeout in milliseconds to wait for a character
 * @return                  negative if timed out, otherwise the character
 */
typedef int (*XmodemReadByte)(uint16_t timeoutMs);

/**
 * Callback prototype for XMODEM to use as stdout.
 *
 * @param[in] byte      character to send out on stdout
 */
typedef void (*XmodemWriteByte)(int byte);

/**
 * Receives a file via the xmodem protocol using the specified read and write
 * functions.
 *
 * @param[in]     callback  the function to store data as it comes in
 * @param[in]     read      used by the protocol to receive bytes
 * @param[in]     write     used by the protocol to send bytes
 * @param[in,out] storage   passed directly to callback for custom use
 * @return                  error number if negative or the size of the file
 */
extern int32_t XmodemReceive(XmodemStoreNextPacket callback, XmodemReadByte read, XmodemWriteByte write, void *storage);

/**
 * Sends a file via the xmodem protocol using the specified read and write
 * functions.
 *
 * @param[in]     callback  the function to buffer data as it gets out
 * @param[in]     read      used by the protocol to receive bytes
 * @param[in]     write     used by the protocol to send bytes
 * @param[in,out] storage   passed directly to callback for custom use
 * @return                  error number if negative or the size of the file
 */
extern int32_t XmodemTransmit(XmodemGetNextPacket callback, XmodemReadByte read, XmodemWriteByte write, void *storage);

#endif /* NUVC_XMODEM_H_ */
