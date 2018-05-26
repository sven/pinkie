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
#include <pinkie.h>
#include <drv/radio/rfm69/radio_rfm69.h>
#include "pca301_rfm69.h"


/*****************************************************************************/
/* Local defines */
/*****************************************************************************/
#define PCA301_FREQ_CARRIER_KHZ     ((uint32_t) 868950)
#define PCA301_BITRATE_BS           6631
#define PCA301_RSSI_THRESHOLD       -114
#define PCA301_FREQ_DEV_HZ          45000


/*****************************************************************************/
/* Local variables */
/*****************************************************************************/
static uint8_t pca301_sync_values[] = { 0x2d, 0xd4 }; /**< sync word values */


/*****************************************************************************/
/** RFM69 Initialization
 */
void pca301_rfm69_init(
    unsigned int flg_nvs_valid,                 /**< NVS data valid flag */
    PCA301_RFM69_NVS_T *nvs                     /**< NVS data */
)
{
    /* initialize NVS if not valid */
    if (!flg_nvs_valid) {
        nvs->freq_carrier_khz = PCA301_FREQ_CARRIER_KHZ;
        nvs->bitrate_bs = PCA301_BITRATE_BS;
        nvs->rssi_threshold = PCA301_RSSI_THRESHOLD;
        nvs->fdev_hz = PCA301_FREQ_DEV_HZ;
    }

    /* put transceiver in standby mode */
    rfm69_opmode_set(RFM69_OPMODE_STANDBY);

    /* frequency: 868.950 MHz */
    rfm69_freq_carrier_khz(nvs->freq_carrier_khz);

    /* bitrate: 6.631 kb/s */
    rfm69_bitrate_bs(nvs->bitrate_bs);

    /* configure RX and TX interrupt generators */
    rfm69_dio_mapping_rx(0, RFM69_DIO0_RX_PAYLOADREADY_TX_TXREADY);
    rfm69_dio_mapping_tx(0, RFM69_DIO0_RX_CRCOK_TX_PACKETSENT);

    /* disable CLKOUT to save power */
    rfm69_clkout(RFM69_CLKOUT_OFF);

    /* configure CRC */
    rfm69_crc_on(0);
    rfm69_crc_auto_clear_off(1);

    /* set payload length */
    rfm69_payload_length(12);

    /* configure sync word */
    rfm69_sync_word(2, pca301_sync_values);
    rfm69_sync_on(1);

    /* RX bandwidth exponent */
    rfm69_rx_bw_exp(2);

    /* RSSI threshold */
    rfm69_rssi_threshold(nvs->rssi_threshold);

    /* variable length packet format */
    rfm69_packet_format_var_len(0);

    /* TX start condition */
    rfm69_tx_start_cond(RFM69_FIFO_NOT_EMPTY);

    /* set frequency deviation in Hz */
    rfm69_fdev_hz(nvs->fdev_hz);

    /* enable receiver mode */
    rfm69_opmode_set(RFM69_OPMODE_RX);

    /* clear fifo */
    rfm69_fifo_clear();
}


/*****************************************************************************/
/** RFM69 Data Processor
 */
void pca301_rfm69_process(
    void
)
{
    static PCA301_FRAME_T frame;                /* PCA301 frame */
    static int rssi = 0;                        /* RSSI */
    uint8_t cnt;                                /* counter */

    if (rfm69_flg_isr) {
        rssi = rfm69_rssi_value(0);
    }

    for (cnt = 0; rfm69_rx_avail() && (cnt < sizeof(frame)); cnt++) {
        ((uint8_t *) &frame)[cnt] = rfm69_fifo_data();
    }

    if (sizeof(frame) <= cnt) {
        pca301_recv(&frame, rssi);
    }
}


/*****************************************************************************/
/** PCA301 Send Frame
 */
PINKIE_RES_T pca301_plat_send(
    PCA301_FRAME_T *pca301                      /**< PCA301 data */
)
{
    return rfm69_send((uint8_t *) pca301, sizeof(PCA301_FRAME_T));
}
