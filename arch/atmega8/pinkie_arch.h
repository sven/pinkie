/**
 * @brief PINKIE - Microchip ATmega8 Architecture
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_ARCH_H
#define PINKIE_ARCH_H

#include <pinkie.h>
#include <drv/nvs/pinkie_nvs.h>
#include <drv/spi/pinkie_spi.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define PINKIE_ARCH_ENDIAN_LITTLE       1
#define pinkie_stdio_exit()

#ifndef PRIu64
#  define PRIu64                        "llu"
#endif

#ifndef PRIi64
#  define PRIi64                        "lli"
#endif

#ifndef PRIx64
#  define PRIx64                        "llx"
#endif


#endif /* PINKIE_ARCH_H */
