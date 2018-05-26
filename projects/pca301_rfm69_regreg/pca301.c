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
#include <pinkie.h>
#include <regreg.h>
#include "pca301.h"


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
static unsigned int pca301_regreg(
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
);


/*****************************************************************************/
/* Global variables */
/*****************************************************************************/
PCA301_FRAME_T pca301_frame;                    /**< PCA301 frame data */


/*****************************************************************************/
/* Local variables */
/*****************************************************************************/
static uint64_t pca301_tout;                    /**< PCA301 response timeout */
static uint8_t pca301_addr[PCA301_ADDR_LEN];    /**< PCA301 address */
static uint8_t pca301_chan;                     /**< PCA301 channel id */
static uint8_t pca301_cmd;                      /**< PCA301 command */
static uint8_t pca301_data;                     /**< PCA301 data */
static uint8_t pca301_retries;                  /**< PCA301 retry counter */
static uint8_t pca301_poll_addr[PCA301_ADDR_LEN]; /**< PCA301 auto-poll address */
static uint8_t pca301_poll_chan;                /**< PCA301 auto-poll channel */
static uint8_t pca301_poll_flag;                /**< PCA301 auto-poll flag */

/**< PCA301 register data */
static PCA301_REGREG_T pca301_regreg_data = {
    /* pca301 */
    { 0, 0, 0 },                                /* addr */
    0,                                          /* channel id */
    0,                                          /* consumption */
    0,                                          /* total consumption */
    0,                                          /* rssi */
    0,                                          /* command */

    /* pca301_common */
    0,                                          /* stats: RX frames */
    0,                                          /* stats: RX invalid CRC */
    0,                                          /* stats: RX timeout */
    0,                                          /* stats: TX frames */
    0,                                          /* stats: TX errors */
    0,                                          /* stats: TX timeout */
    PCA301_DFL_FLG_PAIR,                        /* pairing enable flag */
    PCA301_DFL_CHAN,                            /* default channel */
    PCA301_DFL_TIMEOUT_RES_MS,                  /* response timeout */
    PCA301_DFL_RETRIES,                         /* retry attempts on timeout */
    PCA301_DFL_FLG_POLL_AUTO,                   /* poll socket if switch is detected */
    PCA301_DFL_FLG_FRAME_DUMP,                  /* dump frame */
};

static REG_ENTRY_T pca301_regreg_info = {       /**< PCA301 register */
    NULL,
    0,
    sizeof(PCA301_REGREG_T) - 1,
    pca301_regreg,
    &pca301_regreg_data,
};


/*****************************************************************************/
/** PCA301 Initialization
 */
void pca301_init(
    uint16_t rr_base                            /**< regreg base address */
)
{
    /* regreg base address */
    pca301_regreg_info.addr_beg = rr_base;
    pca301_regreg_info.addr_end = rr_base + sizeof(pca301_regreg_data);

    /* create RegReg registers */
    reg_add(&pca301_regreg_info);
}


/*****************************************************************************/
/** PCA301 Data Dumper
 */
void pca301_dump(
    PCA301_FRAME_T *pca301                      /**< PCA301 data */
)
{
    pinkie_stdio_putc('\n');
    pinkie_printf("channel: %u\n", pca301->chan);
    pinkie_printf("command: %u\n", pca301->cmd);
    pinkie_printf("addr: 0x%x 0x%x 0x%x\n", pca301->addr[0], pca301->addr[1], pca301->addr[2]);
    pinkie_printf("data: 0x%x\n", pca301->data);
    pinkie_printf("cons: %u Wh\n", PINKIE_BE16TOH(pca301->cons_be16));
    pinkie_printf("cons_tot: %u kWh\n", PINKIE_BE16TOH(pca301->cons_tot_be16));
    pinkie_printf("crc16: 0x%x\n", PINKIE_BE16TOH(pca301->crc16_be16));
    pinkie_stdio_putc('\n');

    pinkie_printf("command: ");
    switch (pca301->cmd) {

        case PCA301_CMD_POLL:
            pinkie_printf("query, data: %s\n", (PCA301_CMD_SWITCH_ON == pca301->data) ? "on" : "off");
            break;

        case PCA301_CMD_SWITCH:
            pinkie_printf("switch, data: %s\n", (PCA301_CMD_SWITCH_ON == pca301->data) ? "on" : "off");
            break;

        case PCA301_CMD_IDENT:
            pinkie_printf("ident, data: %u\n", pca301->data);
            break;

        case PCA301_CMD_PAIR:
            pinkie_printf("pair, data: %u\n", pca301->data);
            break;

        default:
            pinkie_printf("unknown\n");
    }

    pinkie_stdio_putc('\n');
}


/*****************************************************************************/
/** PCA301 Receive Handler
 */
void pca301_recv(
    PCA301_FRAME_T *pca301,                     /**< PCA301 data */
    int8_t rssi                                 /**< Receive Signal Strength Indicator */
)
{
    uint16_t crc16_be16;                        /* packet CRC */
    uint32_t addr;                              /* address */
    uint8_t cmd;                                /* command */
    uint16_t cons;                              /* consumption */

    /* check CRC of received data and drop invalid frames */
    crc16_be16 = pinkie_crc16((uint8_t *) pca301,
                              sizeof(PCA301_FRAME_T) - sizeof(pca301->crc16_be16),
                              PCA301_CRC_POLY);
    crc16_be16 = PINKIE_HTOBE16(crc16_be16);

    /* process valid frames */
    if (crc16_be16 != pca301->crc16_be16) {

        /* stats: RX invalid CRC */
        pca301_regreg_data.stat_rx_crc_inval++;

        return;
    }

    /* stats: RX frames */
    pca301_regreg_data.stat_rx++;

    /* dump frame content */
    if (pca301_regreg_data.flg_frame_dump) {
        pca301_dump(pca301);
    }

    /* convert address to host endianness */
    addr = PINKIE_BE24TOH(pca301->addr);

    /* report address, channel and rssi to detect channel changes instantly */
    reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_ADDR, &addr, PCA301_ADDR_LEN);
    reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CHAN, &pca301->chan, sizeof(pca301->chan));
    reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_RSSI, &rssi, sizeof(rssi));

    /* handle command */
    switch (pca301->cmd) {

        case PCA301_CMD_PAIR:

            /* 1. if no channel is assigned and pairing is enabled we set our default channel
             * 2. if no channel is assigned and pairing is disable we do nothing
             * 3. if a channel is assigned we just report it but do not pair
             */
            if (PCA301_CHAN_NONE == pca301->chan) {

                if (!pca301_regreg_data.flg_pair_ena) {
                    /* case 2: ignore device */
                    return;
                }

                /* case 1: set default channel */
                pca301->chan = pca301_regreg_data.chan_dfl;

                /* pair device */
                pca301_send(pca301->addr, pca301->chan, PCA301_CMD_PAIR, 0);
            }

            /* case 1 & case 3 */
            cmd = PCA301_REGREG_CMD_PAIR;
            reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CMD, &cmd, sizeof(cmd));

            break;

        case PCA301_CMD_POLL:

            /* check timeout */
            if (pinkie_timer_get() >= pca301_tout) {
                return;
            }

            /* ignore frames from other stations */
            if (((PCA301_ID_STATION == pca301->cons_be16) && (PCA301_ID_STATION == pca301->cons_tot_be16)) ||
                ((PCA301_ID_STATION_MONITOR == pca301->cons_be16) && (PCA301_ID_STATION_MONITOR == pca301->cons_tot_be16))) {

                return;
            }

            /* clear timeout */
            pca301_tout = 0;

            cmd = pca301->data ? PCA301_REGREG_CMD_ON : PCA301_REGREG_CMD_OFF;
            reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CMD, &cmd, sizeof(cmd));

            cons = PINKIE_BE16TOH(pca301->cons_be16);
            reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CONS, &cons, sizeof(cons));

            cons = PINKIE_BE16TOH(pca301->cons_tot_be16);
            reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CONS_TOT, &cons, sizeof(cons));

            pinkie_printf("pca301: poll, addr = 0x%02x%02x%02x, state = %u, cons: %u, cons_tot: %u, rssi: %i\n",
                          pca301->addr[0], pca301->addr[1], pca301->addr[2],
                          pca301->data, PINKIE_BE16TOH(pca301->cons_be16), PINKIE_BE16TOH(pca301->cons_tot_be16), rssi);

            break;

        case PCA301_CMD_SWITCH:

            /* check timeout and if ACK is for our request */
            if ((pinkie_timer_get() >= pca301_tout)
                || (memcmp(&pca301_frame, pca301, (char *) &pca301->cons_be16 - (char *) pca301))) {

                /* switch was not initiated by us and we can't detect if its
                 * on/off by the data part because the frame format is a bit
                 * confusing as the device seems to reverse the data part if
                 * the switch is initiated by the button instead of the switch
                 * request from a station */
                if (pca301_regreg_data.flg_poll_auto) {
                    memcpy(pca301_poll_addr, pca301->addr, sizeof(pca301_poll_addr));
                    pca301_poll_chan = pca301->chan;
                    pca301_poll_flag = 1;
                }

                return;
            }

            /* clear timeout */
            pca301_tout = 0;

            cmd = pca301->data ? PCA301_REGREG_CMD_ON : PCA301_REGREG_CMD_OFF;
            reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CMD, &cmd, sizeof(cmd));

            pinkie_printf("pca301: switch ack, addr = 0x%02x%02x%02x, state = %u, rssi = %i\n",
                          pca301->addr[0], pca301->addr[1], pca301->addr[2],
                          pca301->data, rssi);

            break;
    }
}


/*****************************************************************************/
/** PCA301 Send To Id
 */
PINKIE_RES_T pca301_send(
    uint8_t *id,                                /**< id pointer*/
    uint8_t chan,                               /**< channel */
    uint8_t cmd,                                /**< command */
    uint8_t data                                /**< data */
)
{
    PINKIE_RES_T res;                           /* result */

    pca301_frame.chan = chan;
    pca301_frame.cmd = cmd;
    memcpy(pca301_frame.addr, id, PCA301_ADDR_LEN);
    pca301_frame.data = data;
    pca301_frame.cons_be16 = PCA301_ID_STATION;
    pca301_frame.cons_tot_be16 = PCA301_ID_STATION;
    pca301_frame.crc16_be16 = pinkie_crc16((uint8_t *) &pca301_frame,
                                           sizeof(PCA301_FRAME_T) - sizeof(pca301_frame.crc16_be16),
                                           PCA301_CRC_POLY);
    pca301_frame.crc16_be16 = PINKIE_HTOBE16(pca301_frame.crc16_be16);
    pca301_dump(&pca301_frame);
    res = pca301_plat_send(&pca301_frame);

    /* handle send errors */
    if (PINKIE_OK == res) {

        /* stats: TX frames */
        pca301_regreg_data.stat_tx++;

    } else {

        /* stats: TX send errors */
        pca301_regreg_data.stat_tx_err++;

        /* no allowed send time */
        if (PINKIE_ERR_NO_BUDGET == res) {
            reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CMD, (uint8_t[]){ PCA301_REGREG_CMD_SEND_BUDGET }, sizeof(uint8_t));
        }
        else if (PINKIE_ERR_TIMEOUT == res) {

            /* increase timeout statistic */
            pca301_regreg_data.stat_tx_tout++;

            reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CMD, (uint8_t[]){ PCA301_REGREG_CMD_TIMEOUT_TX }, sizeof(uint8_t));
        }
    }

    return res;
}


/*****************************************************************************/
/** PCA301 RegReg Handler
 */
static unsigned int pca301_regreg(
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
)
{
    PINKIE_RES_T res;                           /* result */

    PINKIE_UNUSED(reg);

    /* if a transfer is already in progress deny access */
    if ((pca301_tout) && (pinkie_timer_get() < pca301_tout)) {
        return REGREG_RES_BUSY;
    }

    /* read access is handled by regreg */
    if (!reg_acc->write_flg) {
        return REGREG_RES_PROCEED;
    }

    if (PCA301_REGREG_REG_CMD != reg_acc->addr_ofs) {
        return REGREG_RES_PROCEED;
    }

    /* copy state so it won't get overwritten by RegReg */
    memcpy(pca301_addr, pca301_regreg_data.addr, sizeof(pca301_addr));
    pca301_chan = pca301_regreg_data.chan;

    switch (*reg_acc->data.read_from) {

        case PCA301_REGREG_CMD_ON:
            pinkie_printf("pca301: cmd = switch on\n");
            pca301_cmd = PCA301_CMD_SWITCH;
            pca301_data = PCA301_CMD_SWITCH_ON;
            pca301_tout = pinkie_timer_get() + (uint64_t) pca301_regreg_data.tout_res;
            pca301_retries = pca301_regreg_data.retries;
            break;

        case PCA301_REGREG_CMD_OFF:
            pinkie_printf("pca301: cmd = switch off\n");
            pca301_cmd = PCA301_CMD_SWITCH;
            pca301_data = PCA301_CMD_SWITCH_OFF;
            pca301_tout = pinkie_timer_get() + (uint64_t) pca301_regreg_data.tout_res;
            pca301_retries = pca301_regreg_data.retries;
            break;

        case PCA301_REGREG_CMD_IDENT:
            pinkie_printf("pca301: cmd = identify (blink)\n");
            pca301_cmd = PCA301_CMD_IDENT;
            pca301_data = 0;
            break;

        case PCA301_REGREG_CMD_POLL:
            pinkie_printf("pca301: cmd = poll\n");
            pca301_cmd = PCA301_CMD_POLL;
            pca301_data = 0;
            pca301_tout = pinkie_timer_get() + (uint64_t) pca301_regreg_data.tout_res;
            pca301_retries = pca301_regreg_data.retries;
            break;

        case PCA301_REGREG_CMD_STATS_RESET:
            pinkie_printf("pca301: cmd = stats reset\n");
            pca301_cmd = PCA301_CMD_POLL;
            pca301_data = PCA301_CMD_POLL_STATS_RESET;
            pca301_tout = pinkie_timer_get() + (uint64_t) pca301_regreg_data.tout_res;
            pca301_retries = pca301_regreg_data.retries;
            break;

        default:
            pinkie_printf("pca301: unknown cmd\n");
            return 0;
    }

    /* transmit command */
    res = pca301_send(pca301_addr, pca301_chan, pca301_cmd, pca301_data);
    if (PINKIE_OK != res) {
        pca301_tout = 0;
    }

    return REGREG_RES_PROCEED;
}


/*****************************************************************************/
/** PCA301 Timeout Handler
 */
void pca301_process(
    void
)
{
    PINKIE_RES_T res;                           /* result */
    uint32_t addr;                              /* address */

    if ((pca301_tout) && (pinkie_timer_get() >= pca301_tout)) {

        if (0 != pca301_retries) {

            /* decrease retry count */
            pca301_retries--;

            /* re-arm timeout */
            pca301_tout = pinkie_timer_get() + (uint64_t) pca301_regreg_data.tout_res;

            /* re-transmit command */
            res = pca301_send(pca301_addr, pca301_chan, pca301_cmd, pca301_data);
            if (PINKIE_OK != res) {
                pca301_tout = 0;
            }

            return;
        }

        /* clear timeout */
        pca301_tout = 0;

        /* increase timeout statistic */
        pca301_regreg_data.stat_rx_tout++;

        /* convert address to host endianness */
        addr = PINKIE_BE24TOH(pca301_regreg_data.addr);

        /* inform about timeout */
        reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_ADDR, &addr, PCA301_ADDR_LEN);
        reg_ann(pca301_regreg_info.addr_beg + PCA301_REGREG_REG_CMD, (uint8_t[]){ PCA301_REGREG_CMD_TIMEOUT_RX }, sizeof(uint8_t));

        return;
    }

    /* poll device if a uninitiated switch was detected */
    if (pca301_poll_flag) {

        pinkie_printf("pca301: cmd = auto-poll\n");
        memcpy(pca301_addr, pca301_poll_addr, sizeof(pca301_addr));
        pca301_chan = pca301_poll_chan;
        pca301_cmd = PCA301_CMD_POLL;
        pca301_data = 0;
        pca301_tout = pinkie_timer_get() + (uint64_t) pca301_regreg_data.tout_res;
        pca301_retries = pca301_regreg_data.retries;

        /* clear auto-poll request */
        pca301_poll_flag = 0;

        /* transmit command */
        res = pca301_send(pca301_addr, pca301_chan, pca301_cmd, pca301_data);
        if (PINKIE_OK != res) {
            pca301_tout = 0;
        }

        return;
    }
}
