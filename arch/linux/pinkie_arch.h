/**
 * @brief PINKIE - Linux Architecture
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_ARCH_H
#define PINKIE_ARCH_H

#include <stdio.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define PINKIE_ARCH_ENDIAN_LITTLE     1
#else
#  define PINKIE_ARCH_ENDIAN_BIG        1
#endif

#define pinkie_stdio_getc               getchar
#define pinkie_stdio_putc               putchar
#define pinkie_stdio_getc               getchar
#define pinkie_stdio_avail()            (!feof(stdin))
#define pinkie_arch_init_fin()


#endif /* PINKIE_ARCH_H */
