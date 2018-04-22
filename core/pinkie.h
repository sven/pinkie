/**
 * @brief PINKIE - Main Header
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_H
#define PINKIE_H


/*****************************************************************************/
/* Includes */
/*****************************************************************************/
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>


/*****************************************************************************/
/* Datatypes */
/*****************************************************************************/
typedef int PINKIE_RES_T;                       /**< PINKIE result */


/*****************************************************************************/
/* Includes */
/*****************************************************************************/
#include <pinkie_cfg.h>
#include <pinkie_arch.h>
#include <pinkie_crc.h>
#include <pinkie_stdio.h>
#include <pinkie_endian.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define PINKIE_CC_ASSERT(cond, msg) _Static_assert(cond, msg)

#define PINKIE_UNUSED(x)            (void)(x)
#define PINKIE_ARRAY_COUNT(x)       (sizeof(x) / sizeof(x[0]))

#ifdef PINKIE_ARCH_ENDIAN_LITTLE
#  define PINKIE_HTOBE16(x)         pinkie_swap16_ua((uint8_t *) &x)
#  define PINKIE_BE16TOH(x)         pinkie_swap16_ua((uint8_t *) &x)
#  define PINKIE_BE24TOH(x)         pinkie_swap24_ua((uint8_t *) &x)
#else
#  define PINKIE_HTOBE16(x)         (x)
#  define PINKIE_BE16TOH(x)         (x)
#  define PINKIE_BE24TOH(x)         (x)
#endif

#define PINKIE_OK                   0
#define PINKIE_ERR_NO_BUDGET        -1
#define PINKIE_ERR_TIMEOUT          -2


#endif /* PINKIE_H */
