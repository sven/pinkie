/**
 * @brief PINKIE - ATmega NVS Driver
 *
 * Notes:
 *   - the NVS CRC16 was taken from
 *     https://users.ece.cmu.edu/~koopman/crc/index.html
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */

#include <drv/nvs/pinkie_nvs.h>
#include <avr/eeprom.h>

/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define PINKIE_NVS_POLY                 0xed2f  /**< NVS CRC polynomial */


/*****************************************************************************/
/** PINKIE NVS Read
 *
 * @retval 0 NVS data valid (CRC matches)
 * @retval other NVS data invalid
 */
int pinkie_nvs_read(
    uint8_t *data,                              /**< NVS data ptr */
    unsigned int len                            /**< NVS data length */
)
{
    uint16_t crc;                               /* calculated CRC */

    /* read EEPROM data */
    eeprom_read_block(data, 0, len);

    /* skip CRC part and calculate CRC from read data */
    crc = pinkie_crc16(&data[2], len - 2, PINKIE_NVS_POLY);

    /* compare CRC values */
    return (crc == *((uint16_t *) data));
}


/*****************************************************************************/
/** PINKIE NVS Write
 */
void pinkie_nvs_write(
    uint8_t *data,                              /**< NVS data ptr */
    unsigned int len                            /**< NVS data length */
)
{
    /* skip CRC part and calculate CRC from read data */
    *((uint16_t *) data) = pinkie_crc16(&data[2], len - 2, PINKIE_NVS_POLY);

    /* write EEPROM data */
    eeprom_write_block(data, 0, len);
}
