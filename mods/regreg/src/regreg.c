/**
 * @brief RegReg - Everything is a register
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <string.h>
#include "regreg.h"


/*****************************************************************************/
/* Local variables */
/*****************************************************************************/
static REG_ENTRY_T *regs = NULL;                /* register list */


/*****************************************************************************/
/** Add a register handler
 */
void reg_add(
    REG_ENTRY_T *reg                            /**< register entry pointer */
)
{
    REG_ENTRY_T **regs_it;                      /* register iterator */

    /* find last element */
    for (regs_it = &regs; *regs_it; regs_it = &(*regs_it)->next);

    /* attach new register entry */
    *regs_it = reg;
}


/*****************************************************************************/
/** Check if register is in range
 *
 * Warning: The result only shows the last performed register access. If
 * something in between when wrong it is skipped.
 *
 * @retval 0 successful
 * @retval other failed
 */
unsigned int reg_rw(
    REG_ACC_T *reg_acc                          /**< register access info */
)
{
    unsigned int res = 0;                       /* result */
    REG_ENTRY_T *reg;                           /* register */
    unsigned int found_flg = 0;                 /* found flag */
    unsigned int data_len = reg_acc->data_len;  /* overall data length */

    while (data_len) {

        for (reg = regs; reg; reg = reg->next) {
            if ((reg_acc->addr >= reg->addr_beg) && (reg_acc->addr <= reg->addr_end)) {
                found_flg = 1;
                break;
            }
        }

        if (!found_flg) {
            if (!(data_len - 1)) {
                return 1;
            }

            data_len--;
            reg_acc->addr++;
            continue;
        }

        /* calculate address offset */
        reg_acc->addr_ofs = reg_acc->addr - reg->addr_beg;

        /* calculate data length depending on overall requested reg_acc->data_len */
        reg_acc->data_len = reg->addr_end - reg->addr_beg + 1;
        if (data_len < reg_acc->data_len) {
            reg_acc->data_len = data_len;
        }
        data_len -= reg_acc->data_len;

        /* if register has a callback, call it once */
        if (reg->cb) {
            res = reg->cb(reg, reg_acc);
            if (REGREG_RES_PROCEED != res) {
                continue;
            }
        }

        /* read or write data to specific data ptr */
        if (reg_acc->write_flg) {
            memcpy(&((uint8_t *) reg->data)[reg_acc->addr_ofs], reg_acc->data.read_from, reg_acc->data_len);
        } else {
            memcpy(reg_acc->data.write_to, &((uint8_t *) reg->data)[reg_acc->addr_ofs], reg_acc->data_len);
        }

        /* correct result if necessary */
        if (REGREG_RES_PROCEED == res) {
            res = 0;
        }
    }

    return res;
}
