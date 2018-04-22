/**
 * @brief PINKIE - Input/Output API
 *
 * Provides functions:
 *   pinkie_i2s - integer to string
 *   pinkie_s2i - string to integer
 *   pinkie_c2i - character to integer
 *   pinkie_printf - printf
 *   pinkie_sscanf - sscanf
 *
 * Most functions work from 8 to 64 bit.
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_STDIO_H
#define PINKIE_STDIO_H

#include <pinkie.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#if PINKIE_CFG_PRINTF_MAX_INT == 1
#  define PINKIE_PRINTF_INT_T int8_t
#  define PINKIE_PRINTF_UINT_T uint8_t
#endif

#if PINKIE_CFG_PRINTF_MAX_INT == 2
#  define PINKIE_PRINTF_INT_T int16_t
#  define PINKIE_PRINTF_UINT_T uint16_t
#endif

#if PINKIE_CFG_PRINTF_MAX_INT == 4
#  define PINKIE_PRINTF_INT_T int32_t
#  define PINKIE_PRINTF_UINT_T uint32_t
#endif

#if PINKIE_CFG_PRINTF_MAX_INT == 8
#  define PINKIE_PRINTF_INT_T int64_t
#  define PINKIE_PRINTF_UINT_T uint64_t
#endif

#if PINKIE_CFG_SSCANF_MAX_INT == 1
#  define PINKIE_SSCANF_INT_T int8_t
#  define PINKIE_SSCANF_UINT_T uint8_t
#endif

#if PINKIE_CFG_SSCANF_MAX_INT == 2
#  define PINKIE_SSCANF_INT_T int16_t
#  define PINKIE_SSCANF_UINT_T uint16_t
#endif

#if PINKIE_CFG_SSCANF_MAX_INT == 4
#  define PINKIE_SSCANF_INT_T int32_t
#  define PINKIE_SSCANF_UINT_T uint32_t
#endif

#if PINKIE_CFG_SSCANF_MAX_INT == 8
#  define PINKIE_SSCANF_INT_T int64_t
#  define PINKIE_SSCANF_UINT_T uint64_t
#endif


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
void pinkie_i2s(
    PINKIE_PRINTF_UINT_T num,                   /**< unsigned integer */
    unsigned int base,                          /**< base */
    uint8_t flg_zero,                           /**< prefix with zero */
    uint8_t cnt_pad                             /**< pad count */
);

#ifndef pinkie_printf
void pinkie_printf(
    const char *fmt,                            /**< format string */
    ...                                         /**< variable arguments */
) __attribute__((format(printf, 1, 2)));
#endif

int pinkie_sscanf(
    const char *str,                            /**< input string */
    const char *fmt,                            /**< format string */
    ...                                         /**< variable arguments */
) __attribute__((format(scanf, 2, 3)));

const char * pinkie_s2i(
    const char *str,                            /**< string */
    unsigned int width,                         /**< width = sizeof(type) */
    PINKIE_SSCANF_UINT_T num_max,               /**< max num value */
    void *val,                                  /**< value */
    unsigned int flg_neg,                       /**< negative flag */
    unsigned int base                           /**< base */
);

int pinkie_c2i(
    const char chr,                             /**< character */
    unsigned int base                           /**< base */
);

PINKIE_RES_T pinkie_stdio_init(
    void
);

#ifndef pinkie_stdio_exit
PINKIE_RES_T pinkie_stdio_exit(
    void
);
#endif

#ifndef pinkie_stdio_avail
PINKIE_RES_T pinkie_stdio_avail(
    void
);
#endif

#ifndef pinkie_stdio_getc
char pinkie_stdio_getc(
    void
);
#endif

#ifndef pinkie_stdio_putc
void pinkie_stdio_putc(
    char c
);
#endif


#endif /* PINKIE_STDIO_H */
