/**
 * @brief PINKIE - Timer Interface
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_TIMER_H
#define PINKIE_TIMER_H


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
void pinkie_timer_init(
    void
);

uint64_t pinkie_timer_get(
    void
);

void pinkie_timer_set(
    uint64_t ms_copy                            /* new ms */
);


#endif /* PINKIE_TIMER_H */
