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
#include <pinkie.h>
#include <stdarg.h>
#include <limits.h>


/*****************************************************************************/
/** Integer To String
 *
 * This function doesn't need an extra buffer to store the string-converted
 * integer but uses a lot of CPU power. It is a compromise because I didn't
 * found a header that contains the actual count of characters that is needed
 * to represent the architecture specific integer as string. I didn't want a
 * catch-all solution that uses the same amount of reserved bytes for an 8-bit
 * system that is needed for an 64-bit integer.
 */
void pinkie_i2s(
    PINKIE_PRINTF_UINT_T num,                   /**< unsigned integer */
    unsigned int base,                          /**< base */
    uint8_t flg_zero,                           /**< prefix with zero */
    uint8_t cnt_pad                             /**< pad count */
)
{
    PINKIE_PRINTF_UINT_T num_temp;              /* temporary number buffer */
    unsigned int digit;                         /* found digit */
    unsigned int digits = 0;                    /* digits found */
    unsigned int digit_cnt;                     /* digit counter */
    uint8_t pad;                                /* pad character */

    /* count digits */
    for (num_temp = num; num_temp; digit = num_temp % base, num_temp /= base, digits++);

    /* check if padding is needed */
    if (cnt_pad > digits) {
        pad = ((flg_zero) || (!digits)) ? '0' : ' ';
        for (cnt_pad -= digits; cnt_pad; cnt_pad--) {
            pinkie_stdio_putc(pad);
        }
    }

    for (; digits; digits--) {
        for (num_temp = num, digit_cnt = digits; digit_cnt; digit_cnt--, digit = num_temp % base, num_temp /= base);

        pinkie_stdio_putc(digit + ((digit < 10) ? '0' : ('a' - 10)));
    }
}


#ifndef pinkie_printf
/*****************************************************************************/
/** PINKIE Just Enough Printf To Work
 */
void pinkie_printf(
    const char *fmt,                            /**< format string */
    ...                                         /**< variable arguments */
)
{
    va_list ap;                                 /* variable argument list */
    unsigned int flg_format = 0;                /* format flag */
    char *sub;                                  /* sub string */
    uint8_t cnt_pad = 1;                        /* pad count */
    uint8_t flg_zero = 0;                       /* prefix with zero */
    uint8_t flg_precision = 0;                  /* precision flag */
    int precision = INT_MAX;                    /* precision */
    PINKIE_PRINTF_INT_T val_int;                /* value */
    uint8_t base = 0;                           /* number base */
    PINKIE_PRINTF_UINT_T val = 0;               /* unsigned value */
    unsigned int int_width = 0;                 /* integer width */

    va_start(ap, fmt);
    for (; *fmt; fmt++) {

        if (flg_format) {

            if ('0' == *fmt) {
                flg_zero = 1;
                continue;
            }

            if (('1' <= *fmt) && ('9' >= *fmt)) {
                cnt_pad = *fmt - '0';
                continue;
            }

            if ((flg_precision) && ('*' == *fmt)) {
                precision = va_arg(ap, int);
                if (0 > precision) {
                    precision = 0;
                }
                continue;
            }

            if ('.' == *fmt) {
                flg_precision = 1;
                continue;
            }

            /* length field */
            if ('h' == *fmt) {
                int_width = (!int_width) ? sizeof(short) : sizeof(char);
                continue;
            }

            if ('l' == *fmt) {
                int_width = (!int_width) ? sizeof(long) : sizeof(long long);
                continue;
            }

            switch (*fmt) {
                case '%':
                    pinkie_stdio_putc('%');
                    break;

                case 's':
                    for (sub = va_arg(ap, char *); (precision--) && (*sub); sub++) {
                        pinkie_stdio_putc(*sub);
                    }
                    break;

                case 'c':
                    pinkie_stdio_putc((char) va_arg(ap, int));
                    break;

                case 'i':
                    val_int = va_arg(ap, int);
                    if (0 > val_int) {
                        pinkie_stdio_putc('-');
                        val_int = -val_int;
                    }
                    pinkie_i2s(val_int, 10, flg_zero, cnt_pad);
                    break;

                case 'u':                       /* unsigned int */
                    base = 10;
                    break;

                case 'x':
                    base = 16;
                    break;

            }

            if (base) {
#if (PINKIE_CFG_PRINTF_MAX_INT >= 4) && (UINT32_MAX != UINT_MAX)
                if (sizeof(uint32_t) == int_width) {
                    val = va_arg(ap, uint32_t);
                }
                else
#endif
#if (PINKIE_CFG_PRINTF_MAX_INT >= 8) && (UINT64_MAX != UINT_MAX)
                if (sizeof(uint64_t) == int_width) {
                    val = va_arg(ap, uint64_t);
                }
                else
#endif
                val = va_arg(ap, unsigned int);

                if (sizeof(uint16_t) == int_width) {
                    val = val & 0xffff;
                }
                else if (sizeof(uint8_t) == int_width) {
                    val = val & 0xff;
                }

                pinkie_i2s(val, base, flg_zero, cnt_pad);
                base = 0;
                int_width = 0;
            }

            cnt_pad = 1;
            flg_zero = 0;
            flg_format = 0;
            precision = INT_MAX;
            flg_precision = 0;

            continue;
        }

        if ('%' == *fmt) {
            flg_format = 1;
            continue;
        }

        pinkie_stdio_putc(*fmt);
    }
    va_end(ap);
}
#endif /* pinkie_printf */


#ifndef pinkie_sscanf
/*****************************************************************************/
/** PINKIE Just Enough Sscanf To Work
 *
 * Supports the following formatters:
 *   - %u, with ll and hh modifiers
 *   - %n
 */
int pinkie_sscanf(
    const char *str,                            /**< input string */
    const char *fmt,                            /**< format string */
    ...                                         /**< variable arguments */
)
{
    va_list ap;                                 /* variable argument list */
    unsigned int flg_format = 0;                /* format flag */
    unsigned int int_width = 0;                 /* integer width */
    const char *str_beg = str;                  /* string begin */
    int args = 0;                               /* parsed arguments counter */
    unsigned int flg_neg = 0;                   /* negative flag */

    va_start(ap, fmt);
    for (; (*fmt) && (*str); fmt++) {

        if (flg_format) {

            /* length field */
            if ('h' == *fmt) {
                int_width = (!int_width) ? sizeof(short) : sizeof(char);
                continue;
            }

            if ('l' == *fmt) {
                int_width = (!int_width) ? sizeof(long) : sizeof(long long);
                continue;
            }

            /* handle conversion */
            switch (*fmt) {

                case 'i':

                    /* detect negative sign */
                    if ('-' == *str) {
                        flg_neg = 1;
                        continue;
                    }

                    /* fallthrough to convert number */

                case 'u':
                    /* unsigned integer */
                    if (!int_width) {
                        str = pinkie_s2i(str, sizeof(unsigned int), UINT_MAX, va_arg(ap, unsigned int *), flg_neg, 0);
                    }
                    else if (sizeof(uint8_t) == int_width) {
                        str = pinkie_s2i(str, sizeof(uint8_t), UINT8_MAX, va_arg(ap, uint8_t *), flg_neg, 0);
                    }
#if (PINKIE_CFG_SSCANF_MAX_INT >= 2) && (UINT16_MAX != UINT_MAX)
                    else if (sizeof(uint16_t) == int_width) {
                        str = pinkie_s2i(str, sizeof(uint16_t), UINT16_MAX, va_arg(ap, uint16_t *), flg_neg, 0);
                    }
#endif
#if (PINKIE_CFG_SSCANF_MAX_INT >= 4) && (UINT32_MAX != UINT_MAX)
                    else if (sizeof(uint32_t) == int_width) {
                        str = pinkie_s2i(str, sizeof(uint32_t), UINT32_MAX, va_arg(ap, uint32_t *), flg_neg, 0);
                    }
#endif
#if (PINKIE_CFG_SSCANF_MAX_INT >= 8) && (UINT64_MAX != UINT_MAX)
                    else if (sizeof(uint64_t) == int_width) {
                        str = pinkie_s2i(str, sizeof(uint64_t), UINT64_MAX, va_arg(ap, uint64_t *), flg_neg, 0);
                    }
#endif

                    /* reset integer width */
                    int_width = 0;

                    /* update args */
                    args++;

                    break;

                case '%':
                    /* percent char */
                    flg_format = 0;
                    goto pinkie_sscanf_match;

                case 'n':
                    /* position */
                    *(va_arg(ap, int *)) = str - str_beg;
                    break;
            }

            flg_format = 0;
            flg_neg = 0;
            continue;
        }

        if ('%' == *fmt) {
            flg_format = 1;
            continue;
        }

pinkie_sscanf_match:
        /* string content must match format */
        if (*fmt != *str++) {
            break;
        }
    }
    va_end(ap);

    return args;
}
#endif /* pinkie_sscanf */


/*****************************************************************************/
/** String To Integer
 */
const char * pinkie_s2i(
    const char *str,                            /**< string */
    unsigned int width,                         /**< width = sizeof(type) */
    PINKIE_SSCANF_UINT_T num_max,               /**< max num value */
    void *val,                                  /**< value */
    unsigned int flg_neg,                       /**< negative flag */
    unsigned int base                           /**< base */
)
{
    PINKIE_SSCANF_UINT_T num = 0;               /* number */
    unsigned int mul = 1;                       /* multiplicator */
    unsigned int cnt = 0;                       /* counter */
    unsigned int cur;                           /* current number */
    const char *str_end = NULL;                 /* number end */

    /* detect number type */
    if (0 == base) {
        if (((str[0]) && ('0' == str[0])) && ((str[1]) && ('x' == str[1]))) {
            base = 16;
            str += 2;
        } else {
            base = 10;
        }
    }

    /* count numbers */
    for (; (*str) && (-1 != pinkie_c2i(*str, base)); str++) {
        cnt++;
    }

    /* store number end */
    str_end = str;

    /* check if anything was detected */
    if (!cnt) {
        goto bail;
    }

    /* convert integers */
    while (cnt--) {
        str--;

        /* apply multiplicator to conv result */
        cur = pinkie_c2i(*str, base) * mul;

        if ((num_max - cur) <= num) {
            str_end = 0;
            goto bail;
        }

        num += cur;
        mul *= base;
    }

bail:
    /* convert result to given width */
    if (sizeof(uint8_t) == width) {
        *((uint8_t *) val) = (flg_neg) ? -num : num;
    }
#if PINKIE_CFG_SSCANF_MAX_INT >= 2
    else if (sizeof(uint16_t) == width) {
        *((uint16_t *) val) = (flg_neg) ? -num : num;
    }
#endif
#if PINKIE_CFG_SSCANF_MAX_INT >= 4
    else if (sizeof(uint32_t) == width) {
        *((uint32_t *) val) = (flg_neg) ? -num : num;
    }
#endif
#if PINKIE_CFG_SSCANF_MAX_INT >= 8
    else if (sizeof(uint64_t) == width) {
        *((uint64_t *) val) = (flg_neg) ? -num : num;
    }
#endif

    return str_end;
}


/*****************************************************************************/
/** Convert character to integer
 *
 * @retval number or -1 if not a number
 */
int pinkie_c2i(
    const char chr,                             /**< character */
    unsigned int base                           /**< base */
)
{
    if (('0' <= chr) && ('9' >= chr)) {
        return (chr - '0');
    }

    if (16 == base) {
        if (('a' <= chr) && ('f' >= chr)) {
            return (chr - 'a' + 10);
        }

        if (('A' <= chr) && ('F' >= chr)) {
            return (chr - 'A' + 10);
        }
    }

    return -1;
}
