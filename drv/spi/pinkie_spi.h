/**
 * @brief PINKIE - SPI Interface
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_SPI_H
#define PINKIE_SPI_H


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
void pinkie_spi_init(
    void
);

int pinkie_spi_xfer(
    const char *src,                            /**< send data */
    char *dst,                                  /**< received data */
    unsigned int len,                           /**< data length */
    uint8_t flg_sel                             /**< slave select ctrl flag */
);

void pinkie_spi_sel_ctrl(
    uint8_t flg_sel                             /**< slave select on */
);


#endif /* PINKIE_SPI_H */
