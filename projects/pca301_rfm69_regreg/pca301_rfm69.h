/**
 * @brief PCA301 RFM69 Adapter
 *
 * This library was created by extensively reading source code and forum
 * comments contributed by other authors. Thank you.
 *
 * It is currently only tested on the Arduino Nano with the RFM69 transmitter
 * connected by SPI. Thanks to this guide (german)
 * https://steigerbalett.wordpress.com/2015/05/23/jeelink-clone-loten-und-mit-einer-firmware-flashen-fur-lacrosse-sensoren-in-fhem/
 * I was able to wire them together.
 *
 * Copyright (c) 2017-2018, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PCA301_RFM69_H
#define PCA301_RFM69_H

#include "pca301.h"


/*****************************************************************************/
/* Persistent data */
/*****************************************************************************/
typedef struct {
    uint32_t freq_carrier_khz;                  /**< carrier frequency in kHz */
    uint16_t bitrate_bs;                        /**< bitrate in b/s */
    uint8_t rssi_threshold;                     /**< RSSI threshold */
    uint16_t fdev_hz;                           /**< freq deviation in Hz */
} PCA301_RFM69_NVS_T;


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
void pca301_rfm69_init(
    unsigned int flg_nvs_valid,                 /**< NVS data valid flag */
    PCA301_RFM69_NVS_T *nvs                     /**< NVS data */
);

void pca301_rfm69_process(
    void
);


#endif /* PCA301_RFM69_H */
