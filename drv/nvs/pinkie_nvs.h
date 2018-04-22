/**
 * @brief PINKIE - NVS Interface
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_NVS_H
#define PINKIE_NVS_H

#include <pinkie.h>


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
PINKIE_RES_T pinkie_nvs_read(
    uint8_t *data,                              /**< NVS data ptr */
    unsigned int len                            /**< NVS data length */
);

void pinkie_nvs_write(
    uint8_t *data,                              /**< NVS data ptr */
    unsigned int len                            /**< NVS data length */
);


#endif /* PINKIE_NVS_H */
