/**
 * @file
 * Implementation of the Xmodem protocol API.
 *
 * Copyright 2001-2010 Georges Menie (www.menie.org)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Modifications by Philipp Schrader (www.nuvation.com)
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "crc16.h"
#include "xmodem.h"

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CTRLZ 0x1A
#define CTRLC 0x03

#define DLY_10MS 10
#define DLY_1S 1000
#define MAXRETRANS 25

#define XMODEM_BUFFER_SIZE 128

#define READY_TO_SEND_STR "<READY_TO_SEND>\n"
#define READY_TO_RECV_STR "<READY_TO_RECEIVE>\n"

static const uint8_t readyToRecvString[] = READY_TO_RECV_STR;
static const uint8_t readyToSendString[] = READY_TO_SEND_STR;

static int check(int crc, const uint8_t *buf, uint16_t sz) {
    if (crc) {
        uint16_t crc = INITIAL_CRC16_CCITT;
        uint16_t tcrc = (buf[sz] << 8) + buf[sz + 1];
        crc16_ccitt(buf, sz, &crc);
        if (crc == tcrc)
            return 1;
    } else {
        int i;
        uint8_t cks = 0;
        for (i = 0; i < sz; ++i) {
            cks += buf[i];
        }
        if (cks == buf[sz])
            return 1;
    }

    return 0;
}

static inline void flushinput(XmodemReadByte read) {
    while (read(DLY_10MS) >= 0)
        ;
}

static inline void printToConsole(XmodemWriteByte write, const uint8_t *string) {
    while ((*string) != '\0') {
        write((int)(*string));
        string++;
    }
}

int32_t XmodemReceive(XmodemStoreNextPacket callback, XmodemReadByte read, XmodemWriteByte write, void *storage) {
    /* 128 for XModem packet + 3 head chars + 2 crc + nul. */
    unsigned char xbuff[XMODEM_BUFFER_SIZE + 3 + 2 + 1];

    unsigned char *p;
    const int bufsz = XMODEM_BUFFER_SIZE;
    int crc = 0;
    unsigned char trychar = 'C';
    unsigned char packetno = 1;
    int i, c;
    int32_t len = 0;
    int retry, retrans = MAXRETRANS;

    flushinput(read);

    printToConsole(write, readyToRecvString);

    for (;;) {
        for (retry = 0; retry < 16; ++retry) {
            if (trychar)
                write(trychar);
            if ((c = read((DLY_1S) << 1)) >= 0) {
                switch (c) {
                case SOH:
                    goto start_recv;
                case EOT:
                    flushinput(read);
                    write(ACK);
                    return len; /* Normal end. */
                case CAN:
                    if ((c = read(DLY_1S)) == CAN) {
                        flushinput(read);
                        write(ACK);
                        return XMODEM_CANCELLED; /* Canceled by remote. */
                    }
                    break;
                case CTRLC:
                    return XMODEM_CANCELLED;
                case STX: /* 1k packet size unsupported */
                default:
                    break;
                }
            }
        }
        if (trychar == 'C') {
            trychar = NAK;
            continue;
        }
        flushinput(read);
        write(CAN);
        write(CAN);
        write(CAN);
        write('\n');
        return XMODEM_PROTOCOL_ERROR; /* sync error */

    start_recv:
        if (trychar == 'C')
            crc = 1;
        trychar = 0;
        p = xbuff;
        *p++ = c;
        for (i = 0; i < (bufsz + (crc ? 1 : 0) + 3); ++i) {
            if ((c = read(DLY_1S)) < 0)
                goto reject;
            *p++ = c;
        }

        if (xbuff[1] == (unsigned char)(~xbuff[2]) &&
            (xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno - 1) && check(crc, &xbuff[3], bufsz)) {
            if (xbuff[1] == packetno) {
                callback(&xbuff[3], bufsz, storage);
                len += bufsz;
                ++packetno;
                retrans = MAXRETRANS + 1;
            }
            if (--retrans <= 0) {
                flushinput(read);
                write(CAN);
                write(CAN);
                write(CAN);
                return XMODEM_TIMEOUT; /* Too many retry error. */
            }
            write(ACK);
            continue;
        }
    reject:
        flushinput(read);
        write(NAK);
    }
}

int32_t XmodemTransmit(XmodemGetNextPacket callback, XmodemReadByte read, XmodemWriteByte write, void *storage) {
    /* 128 for XModem packet + 3 head chars + 2 crc + nul. */
    uint8_t xbuff[XMODEM_BUFFER_SIZE + 3 + 2 + 1];

    const int bufsz = XMODEM_BUFFER_SIZE;
    int crc = -1;
    uint8_t packetno = 1;
    int i, c;
    int32_t len = 0;
    int retry;

    printToConsole(write, readyToSendString);

    for (;;) {
        for (retry = 0; retry < 16; ++retry) {
            if ((c = read((DLY_1S) << 1)) >= 0) {
                switch (c) {
                case 'C':
                    crc = 1;
                    goto start_trans;
                case NAK:
                    crc = 0;
                    goto start_trans;
                case CAN:
                    if ((c = read(DLY_1S)) == CAN) {
                        write(ACK);
                        flushinput(read);
                        return XMODEM_CANCELLED; /* Canceled by remote. */
                    }
                    break;
                case CTRLC:
                    return XMODEM_CANCELLED;
                default:
                    break;
                }
            }
        }
        write(CAN);
        write(CAN);
        write(CAN);
        flushinput(read);
        return XMODEM_PROTOCOL_ERROR; /* No sync. */

        for (;;) {
        start_trans:
            xbuff[0] = SOH;
            xbuff[1] = packetno;
            xbuff[2] = ~packetno;
            c = callback(&xbuff[3], bufsz, storage);
            if (c > 0) {
                if (c == 0) {
                    xbuff[3] = CTRLZ;
                } else {
                    if (c < bufsz)
                        xbuff[3 + c] = CTRLZ;
                }
                if (crc) {
                    uint16_t ccrc = INITIAL_CRC16_CCITT;
                    crc16_ccitt(&xbuff[3], bufsz, &ccrc);
                    xbuff[bufsz + 3] = (ccrc >> 8) & 0xFF;
                    xbuff[bufsz + 4] = ccrc & 0xFF;
                } else {
                    uint8_t ccks = 0;
                    for (i = 3; i < bufsz + 3; ++i) {
                        ccks += xbuff[i];
                    }
                    xbuff[bufsz + 3] = ccks;
                }
                for (retry = 0; retry < MAXRETRANS; ++retry) {
                    for (i = 0; i < bufsz + 4 + (crc ? 1 : 0); ++i) {
                        write(xbuff[i]);
                    }
                    if ((c = read(DLY_1S)) >= 0) {
                        switch (c) {
                        case ACK:
                            ++packetno;
                            len += bufsz;
                            goto start_trans;
                        case CAN:
                            if ((c = read(DLY_1S)) == CAN) {
                                write(ACK);
                                flushinput(read);
                                return XMODEM_CANCELLED; /* Canceled by
                                                          * remote. */
                            }
                            break;
                        case NAK:
                        default:
                            break;
                        }
                    }
                }
                write(CAN);
                write(CAN);
                write(CAN);
                flushinput(read);
                return XMODEM_TIMEOUT; /* xmit error. */
            } else {
                for (retry = 0; retry < 10; ++retry) {
                    write(EOT);
                    if ((c = read((DLY_1S) << 1)) == ACK)
                        break;
                }
                flushinput(read);
                return (c == ACK) ? len : XMODEM_PROTOCOL_ERROR;
            }
        }
    }
}
