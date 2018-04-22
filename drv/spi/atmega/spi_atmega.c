/**
 * @brief PINKIE - ATmega SPI Driver
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <avr/io.h>
#include <util/delay.h>
#include <drv/spi/pinkie_spi.h>
#include <pinkie_stdio.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define PINKIE_SPI_ATMEGA_TIMEOUT       1000    /* SPI transfer timeout in ms */


/*****************************************************************************/
/** SPI Initialization
 *
 * Note: If !SS isn't configured as output it happens that SPI enable switches
 *       to slave mode.
 */
void pinkie_spi_init(
    void
)
{
    /* configure outputs (MOSI, !SS, SCK) */
    DDRB |= (1 << DDB3) | (1 << DDB2) | (1 << DDB5);

    /* configure input (MISO) */
    DDRB &= ~(1 << DDB4);

    /* configure SPI (SPI en, master) */
    SPCR = (1 << SPE) | (1 << MSTR);
}


/*****************************************************************************/
/** SPI transfer
 *
 * Src or dst can be NULL if not needed.
 */
int pinkie_spi_xfer(
    const char *src,                            /**< send data */
    char *dst,                                  /**< received data */
    unsigned int len,                           /**< data length */
    uint8_t flg_sel                             /**< slave select ctrl flag */
)
{
    unsigned int cnt;                           /* counter */

    /* set slave select to low */
    if (flg_sel) {
        pinkie_spi_sel_ctrl(1);
    }

    while (len--) {

        /* send byte */
        SPDR = (src) ? *src++ : 0x00;

        /* wait for transfer ready */
        for (cnt = 0; cnt < PINKIE_SPI_ATMEGA_TIMEOUT; cnt++) {
            if (0 != (SPSR & (1 << SPIF))) {
                break;
            }
            _delay_ms(1);
        }

        if (PINKIE_SPI_ATMEGA_TIMEOUT <= cnt) {
            pinkie_printf("SPI timeout\n");
            return 1;
        }

        /* receive byte */
        if (dst) {
            *dst++ = SPDR;
        }
    }

    /* set slave select to high */
    if (flg_sel) {
        pinkie_spi_sel_ctrl(0);
    }

    return 0;
}


/*****************************************************************************/
/** SPI slave select control
 */
void pinkie_spi_sel_ctrl(
    uint8_t flg_sel                             /**< slave select on */
)
{
    if (flg_sel) {
        PORTB &= ~(1 << PB2);
    } else {
        PORTB |= (1 << PB2);
    }
}
