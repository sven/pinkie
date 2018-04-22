/**
 * @brief RegReg - Access registers via ACyCLIC CLI commands
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <acyclic.h>
#include <regreg.h>
#include <regreg_acyclic.h>


/*****************************************************************************/
/* Local datatypes */
/*****************************************************************************/
typedef union {
    int8_t vali8;                               /**< 8-bit signed value */
    uint8_t val8;                               /**< 8-bit unsigned value */
    int16_t vali16;                             /**< 16-bit signed value */
    uint16_t val16;                             /**< 16-bit unsigned value */
#if PINKIE_CFG_PRINTF_MAX_INT >= 4
    int32_t vali32;                             /**< 32-bit signed value */
    uint32_t val32;                             /**< 32-bit unsigned value */
#endif
#if PINKIE_CFG_PRINTF_MAX_INT >= 8
    int64_t vali64;                             /**< 64-bit signed value */
    uint64_t val64;                             /**< 64-bit unsigned value */
#endif
} REGREG_ACYCLIC_DATA_T;


/*****************************************************************************/
/* Local prototypes */
/*****************************************************************************/
static uint8_t cmd_func_reg_read(
    struct ACYCLIC_T *a
);

static uint8_t cmd_func_reg_write(
    struct ACYCLIC_T *a
);


/*****************************************************************************/
/* Commands */
/*****************************************************************************/
ACYCLIC_CMD(reg_write, "write", NULL, NULL, cmd_func_reg_write);
ACYCLIC_CMD(reg_read64, "read64", &cmd_reg_write, NULL, cmd_func_reg_read);
ACYCLIC_CMD(reg_read32, "read32", &cmd_reg_read64, NULL, cmd_func_reg_read);
ACYCLIC_CMD(reg_read16, "read16", &cmd_reg_read32, NULL, cmd_func_reg_read);
ACYCLIC_CMD(reg_read, "read", &cmd_reg_read16, NULL, cmd_func_reg_read);
ACYCLIC_CMD(reg, "reg", NULL, &cmd_reg_read, NULL);


/*****************************************************************************/
/** Register commands
 */
int regreg_acyclic_init(
    struct ACYCLIC_T *a
)
{
    return acyclic_cmd_add(a, &a->cmds, &cmd_reg);
}


/*****************************************************************************/
/** CLI Register Read
 */
static uint8_t cmd_func_reg_read(
    struct ACYCLIC_T *a
)
{
    uint8_t res = 0;                            /* result */
    REGREG_ACYCLIC_DATA_T data;                 /* register data */
    uint16_t range;                             /* read range */
    uint8_t flg_string = 0;                     /* string flag */
    REG_ACC_T reg_acc;                          /* register access */

    /* read size */
    if (cmd_name_reg_read16 == a->args[1].name) {
        reg_acc.data_len = sizeof(uint16_t);
    }
#if PINKIE_CFG_PRINTF_MAX_INT >= 4
    else if (cmd_name_reg_read32 == a->args[1].name) {
        reg_acc.data_len = sizeof(uint32_t);
    }
#endif
#if PINKIE_CFG_PRINTF_MAX_INT >= 8
    else if (cmd_name_reg_read64 == a->args[1].name) {
        reg_acc.data_len = sizeof(uint64_t);
    }
#endif
    else {
        reg_acc.data_len = sizeof(uint8_t);
    }

    /* initialize register access */
    pinkie_s2i(a->args[2].name, sizeof(uint16_t), UINT16_MAX, &reg_acc.addr, 0, 0);
    reg_acc.write_flg = 0;
    reg_acc.data.write_to = (uint8_t *) &data;
    range = (3 < a->arg_cnt) ? (uint8_t) str_to_u16(a->args[3].name, a->args[3].len) : 1;

    /* read string flag if available */
    if (4 < a->arg_cnt) {
        flg_string = ('s' == a->args[4].name[0]);
    }

    for (; range--; reg_acc.addr++) {
        res = reg_rw(&reg_acc);
        if (res) {
            ACYCLIC_PLAT_PRINTF("%u: denied\n", reg_acc.addr);
            continue;
        }

        if (flg_string) {
            ACYCLIC_PLAT_PUTC(*((char *) &data));
        } else {
            if (sizeof(uint16_t) == reg_acc.data_len) {
                ACYCLIC_PLAT_PRINTF("%u: 0x%04" PRIx16 " (u: %" PRIu16 ", i: %" PRIi16 ")\n", reg_acc.addr, data.val16, data.val16, data.vali16);
                reg_acc.addr++;
            }
#if PINKIE_CFG_PRINTF_MAX_INT >= 4
            else if (sizeof(uint32_t) == reg_acc.data_len) {
                ACYCLIC_PLAT_PRINTF("%u: 0x%08" PRIx32 " (u: %" PRIu32 ", i: %" PRIi32 ")\n", reg_acc.addr, data.val32, data.val32, data.vali32);
                reg_acc.addr += 3;
            }
#endif
#if PINKIE_CFG_PRINTF_MAX_INT >= 8
            else if (sizeof(uint64_t) == reg_acc.data_len) {
                ACYCLIC_PLAT_PRINTF("%u: 0x%016" PRIx64 " (u: %" PRIu64 ", i: %" PRIi64 ")\n", reg_acc.addr, data.val64, data.val64, data.vali64);
                reg_acc.addr += 7;
            }
#endif
            else {
                ACYCLIC_PLAT_PRINTF("%u: 0x%02x (u: %u, i: %i)\n", reg_acc.addr, data.val8, data.val8, data.vali8);
            }
        }
    }

    if (flg_string) {
        ACYCLIC_PLAT_PUTC('\n');
    }

    return res;
}


/*****************************************************************************/
/** CLI Write Single Register
 */
static uint8_t cmd_func_reg_write(
    struct ACYCLIC_T *a
)
{
    uint8_t data;                               /* register data */
    unsigned int cnt_arg;                       /* argument counter */
    REG_ACC_T reg_acc;                          /* register access */

    /* initialize register access */
    reg_acc.addr = str_to_u16(a->args[2].name, a->args[2].len);
    reg_acc.write_flg = 1;

    /* iterate through arguments */
    for (cnt_arg = 3; cnt_arg < a->arg_cnt; cnt_arg++) {

        /* check if data argument is a string */
        if ('"' == a->args[cnt_arg].name[0]) {
            reg_acc.data.read_from = (const uint8_t *) &a->args[cnt_arg].name[1];
            reg_acc.data_len = 0;
            for (a->args[cnt_arg].name++; ('"' != a->args[cnt_arg].name[0]) && (0 < a->args[cnt_arg].len); a->args[cnt_arg].name++, a->args[cnt_arg].len--, reg_acc.data_len++);

            /* only 1 string is supported, leave after processing it */
            cnt_arg = a->arg_cnt;

        } else {

            /* TODO: switch to sscanf - argument is a single byte */
            data = (uint8_t) str_to_u16(a->args[cnt_arg].name, a->args[cnt_arg].len);
            reg_acc.data.read_from = &data;
            reg_acc.data_len = 1;
        }

        if (reg_rw(&reg_acc)) {
            ACYCLIC_PLAT_PRINTF("%u: write failed\n", reg_acc.addr);
            break;
        }

        /* increment register address for single byte access */
        reg_acc.addr++;
    }

    return 0;
}


/*****************************************************************************/
/** String To Unsigned Int 16 Bit
 */
uint16_t str_to_u16(
    const char *str,                            /**< string */
    unsigned int len                            /**< length */
)
{
    uint16_t val = 0;                           /* value */
    unsigned int mul = 1;                       /* multiplicator */

    /* check if parameters are valid (max value 65536) */
    if ((NULL == str) || (5 < len)) {
        return 0;
    }

    /* parse from string end */
    for (; len--;) {
        if (('0' <= str[len]) && ('9' >= str[len])) {
            val += ((unsigned int) str[len] - '0') * mul;
            mul *= 10;
        } else {
            return 0;
        }
    }

    return val;
}
