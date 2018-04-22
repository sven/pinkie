/**
 * @brief PINKIE - Endianness Conversion
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_ENDIAN_H
#define PINKIE_ENDIAN_H

#include <pinkie.h>


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
uint16_t pinkie_swap16_ua(
    uint8_t *val                                /**< ptr to value */
);

uint32_t pinkie_swap24_ua(
    uint8_t *val                                /**< ptr to value */
);


#endif /* PINKIE_ENDIAN_H */
