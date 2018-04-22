/**
 * @brief PINKIE - ATmega Timer Driver
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define ATMEGA_TIMER_PRESCALE   64              /**< prescaler */
#define ATMEGA_TIMER_CMP        0xfa            /**< 250 is about 1 ms */


/*****************************************************************************/
/* Variables */
/*****************************************************************************/
static uint64_t ms = 0;                         /**< milliseconds */


/*****************************************************************************/
/** Timer Initialization
 *
 * For 16.000.000 MHz a prescaler of 64 is used to get 1 ms:
 *   1 s = 16.000.000 / 64 = 250000
 *   1 ms = 250000 / 1000 = 250
 *
 * Note: Enabling interrupts must be done by the application.
 */
void pinkie_timer_init(
    void
)
{
    /* set timer to auto reload */
    TCCR0A = (1 << WGM01);

    /* set trigger value to 250 */
    OCR0A = 250 - 1;

    /* enable compare-match interrupt */
    TIMSK0 = (1 << OCIE0A);

    /* set the prescaler to 64 (for 1 ms on 16 MHz) */
    TCCR0B = (1 << CS01) | (1 << CS00);
}


/*****************************************************************************/
/** Timer IRQ
 *
 * Src or dst can be NULL if not needed.
 */
ISR (TIMER0_COMPA_vect)
{
    ms++;
}


/*****************************************************************************/
/** Get current millisconds
 */
uint64_t pinkie_timer_get(
    void
)
{
    uint64_t ms_copy;                           /* ms temp copy */

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ms_copy = ms;
    }

    return ms_copy;
}


/*****************************************************************************/
/** Set current millisconds
 */
void pinkie_timer_set(
    uint64_t ms_copy                            /* new ms */
)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ms = ms_copy;
    }
}
