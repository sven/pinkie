/**
 * @brief PINKIE - RFM69 Radio Driver
 *
 * Just another RFM69 library but currently only with enough functionality to
 * setup a ELV PCA301 compatible communication.
 *
 * This library was created by extensively reading source code and forum
 * comments contributed by other authors. Thank you.
 *
 * Warning: Try to avoid accessing the RFM69 registers (IRQFLAGS, FIFO) during
 * the ISR. This may prevent the timer ISR and therefore the timeout in the
 * access functions from working.
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef RADIO_RFM69_H
#define RADIO_RFM69_H


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define RFM69_UNIT_KILO                             1000
#define RFM69_UNIT_MEGA                             1000000

#define RFM69_FREQ_FXOSC_HZ                         (32.0 * RFM69_UNIT_MEGA)
#define RFM69_FREQ_FSTEP_HZ                         (RFM69_FREQ_FXOSC_HZ / 524288)

#define RFM69_TIMEOUT_MS                            ((uint64_t) 200)

/* 1 percent of 1000 ms * 60 sec * 60 min */
#define RFM69_TIME_BUDGET_MS                        ((uint16_t) 36000)

/* time recovering per millisecond (36000 ms allowed per hour / 60 * 60 * 1000) */
#define RFM69_TIME_BUDGET_RECOVER_MS                ((uint16_t) 10)

/* minimal time needed to allow a send request (1 second) */
#define RFM69_TIME_BUDGET_MIN_MS                    ((uint16_t) 3600)

/* remove at least 1 ms from time budget */
#define RFM69_TIME_BUDGET_EXTRA_MS                  ((uint16_t) 1)


/*****************************************************************************/
/* SPI */
/*****************************************************************************/
#define SPI_WRITE                                   0x80


/*****************************************************************************/
/* 0x00 RegFifo */
/*****************************************************************************/
#define RFM69_REG_FIFO                              0x00


/*****************************************************************************/
/* 0x01 RegOpMode */
/*****************************************************************************/
#define RFM69_REG_OPMODE                            0x01
#define RFM69_MSK_OPMODE_MODE                       0x07
#define RFM69_SHF_OPMODE_MODE                       2

#define RFM69_OPMODE_STANDBY                        0x01
#define RFM69_OPMODE_TX                             0x03
#define RFM69_OPMODE_RX                             0x04


/*****************************************************************************/
/* 0x03 RegBitrateMsb */
/* 0x04 RegBitrateLsb */
/*****************************************************************************/
#define RFM69_REG_BITRATEMSB                        0x03
#define RFM69_REG_BITRATELSB                        0x04


/*****************************************************************************/
/* 0x05 RegFdevMsb */
/* 0x06 RegFdevLsb */
/*****************************************************************************/
#define RFM69_REG_FDEVMSB                           0x05
#define RFM69_REG_FDEVLSB                           0x06

#define RFM69_MSK_FDEVMSB_FDEV                      0x3f
#define RFM69_SHF_FDEVMSB_FDEV                      0

#define RFM69_MSK_FDEVLSB_FDEV                      0xff
#define RFM69_SHF_FDEVLSB_FDEV                      0


/*****************************************************************************/
/* 0x07 RegFrfMsb */
/* 0x08 RegFrfMid */
/* 0x09 RegFrfLsb */
/*****************************************************************************/
#define RFM69_REG_FRFMSB                            0x07
#define RFM69_REG_FRFMID                            0x08
#define RFM69_REG_FRFLSB                            0x09

/*****************************************************************************/
/* 0x0a RegOsc1 */
/*****************************************************************************/
#define RFM69_REG_OSC1                              0x0a

#define RFM69_MSK_OSC1_RCCALSTART                   1
#define RFM69_SHF_OSC1_RCCALSTART                   7
#define RFM69_OSC1_RCCALSTART                       1

#define RFM69_MSK_OSC1_RCCALDONE                    1
#define RFM69_SHF_OSC1_RCCALDONE                    6
#define RFM69_OSC1_RCCALDONE                        1


/*****************************************************************************/
/* 0x11 RegPaLevel */
/*****************************************************************************/
#define RFM69_REG_PALEVEL                           0x11

#define RFM69_MSK_PALEVEL_PA_ON                     0x07
#define RFM69_SHF_PALEVEL_PA_ON                     5

#define RFM69_MSK_PALEVEL_OUTPUTPOWER               0x1f
#define RFM69_SHF_PALEVEL_OUTPUTPOWER               0

#define RFM69_PA_0_ON                               0x04
#define RFM69_PA_1_ON                               0x02
#define RFM69_PA_2_ON                               0x01
#define RFM69_PA_POUT_MAX                           RFM69_MSK_PALEVEL_OUTPUTPOWER


/*****************************************************************************/
/* 0x13 RegOcp */
/*****************************************************************************/
#define RFM69_REG_OCP                               0x13

#define RFM69_MSK_OCP_OCP_ON                        0x01
#define RFM69_SHF_OCP_OCP_ON                        4


/*****************************************************************************/
/* 0x19 RegRxBw */
/*****************************************************************************/
#define RFM69_REG_RXBW                              0x19

#define RFM69_MSK_RXBW_RXBWEXP                      0x07
#define RFM69_SHF_RXBW_RXBWEXP                      0


/*****************************************************************************/
/* 0x23 RegRssiConfig */
/* 0x24 RegRssiValue */
/*****************************************************************************/
#define RFM69_REG_RSSICONFIG                        0x23
#define RFM69_REG_RSSIVALUE                         0x24

#define RFM69_MSK_RSSICONFIG_RSSIDONE               0x01
#define RFM69_SHF_RSSICONFIG_RSSIDONE               1
#define RFM69_RSSICONFIG_RSSIDONE                   1

#define RFM69_MSK_RSSICONFIG_RSSISTART              0x01
#define RFM69_SHF_RSSICONFIG_RSSISTART              0
#define RFM69_RSSICONFIG_RSSISTART                  1


/*****************************************************************************/
/* 0x25 RegDioMapping1 */
/* 0x26 RegDioMapping2 */
/*****************************************************************************/
#define RFM69_REG_DIOMAPPING1                       0x25
#define RFM69_REG_DIOMAPPING2                       0x26
#define RFM69_MSK_DIOMAPPING                        3

#define RFM69_DIO0_RX_CRCOK_TX_PACKETSENT           0x00
#define RFM69_DIO0_RX_PAYLOADREADY_TX_TXREADY       0x01

#define RFM69_MSK_DIOMAPPING2_CLKOUT                0x07
#define RFM69_SHF_DIOMAPPING2_CLKOUT                0
#define RFM69_CLKOUT_OFF                            0x07


/*****************************************************************************/
/* 0x27 RegIrqFlags1 */
/*****************************************************************************/
#define RFM69_REG_IRQFLAGS1                         0x27

#define RFM69_MSK_IRQFLAGS1_MODEREADY               0x01
#define RFM69_SHF_IRQFLAGS1_MODEREADY               7

#define RFM69_MSK_IRQFLAGS1_RXREADY                 0x01
#define RFM69_SHF_IRQFLAGS1_RXREADY                 6

#define RFM69_MSK_IRQFLAGS1_TXREADY                 0x01
#define RFM69_SHF_IRQFLAGS1_TXREADY                 5


/*****************************************************************************/
/* 0x28 RegIrqFlags2 */
/*****************************************************************************/
#define RFM69_REG_IRQFLAGS2                         0x28

#define RFM69_MSK_IRQFLAGS2_FIFOOVERRUN             0x01
#define RFM69_SHF_IRQFLAGS2_FIFOOVERRUN             4

#define RFM69_MSK_IRQFLAGS2_PACKETSENT              0x01
#define RFM69_SHF_IRQFLAGS2_PACKETSENT              3

#define RFM69_MSK_IRQFLAGS2_PAYLOADREADY            0x01
#define RFM69_SHF_IRQFLAGS2_PAYLOADREADY            2


/*****************************************************************************/
/* 0x29 RegRssiThresh */
/*****************************************************************************/
#define RFM69_REG_RSSITHRESH                        0x29


/*****************************************************************************/
/* 0x2E RegSyncConfig */
/* 0x2F RegSyncValue1 */
/*****************************************************************************/
#define RFM69_REG_SYNCCONFIG                        0x2e
#define RFM69_REG_SYNCVALUE1                        0x2f

#define RFM69_MSK_SYNCCONFIG_SYNCON                 0x01
#define RFM69_SHF_SYNCCONFIG_SYNCON                 7

#define RFM69_MSK_SYNCCONFIG_SYNCSIZE               0x07
#define RFM69_SHF_SYNCCONFIG_SYNCSIZE               3


/*****************************************************************************/
/* 0x37 RegPacketConfig1 */
/*****************************************************************************/
#define RFM69_REG_PACKETCONFIG1                     0x37

#define RFM69_MSK_PACKETCONFIG1_PACKETFORMAT        0x01
#define RFM69_SHF_PACKETCONFIG1_PACKETFORMAT        7

#define RFM69_MSK_PACKETCONFIG1_CRCON               0x01
#define RFM69_SHF_PACKETCONFIG1_CRCON               4

#define RFM69_MSK_PACKETCONFIG1_CRCAUTOCLEAROFF     0x01
#define RFM69_SHF_PACKETCONFIG1_CRCAUTOCLEAROFF     3


/*****************************************************************************/
/* 0x38 PayloadLength */
/*****************************************************************************/
#define RFM69_REG_PAYLOADLENGTH                     0x38


/*****************************************************************************/
/* 0x3c RegFifoThresh */
/*****************************************************************************/
#define RFM69_REG_FIFOTHRESH                        0x3c

#define RFM69_MSK_FIFOTHRESH_TXSTARTCONDITION       0x01
#define RFM69_SHF_FIFOTHRESH_TXSTARTCONDITION       7
#define RFM69_FIFO_LEVEL                            0
#define RFM69_FIFO_NOT_EMPTY                        1


/*****************************************************************************/
/* 0x3d RegPacketConfig2 */
/*****************************************************************************/
#define RFM69_REG_PACKETCONFIG2                     0x3d

#define RFM69_MSK_PACKETCONFIG2_RXRESTART           0x01
#define RFM69_SHF_PACKETCONFIG2_RXRESTART           2
#define RFM69_RXRESTART                             1


/*****************************************************************************/
/* 0x4e RegTemp1 */
/* 0x4f RegTemp2 */
/*****************************************************************************/
#define RFM69_REG_TEMP1                             0x4e
#define RFM69_REG_TEMP2                             0x4f

#define RFM69_MSK_TEMP_MEAS_START                   0x01
#define RFM69_SHF_TEMP_MEAS_START                   3
#define RFM69_TEMP_MEAS_START                       1

#define RFM69_MSK_TEMP_MEAS_RUNNING                 0x01
#define RFM69_SHF_TEMP_MEAS_RUNNING                 2


/*****************************************************************************/
/* 0x5a RegTestPa1 */
/* 0x5c RegTestPa2 */
/*****************************************************************************/
#define RFM69_REG_TESTPA1                           0x5a
#define RFM69_REG_TESTPA2                           0x5c

#define RFM69_PA20DBM1_NORMAL                       0x55
#define RFM69_PA20DBM1_20DBM_MODE                   0x5d

#define RFM69_PA20DBM2_NORMAL                       0x70
#define RFM69_PA20DBM2_20DBM_MODE                   0x7c


/*****************************************************************************/
/* Global variables */
/*****************************************************************************/
extern volatile uint8_t rfm69_flg_isr;          /**< ISR flag */


/*****************************************************************************/
/* Prototypes */
/*****************************************************************************/
void rfm69_init(
    uint8_t flg_is_rfm69hw                      /**< output power flag */
);

uint8_t rfm69_reg_read_raw(
    uint8_t addr                                /**< register address */
);

void rfm69_reg_write_raw(
    uint8_t addr,                               /**< register address */
    uint8_t val                                 /**< value */
);

uint8_t rfm69_reg_read(
    uint8_t addr,                               /**< register address */
    uint8_t mask,                               /**< value mask */
    uint8_t shift                               /**< value shift */
);

void rfm69_reg_rw(
    uint8_t addr,                               /**< register address */
    uint8_t mask,                               /**< value mask */
    uint8_t shift,                              /**< value shift */
    uint8_t val                                 /**< value */
);

uint8_t rfm69_opmode_get(
    void
);

void rfm69_opmode_set(
    uint8_t mode                                /**< transceiver mode */
);

void rfm69_freq_carrier_khz(
    uint32_t freq_khz                           /**< carrier frequency in kHz */
);

void rfm69_bitrate_bs(
    uint16_t bitrate_bs                         /**< bitrate in b/s */
);

void rfm69_dio_mapping_rx(
    uint8_t dio,                                /**< DIO number */
    uint8_t val                                 /**< map value */
);

void rfm69_dio_mapping_tx(
    uint8_t dio,                                /**< DIO number */
    uint8_t val                                 /**< map value */
);

void rfm69_dio_mapping(
    uint8_t dio,                                /**< DIO number */
    uint8_t val                                 /**< map value */
);

void rfm69_clkout(
    uint8_t clkout                              /**< CLKOUT config */
);

void rfm69_crc_on(
    uint8_t on                                  /**< CRC calculation on flag */
);

void rfm69_crc_auto_clear_off(
    uint8_t off                                 /**< CRC auto clear off flag */
);

void rfm69_payload_length(
    uint8_t len                                 /**< payload length */
);

void rfm69_sync_on(
    uint8_t on                                  /**< sync on flag */
);

void rfm69_sync_word(
    uint8_t size,                               /**< sync word size */
    uint8_t *values                             /**< sync values */
);

void rfm69_rx_bw_exp(
    uint8_t exp                                 /**< exponent */
);

void rfm69_rssi_threshold(
    int threshold                               /**< threshold in dBm */
);

int rfm69_rssi_value(
    uint8_t flg_trigger                         /**< trigger RSSI measurement */
);

void rfm69_fifo_clear(
    void
);

uint8_t rfm69_fifo_data_avail(
    void
);

uint8_t rfm69_fifo_data(
    void
);

PINKIE_RES_T rfm69_send(
    uint8_t *data,                              /**< data */
    uint8_t len                                 /**< data length */
);

void rfm69_packet_format_var_len(
    uint8_t var_len                             /**< variable length flag */
);

void rfm69_tx_start_cond(
    uint8_t val                                 /**< TX start condition */
);

void rfm69_fdev_hz(
    uint16_t fdev_hz                            /**< value in Hz */
);

void rfm69_pa_sel(
    uint8_t pa_sel                              /**< power amplifier mask */
);

void rfm69_output_power(
    uint8_t val                                 /**< output power in percent */
);

uint8_t rfm69_rx_avail(
    void
);

void rfm69_ocp(
    uint8_t on                                  /**< OCP on flag */
);

void rfm69_high_power_pa(
    uint8_t on                                  /**< high power PA */
);

void rfm69_int_ctrl(
    uint8_t on                                  /**< interrupt on */
);

uint8_t rfm69_temp(
    void
);

void rfm69_rc_osc_cal(
    void
);

uint16_t rfm69_send_budget_ms_get(
    void
);


#endif /* RADIO_RFM69_H */
