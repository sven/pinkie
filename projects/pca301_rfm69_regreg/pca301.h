/**
 * @brief PCA301 Remote Power Outlet Driver
 *
 * This driver was created by extensively reading source code and forum
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
#ifndef PCA301_H
#define PCA301_H

#include <pinkie.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define PCA301_CMD_POLL                     4   /* command poll */
#define PCA301_CMD_SWITCH                   5   /* command switch */
#define PCA301_CMD_IDENT                    6   /* command identify (blink) */
#define PCA301_CMD_PAIR                     17  /* command pair */

#define PCA301_CMD_POLL_STATS_RESET         1   /* reset statistics */

#define PCA301_CMD_SWITCH_OFF               0   /* switch state off */
#define PCA301_CMD_SWITCH_ON                1   /* switch state on */

#define PCA301_REGREG_REG_ADDR              offsetof(PCA301_REGREG_T, addr)
#define PCA301_REGREG_REG_CHAN              offsetof(PCA301_REGREG_T, chan)
#define PCA301_REGREG_REG_RSSI              offsetof(PCA301_REGREG_T, rssi)
#define PCA301_REGREG_REG_CMD               offsetof(PCA301_REGREG_T, cmd)
#define PCA301_REGREG_REG_CONS              offsetof(PCA301_REGREG_T, cons)
#define PCA301_REGREG_REG_CONS_TOT          offsetof(PCA301_REGREG_T, cons_tot)

#define PCA301_REGREG_CMD_NONE              0   /* no command */
#define PCA301_REGREG_CMD_POLL              1   /* query state incl. consumption */
#define PCA301_REGREG_CMD_ON                2   /* set state on */
#define PCA301_REGREG_CMD_OFF               3   /* set state off */
#define PCA301_REGREG_CMD_IDENT             4   /* identify (blink) */
#define PCA301_REGREG_CMD_TIMEOUT_RX        5   /* receive timeout */
#define PCA301_REGREG_CMD_PAIR              6   /* pairing (channel update) */
#define PCA301_REGREG_CMD_SEND_BUDGET       7   /* send time limit reached */
#define PCA301_REGREG_CMD_TIMEOUT_TX        8   /* send timeout */
#define PCA301_REGREG_CMD_STATS_RESET       9   /* reset statistics */

#define PCA301_CRC_POLY                0x8005   /* CRC polynom */

#define PCA301_CHAN_NONE                    0   /* channel not set */

#define PCA301_ADDR_LEN                     3   /* address length */

#define PCA301_ID_STATION              0xffff   /* station id */
#define PCA301_ID_STATION_MONITOR      0xaaaa   /* monitor id */

#define PCA301_DFL_CHAN                     1   /* default channel */
#define PCA301_DFL_TIMEOUT_RES_MS         500   /* default response timeout */
#define PCA301_DFL_FLG_PAIR                 0   /* pairing enable flag */
#define PCA301_DFL_FLG_POLL_AUTO            1   /* auto-poll on switch detect flag */
#define PCA301_DFL_RETRIES                  2   /* resend retries */
#define PCA301_DFL_FLG_FRAME_DUMP           0   /* frame dump enable flag */


/*****************************************************************************/
/* Data types */
/*****************************************************************************/
/**< PCA301 frame */
typedef struct {
    uint8_t chan;                               /**< channel */
    uint8_t cmd;                                /**< command */
    uint8_t addr[3];                            /**< address */
    uint8_t data;                               /**< data */
    uint16_t cons_be16;                         /**< current consumption */
    uint16_t cons_tot_be16;                     /**< total consumption */
    uint16_t crc16_be16;                        /**< CRC16 */
} __attribute__((packed)) PCA301_FRAME_T;


/**< PCA301 RegReg mapping */
typedef struct {
    /* pca301 */
    uint8_t addr[PCA301_ADDR_LEN];              /**< [rr:0-2] addr */
    uint8_t chan;                               /**< [rr:3] channel id */
    uint16_t cons;                              /**< [rr:4-5] current consumption */
    uint16_t cons_tot;                          /**< [rr:6-7] total consumption */
    int8_t rssi;                                /**< [rr:8] rssi */
    uint8_t cmd;                                /**< [rr:9] command */

    /* pca301_common */
    uint16_t stat_rx;                           /**< [rr:0-1] stats: RX frames */
    uint16_t stat_rx_crc_inval;                 /**< [rr:2-3] stats: RX invalid CRC */
    uint16_t stat_rx_tout;                      /**< [rr:4-5] stats: RX timeout */
    uint16_t stat_tx;                           /**< [rr:6-7] stats: TX frames */
    uint16_t stat_tx_err;                       /**< [rr:8-9] stats: TX errors */
    uint16_t stat_tx_tout;                      /**< [rr:10-11] stats: TX timeout */
    uint8_t flg_pair_ena;                       /**< [rr:12] pairing enable flag */
    uint8_t chan_dfl;                           /**< [rr:13] default channel */
    uint16_t tout_res;                          /**< [rr:14-15] response timeout */
    uint8_t retries;                            /**< [rr:16] retry attempts on timeout */
    uint8_t flg_poll_auto;                      /**< [rr:17] poll socket if switch is detected */
    uint8_t flg_frame_dump;                     /**< [rr:18] dump frames */
} __attribute__((packed)) PCA301_REGREG_T;


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
void pca301_init(
    uint16_t rr_base                            /**< regreg base address */
);

void pca301_dump(
    PCA301_FRAME_T *pca301                      /**< PCA301 data */
);

void pca301_recv(
    PCA301_FRAME_T *data,                       /**< received data */
    int8_t rssi                                 /**< Receive Signal Strength Indicator */
);

PINKIE_RES_T pca301_plat_send(
    PCA301_FRAME_T *pca301                      /**< PCA301 data */
);

PINKIE_RES_T pca301_send(
    uint8_t *id,                                /**< id pointer*/
    uint8_t chan,                               /**< channel */
    uint8_t cmd,                                /**< command */
    uint8_t data                                /**< data */
);

void pca301_process(
    void
);


#endif /* PCA301_H */
