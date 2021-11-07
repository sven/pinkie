/**
 * @brief PINKIE - Microchip ATmega328 Architecture
 *
 * Info:
 *   - UART is non-blocking and has 50 bytes buffer
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <pinkie.h>
#include <avr/interrupt.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define BAUD 57600

/* http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_wrong_baud_rate */
#define UBRR_VAL ((F_CPU + BAUD * 8L) / (BAUD * 16L) - 1)

#define PINKIE_ARCH_UART_BUF_SIZE           50  /**< UART ringbuffer size */


/*****************************************************************************/
/* Local variables */
/*****************************************************************************/
static char rb_uart[PINKIE_ARCH_UART_BUF_SIZE]; /**< UART ringbuffer */
static uint8_t volatile rb_uart_rd = 0;         /**< UART read index */
static uint8_t volatile rb_uart_wr = 1;         /**< UART write index */


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
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);
    UBRR0H = (UBRR_VAL >> 8);
    UBRR0L = UBRR_VAL;

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
    if ('\n' == c) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = '\r';
    }

    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}


/*****************************************************************************/
/** PINKIE STDIO Check For Data
 *
 * Check non-blocking if a new character is available.
 */
int pinkie_stdio_avail(
    void
)
{
    /* check for empty ringbuffer */
    if (((rb_uart_rd + 1) % PINKIE_ARCH_UART_BUF_SIZE) == rb_uart_wr) {
        return 0;
    }

    return 1;
}


/*****************************************************************************/
/** PINKIE UART ISR
 */
ISR(USART_RX_vect)
{
    if (rb_uart_wr != rb_uart_rd) {
        rb_uart[rb_uart_wr++] = UDR0;
        rb_uart_wr %= PINKIE_ARCH_UART_BUF_SIZE;
    }
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
    uint8_t rd;

    do {
        rd = (rb_uart_rd + 1) % PINKIE_ARCH_UART_BUF_SIZE;
    } while (rd == rb_uart_wr);

    /* return character */
    rb_uart_rd = rd;

    return rb_uart[rb_uart_rd];
}


/*****************************************************************************/
/** PINKIE Arch Init Finish
 *
 * Finalizes the architecture initialization like enabling IRQs.
 */
void pinkie_arch_init_fin(
    void
)
{
    /* enable interrupts */
    sei();
}
