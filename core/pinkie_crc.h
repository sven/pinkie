/**
 * @brief PINKIE - CRC Routines
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_CRC_H
#define PINKIE_CRC_H


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
uint16_t pinkie_crc16(
    uint8_t *data,                              /**< data */
    unsigned int len,                           /**< data length */
    uint16_t poly                               /**< polynomial */
);


#endif /* PINKIE_CRC_H */
