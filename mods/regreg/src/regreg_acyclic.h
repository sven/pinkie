/**
 * @brief RegReg - Access registers via ACyCLIC CLI commands
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef REGREG_ACYCLIC_H
#define REGREG_ACYCLIC_H


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
int regreg_acyclic_init(
    struct ACYCLIC_T *a
);

uint16_t str_to_u16(
    const char *str,                            /**< string */
    unsigned int len                            /**< length */
);


#endif /* REGREG_ACYCLIC_H */
