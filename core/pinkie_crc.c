/**
 * @brief PINKIE - CRC Routines
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <pinkie.h>


/*****************************************************************************/
/** PINKIE CRC16 Calculation following CRC-CCITT XMODEM
 */
uint16_t pinkie_crc16(
    uint8_t *data,                              /**< data */
    unsigned int len,                           /**< data length */
    uint16_t poly                               /**< polynomial */
)
{
    uint16_t crc;                               /* CRC16 */
    unsigned int cnt;                           /* data counter */
    unsigned int cnt_crc;                       /* CRC counter */

    /* initial value must be zero */
    crc = 0;

    for (cnt = 0; cnt < len; cnt++) {
        crc ^= data[cnt] << 8;

        for (cnt_crc = 0; cnt_crc < 8; cnt_crc++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ poly;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}
