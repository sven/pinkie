/**
 * @brief PINKIE - Byte Swappings
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <pinkie.h>


/*****************************************************************************/
/** Swap Bytes, Unaligned, 16-Bit
 */
uint16_t pinkie_swap16_ua(
    uint8_t *val                                /**< ptr to value */
)
{
    return (val[0] << 8) | (val[1] & 0xff);
}


/*****************************************************************************/
/** Swap Bytes, Unaligned, 24-Bit
 */
uint32_t pinkie_swap24_ua(
    uint8_t *val                                /**< ptr to value */
)
{
    return ((uint32_t) val[0] << 16) | ((uint32_t) val[1] << 8) | ((uint32_t) val[2] & 0xff);
}
