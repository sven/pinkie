/**
 * @brief RegReg - Everything is a register
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef REGREG_H
#define REGREG_H

#include <inttypes.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define REGREG_RES_PROCEED              2       /**< proceed with register access */


/*****************************************************************************/
/* Structures */
/*****************************************************************************/
struct REG_ACC_T;
struct REG_ENTRY_T;


/**< register callback function */
typedef unsigned int (* REG_CB_T)( \
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
);


/**< register access info */
typedef struct REG_ACC_T {
    uint16_t addr;                              /**< address */
    uint16_t addr_ofs;                          /**< address offset */
    uint8_t write_flg;                          /**< write flag */
    union {
        const uint8_t *read_from;               /**< data pointer */
        uint8_t *write_to;                      /**< data pointer */
    } data;
    uint16_t data_len;                          /**< data length pointer */
} REG_ACC_T;


/**< register entry */
typedef struct REG_ENTRY_T {
    struct REG_ENTRY_T *next;                   /**< next element */

    uint16_t addr_beg;                          /**< start address */
    uint16_t addr_end;                          /**< end address */

    REG_CB_T cb;                                /**< callback function */
    void *data;                                 /**< specific data */
} REG_ENTRY_T;


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
void reg_add(
    REG_ENTRY_T *reg                            /**< register entry pointer */
);

unsigned int reg_rw(
    REG_ACC_T *reg_acc                          /**< register access info */
);

void reg_ann(
    uint16_t addr,                              /**< register address */
    void *data,                                 /**< data */
    unsigned int len                            /**< data length */
);


#endif /* REGREG_H */
