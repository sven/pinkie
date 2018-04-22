/**
 * @brief PINKIE - Microchip ATmega8 Architecture
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <avr/io.h>
#include <pinkie_stdio.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define BAUD 9600

/* http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_wrong_baud_rate */
#define UBRR_VAL ((F_CPU + BAUD * 8L) / (BAUD * 16L) - 1)


/*****************************************************************************/
/** PINKIE STDIO Init
 *
 * Initialize standard I/O interface.
 */
int pinkie_stdio_init(
    void
)
{
    /* initialize UART */
    UBRRH = (unsigned char) (UBRR_VAL >> 8);
    UBRRL = (unsigned char) (UBRR_VAL & 0xff);
    UCSRB = (1 << RXEN | 1 << TXEN);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);

    return 0;
}


/*****************************************************************************/
/** PINKIE STDIO Put Character
 *
 * Writes a character to the standard output. This function blocks until the
 * character can be send.
 */
void pinkie_stdio_putc(
    char c
)
{
    while (!(UCSRA & (1 << UDRE)));
    UDR = c;
}


/*****************************************************************************/
/** PINKIE STDIO Get Character
 *
 * Reads a character from the standard input. This function blocks until a
 * character is available.
 */
char pinkie_stdio_getc(
    void
)
{
    while (!(UCSRA & (1 << RXC)));
    return UDR;
}
