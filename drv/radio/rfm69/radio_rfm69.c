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
 * Warning: Don't rely on the implementation of the 1 percent rule. Please
 * check the algorithm and send a report if the implementation is wrong.
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <pinkie.h>
#include <drv/spi/pinkie_spi.h>
#include <drv/timer/pinkie_timer.h>
#include <drv/radio/rfm69/radio_rfm69.h>


/*****************************************************************************/
/* Global variables */
/*****************************************************************************/
volatile uint8_t rfm69_flg_isr = 0;             /**< ISR flag */


/*****************************************************************************/
/* Local variables */
/*****************************************************************************/
static uint8_t rfm69_var_len = 0;               /**< variable length flag */
static uint8_t rfm69_opmode = 0xff;             /**< operation mode */
static uint8_t rfm69_flg_is_hw = 0;             /**< RFM69HW flag */
static uint8_t rfm69_dio_mapping_rx_dio = 0xff; /**< RX DIO selector */
static uint8_t rfm69_dio_mapping_rx_val;        /**< RX DIO value */
static uint8_t rfm69_dio_mapping_tx_dio = 0xff; /**< TX DIO selector */
static uint8_t rfm69_dio_mapping_tx_val;        /**< TX DIO value */
static uint16_t rfm69_time_budget_ms;           /**< send time budget in ms */
static uint64_t rfm69_time_send_last_ms;        /**< timestamp of last send */


/*****************************************************************************/
/* Local prototypes */
/*****************************************************************************/
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


/*****************************************************************************/
/** RFM69 SPI Initialization
 */
void rfm69_init(
    uint8_t flg_is_rfm69hw                      /**< output power flag */
)
{
    /* store variant for output power control */
    rfm69_flg_is_hw = flg_is_rfm69hw;

    /* select Power Amplifier depending on variant */
    if (flg_is_rfm69hw) {
        rfm69_pa_sel(RFM69_PA_1_ON | RFM69_PA_2_ON);
        rfm69_ocp(0);
    } else {
        rfm69_pa_sel(RFM69_PA_0_ON);
    }

    /* send time budget in milliseconds */
    rfm69_time_budget_ms = RFM69_TIME_BUDGET_MS;
}


/*****************************************************************************/
/** RFM69 Read Full Register
 */
uint8_t rfm69_reg_read_raw(
    uint8_t addr                                /**< register address */
)
{
    uint8_t data[2];                            /* SPI data */

    data[0] = addr;

    pinkie_spi_xfer((char *) data, (char *) data, sizeof(data), 1);

    return data[1];
}


/*****************************************************************************/
/** RFM69 Write Full Register
 */
void rfm69_reg_write_raw(
    uint8_t addr,                               /**< register address */
    uint8_t val                                 /**< value */
)
{
    uint8_t data[2];                            /* SPI data */

    data[0] = SPI_WRITE | addr;
    data[1] = val;
    pinkie_spi_xfer((char *) data, NULL, sizeof(data), 1);
}


/*****************************************************************************/
/** RFM69 Read Register Value
 */
uint8_t rfm69_reg_read(
    uint8_t addr,                               /**< register address */
    uint8_t mask,                               /**< value mask */
    uint8_t shift                               /**< value shift */
)
{
    return (rfm69_reg_read_raw(addr) >> shift) & mask;
}


/*****************************************************************************/
/** RFM69 Update Register Value
 *
 * Reads a register, updates the value and writes it back.
 */
void rfm69_reg_rw(
    uint8_t addr,                               /**< register address */
    uint8_t mask,                               /**< value mask */
    uint8_t shift,                              /**< value shift */
    uint8_t val                                 /**< value */
)
{
    rfm69_reg_write_raw(addr, (rfm69_reg_read_raw(addr) & ~(mask << shift)) | ((val & mask) << shift));
}


/*****************************************************************************/
/** RFM69 Get Operation Mode
 */
uint8_t rfm69_opmode_get(
    void
)
{
    /* check if mode is already known */
    if (0xff == rfm69_opmode) {

        /* get mode */
        rfm69_opmode = rfm69_reg_read(RFM69_REG_OPMODE,
                                      RFM69_MSK_OPMODE_MODE,
                                      RFM69_SHF_OPMODE_MODE);
    }

    return rfm69_opmode;
}


/*****************************************************************************/
/** RFM69 Set Operation Mode
 */
void rfm69_opmode_set(
    uint8_t mode                                /**< transceiver mode */
)
{
    uint64_t ts64;                              /* timeout timestamp */

    /* configure DIO mapping if set */
    if (RFM69_OPMODE_RX == mode) {
        if (rfm69_dio_mapping_rx_dio != 0xff) {
            rfm69_dio_mapping(rfm69_dio_mapping_rx_dio, rfm69_dio_mapping_rx_val);
        }
    }
    else if (RFM69_OPMODE_TX == mode) {
        if (rfm69_dio_mapping_tx_dio != 0xff) {
            rfm69_dio_mapping(rfm69_dio_mapping_tx_dio, rfm69_dio_mapping_tx_val);
        }
    }

    /* clear ISR flag */
    rfm69_flg_isr = 0;

    /* set mode */
    rfm69_reg_rw(RFM69_REG_OPMODE,
                 RFM69_MSK_OPMODE_MODE,
                 RFM69_SHF_OPMODE_MODE,
                 mode);

    /* wait until mode is ready */
    ts64 = pinkie_timer_get() + RFM69_TIMEOUT_MS;
    while (!rfm69_reg_read(RFM69_REG_IRQFLAGS1,
                           RFM69_MSK_IRQFLAGS1_MODEREADY,
                           RFM69_SHF_IRQFLAGS1_MODEREADY)) {

        if (pinkie_timer_get() >= ts64) {
            pinkie_printf("opmode: timeout\n");
            break;
        }
    }

    /* enable high power output for RFM69HW if mode is TX */
    if (rfm69_flg_is_hw) {
        if (RFM69_OPMODE_TX == mode) {
            rfm69_high_power_pa(1);
        } else {
            rfm69_high_power_pa(0);
        }
    }

    /* restart RX if mode is RX */
    if (RFM69_OPMODE_RX == mode) {
        rfm69_reg_rw(RFM69_REG_PACKETCONFIG2,
                     RFM69_MSK_PACKETCONFIG2_RXRESTART,
                     RFM69_SHF_PACKETCONFIG2_RXRESTART,
                     RFM69_RXRESTART);
    }

    /* update global opmode */
    rfm69_opmode = mode;
}


/*****************************************************************************/
/** RFM69 Carrier Frequency in kHz
 *
 * Example: 868000 for 868 MHz.
 */
void rfm69_freq_carrier_khz(
    uint32_t freq_khz                           /**< carrier frequency in kHz */
)
{
    uint32_t frf;                               /* carrier frequency register value */

    frf = freq_khz / (RFM69_FREQ_FSTEP_HZ / RFM69_UNIT_KILO);

    rfm69_reg_write_raw(RFM69_REG_FRFMSB, (uint8_t) (frf >> 16));
    rfm69_reg_write_raw(RFM69_REG_FRFMID, (uint8_t) (frf >> 8));
    rfm69_reg_write_raw(RFM69_REG_FRFLSB, (uint8_t) frf);
}


/*****************************************************************************/
/** RFM69 Bitrate in b/s
 *
 * Example: 6631 for 6.631 kb/s.
 */
void rfm69_bitrate_bs(
    uint16_t bitrate_bs                         /**< bitrate in b/s */
)
{
    uint16_t bitrate;                           /* bitrate register value */

    bitrate = RFM69_FREQ_FXOSC_HZ / bitrate_bs;

    rfm69_reg_write_raw(RFM69_REG_BITRATEMSB, (uint8_t) (bitrate >> 8));
    rfm69_reg_write_raw(RFM69_REG_BITRATELSB, (uint8_t) bitrate);
}


/*****************************************************************************/
/** RFM69 DIO Pin Mapping for RX
 */
void rfm69_dio_mapping_rx(
    uint8_t dio,                                /**< DIO number */
    uint8_t val                                 /**< map value */
)
{
    rfm69_dio_mapping_rx_dio = dio;
    rfm69_dio_mapping_rx_val = val;
}


/*****************************************************************************/
/** RFM69 DIO Pin Mapping for TX
 */
void rfm69_dio_mapping_tx(
    uint8_t dio,                                /**< DIO number */
    uint8_t val                                 /**< map value */
)
{
    rfm69_dio_mapping_tx_dio = dio;
    rfm69_dio_mapping_tx_val = val;
}


/*****************************************************************************/
/** RFM69 DIO Pin Mapping
 */
void rfm69_dio_mapping(
    uint8_t dio,                                /**< DIO number */
    uint8_t val                                 /**< map value */
)
{
    uint8_t reg;
    uint8_t shift;

    if (4 > dio) {
        reg = RFM69_REG_DIOMAPPING1;
        shift = 6 - (dio * 2);
    } else {
        reg = RFM69_REG_DIOMAPPING2;
        shift = 6 - ((dio - 4) * 2);
    }

    rfm69_reg_rw(reg, RFM69_MSK_DIOMAPPING, shift, val);
}


/*****************************************************************************/
/** RFM69 Control CLKOUT
 */
void rfm69_clkout(
    uint8_t clkout                              /**< CLKOUT config */
)
{
    rfm69_reg_rw(RFM69_REG_DIOMAPPING2,
                 RFM69_MSK_DIOMAPPING2_CLKOUT,
                 RFM69_SHF_DIOMAPPING2_CLKOUT,
                 clkout);
}


/*****************************************************************************/
/** RFM69 CRC Calculation Control
 */
void rfm69_crc_on(
    uint8_t on                                  /**< CRC calculation on flag */
)
{
    rfm69_reg_rw(RFM69_REG_PACKETCONFIG1,
                 RFM69_MSK_PACKETCONFIG1_CRCON,
                 RFM69_SHF_PACKETCONFIG1_CRCON,
                 (on) ? 1 : 0);
}


/*****************************************************************************/
/** RFM69 CRC Auto Clear Control
 */
void rfm69_crc_auto_clear_off(
    uint8_t off                                 /**< CRC auto clear off flag */
)
{
    rfm69_reg_rw(RFM69_REG_PACKETCONFIG1,
                 RFM69_MSK_PACKETCONFIG1_CRCAUTOCLEAROFF,
                 RFM69_SHF_PACKETCONFIG1_CRCAUTOCLEAROFF,
                 (off) ? 1 : 0);
}


/*****************************************************************************/
/** RFM69 Payload Length
 */
void rfm69_payload_length(
    uint8_t len                                 /**< payload length */
)
{
    rfm69_reg_write_raw(RFM69_REG_PAYLOADLENGTH, len);
}


/*****************************************************************************/
/** RFM69 Sync Word Generation And Detection
 */
void rfm69_sync_on(
    uint8_t on                                  /**< sync on flag */
)
{
    rfm69_reg_rw(RFM69_REG_SYNCCONFIG,
                 RFM69_MSK_SYNCCONFIG_SYNCON,
                 RFM69_SHF_SYNCCONFIG_SYNCON,
                 (on) ? 1 : 0);
}


/*****************************************************************************/
/** RFM69 Sync Word Size
 */
void rfm69_sync_word(
    uint8_t size,                               /**< sync word size */
    uint8_t *values                             /**< sync values */
)
{
    unsigned int cnt;                           /* counter */

    /* sync size always add +1 so decrement size here */
    rfm69_reg_rw(RFM69_REG_SYNCCONFIG,
                 RFM69_MSK_SYNCCONFIG_SYNCSIZE,
                 RFM69_SHF_SYNCCONFIG_SYNCSIZE,
                 size - 1);

    /* fill sync values */
    for (cnt = 0; cnt < size; cnt++) {
        rfm69_reg_write_raw(RFM69_REG_SYNCVALUE1 + cnt, values[cnt]);
    }
}


/*****************************************************************************/
/** RFM69 Channel Filter Bandwidth Control
 */
void rfm69_rx_bw_exp(
    uint8_t exp                                 /**< exponent */
)
{
    rfm69_reg_rw(RFM69_REG_RXBW,
                 RFM69_MSK_RXBW_RXBWEXP,
                 RFM69_SHF_RXBW_RXBWEXP,
                 exp);
}


/*****************************************************************************/
/** RFM69 RSSI Threshold
 *
 * Default: 228 (0xe4) => 228 / 2 = -114 dBm
 */
void rfm69_rssi_threshold(
    int threshold                               /**< threshold in dBm */
)
{
    rfm69_reg_write_raw(RFM69_REG_RSSITHRESH, (-threshold) << 1);
}


/*****************************************************************************/
/** RFM69 RSSI Value in dBm
 *
 * If flg_trigger isn't set the last recorded RSSI value is reported.
 *
 * Returned value is derived from formula: RSSI = - (RssiValue / 2) dBm
 */
int rfm69_rssi_value(
    uint8_t flg_trigger                         /**< trigger RSSI measurement */
)
{
    uint64_t ts64;                              /* timeout timestamp */

    if (flg_trigger) {

        rfm69_reg_rw(RFM69_REG_RSSICONFIG,
                     RFM69_MSK_RSSICONFIG_RSSISTART,
                     RFM69_SHF_RSSICONFIG_RSSISTART,
                     RFM69_RSSICONFIG_RSSISTART);

        ts64 = pinkie_timer_get() + RFM69_TIMEOUT_MS;
        while (RFM69_RSSICONFIG_RSSIDONE != rfm69_reg_read(RFM69_REG_RSSICONFIG,
                                                           RFM69_MSK_RSSICONFIG_RSSIDONE,
                                                           RFM69_SHF_RSSICONFIG_RSSIDONE)) {

            if (pinkie_timer_get() >= ts64) {
                pinkie_printf("rssi: timeout\n");
                return 0;
            }
        }
    }

    return -(rfm69_reg_read_raw(RFM69_REG_RSSIVALUE) >> 1);
}


/*****************************************************************************/
/** RFM69 Clear Fifo
 */
void rfm69_fifo_clear(
    void
)
{
    rfm69_reg_write_raw(RFM69_REG_IRQFLAGS2,
                        RFM69_MSK_IRQFLAGS2_FIFOOVERRUN << RFM69_SHF_IRQFLAGS2_FIFOOVERRUN);
}


/*****************************************************************************/
/** RFM69 Fifo Data Available
 */
uint8_t rfm69_fifo_data_avail(
    void
)
{
    return (rfm69_reg_read(RFM69_REG_IRQFLAGS2,
                           RFM69_MSK_IRQFLAGS2_PAYLOADREADY,
                           RFM69_SHF_IRQFLAGS2_PAYLOADREADY)) ? 1 : 0;
}


/*****************************************************************************/
/** RFM69 Fifo Data
 */
uint8_t rfm69_fifo_data(
    void
)
{
    uint8_t fifo;                               /* FIFO data */

    /* read FIFO */
    fifo = rfm69_reg_read_raw(RFM69_REG_FIFO);

    return fifo;
}


/*****************************************************************************/
/** RFM69 Send Data
 *
 * Send given data and switch back to RX mode.
 */
PINKIE_RES_T rfm69_send(
    uint8_t *data,                              /**< data */
    uint8_t len                                 /**< data length */
)
{
    PINKIE_RES_T res = PINKIE_OK;               /* result */
    uint64_t ts64;                              /* send timestamp */
    uint64_t ts64_tout;                         /* timeout timestamp */
    uint8_t addr;                               /* RFM69 address */

    /* check if sending is allowed */
    if (RFM69_TIME_BUDGET_MIN_MS > rfm69_send_budget_ms_get()) {
        return PINKIE_ERR_NO_BUDGET;
    }

    /* restart RX to avoid RX deadlocks */
    rfm69_reg_rw(RFM69_REG_PACKETCONFIG2,
                 RFM69_MSK_PACKETCONFIG2_RXRESTART,
                 RFM69_SHF_PACKETCONFIG2_RXRESTART,
                 RFM69_RXRESTART);

    /* disable receiver and interrupts */
    rfm69_opmode_set(RFM69_OPMODE_STANDBY);
    rfm69_fifo_clear();
    rfm69_int_ctrl(0);

    /* transfer data */
    pinkie_spi_sel_ctrl(1);
    addr = SPI_WRITE | RFM69_REG_FIFO;
    pinkie_spi_xfer((char *) &addr, NULL, 1, 0);

    for (; len; len--, data++) {
        pinkie_spi_xfer((char *) data, NULL, 1, 0);
    }
    pinkie_spi_sel_ctrl(0);

    /* record start time to calculate send time */
    ts64 = pinkie_timer_get();

    /* enable interrupts and send frame */
    rfm69_int_ctrl(1);
    rfm69_opmode_set(RFM69_OPMODE_TX);

    /* wait until data was sent
     * (ISR flag is cleared at next mode set)
     */
    ts64_tout = pinkie_timer_get() + RFM69_TIMEOUT_MS;
    while (1 != rfm69_flg_isr) {
        if (pinkie_timer_get() >= ts64_tout) {
            pinkie_printf("send: timeout\n");
            rfm69_fifo_clear();
            res = PINKIE_ERR_TIMEOUT;
            break;
        }
    }

    /* switch back to receive mode */
    rfm69_opmode_set(RFM69_OPMODE_STANDBY);
    rfm69_opmode_set(RFM69_OPMODE_RX);

    /* calculate time needed to send frame */
    rfm69_time_send_last_ms = pinkie_timer_get();
    ts64 = (rfm69_time_send_last_ms - ts64) + RFM69_TIME_BUDGET_EXTRA_MS;
    if (ts64 > rfm69_time_budget_ms) {
        rfm69_time_budget_ms = 0;
    } else {
        rfm69_time_budget_ms -= ts64;
    }

    return res;
}


/*****************************************************************************/
/** RFM69 Packet Format
 */
void rfm69_packet_format_var_len(
    uint8_t var_len                             /**< variable length flag */
)
{
    rfm69_var_len = var_len;

    rfm69_reg_rw(RFM69_REG_PACKETCONFIG1,
                 RFM69_MSK_PACKETCONFIG1_PACKETFORMAT,
                 RFM69_SHF_PACKETCONFIG1_PACKETFORMAT,
                 (var_len) ? 1 : 0);
}


/*****************************************************************************/
/** RFM69 TX Start Condition
 */
void rfm69_tx_start_cond(
    uint8_t val                                 /**< TX start condition */
)
{
    rfm69_reg_rw(RFM69_REG_FIFOTHRESH,
                 RFM69_MSK_FIFOTHRESH_TXSTARTCONDITION,
                 RFM69_SHF_FIFOTHRESH_TXSTARTCONDITION,
                 val);
}


/*****************************************************************************/
/** RFM69 Frequency Deviation in Hz
 */
void rfm69_fdev_hz(
    uint16_t fdev_hz                            /**< value in Hz */
)
{
    uint16_t fdev;                              /* frequency deviation reg val */

    fdev = fdev_hz / RFM69_FREQ_FSTEP_HZ;

    rfm69_reg_write_raw(RFM69_REG_FDEVMSB, (uint8_t) (fdev >> 8));
    rfm69_reg_write_raw(RFM69_REG_FDEVLSB, (uint8_t) fdev);
}


/*****************************************************************************/
/** RFM69 Power Amplifier Selection
 */
void rfm69_pa_sel(
    uint8_t pa_sel                              /**< power amplifier mask */
)
{
    rfm69_reg_rw(RFM69_REG_PALEVEL,
                 RFM69_MSK_PALEVEL_PA_ON,
                 RFM69_SHF_PALEVEL_PA_ON,
                 pa_sel);
}


/*****************************************************************************/
/** RFM69 Output Power Level in Percent
 *
 * RFM69W = -18 .. 13 dBm => 0 = -18 dBm, 50 = -3 dBm, 100 = 13 dBm
 * RFM69HW = +5 .. 20 dBm => 0 = 5 dBm, 50 = 12 dBm, 100 = 20 dBM
 */
void rfm69_output_power(
    uint8_t val                                 /**< output power in percent */
)
{
    if (rfm69_flg_is_hw) {
        val = (val * (20 - 5)) / 100;
    } else {
        val = (val * (13 - (-18))) / 100;
    }

    rfm69_reg_rw(RFM69_REG_PALEVEL,
                 RFM69_MSK_PALEVEL_OUTPUTPOWER,
                 RFM69_SHF_PALEVEL_OUTPUTPOWER,
                 val);
}


/*****************************************************************************/
/** RFM69 Packet Receive Check
 */
uint8_t rfm69_rx_avail(
    void
)
{
    if (RFM69_OPMODE_RX != rfm69_opmode) {
        return 0;
    }

    if (1 == rfm69_flg_isr) {
        rfm69_flg_isr = 0;
        return 1;
    }

    return rfm69_fifo_data_avail();
}


/*****************************************************************************/
/** RFM69 Over Current Protection
 */
void rfm69_ocp(
    uint8_t on                                  /**< OCP on flag */
)
{
    rfm69_reg_rw(RFM69_REG_OCP,
                 RFM69_MSK_OCP_OCP_ON,
                 RFM69_SHF_OCP_OCP_ON,
                 !!on);
}


/*****************************************************************************/
/** RFM69 High Power Power Amplifier
 */
void rfm69_high_power_pa(
    uint8_t on                                  /**< high power PA */
)
{
    rfm69_reg_write_raw(RFM69_REG_TESTPA1,
                        (on) ? RFM69_PA20DBM1_20DBM_MODE : RFM69_PA20DBM1_NORMAL);

    rfm69_reg_write_raw(RFM69_REG_TESTPA2,
                        (on) ? RFM69_PA20DBM2_20DBM_MODE : RFM69_PA20DBM2_NORMAL);
}


/*****************************************************************************/
/** RFM69 Temperature Measurement
 *
 * See datasheet chapter "Temperature Sensor Registers" for details.
 *
 * @returns temperature or 0xff on error
 */
uint8_t rfm69_temp(
    void
)
{
    uint64_t ts64;                              /* timeout timestamp */
    uint8_t opmode;                             /* current opmode */

    /* store current opmode */
    opmode = rfm69_opmode_get();

    /* put transceiver in standby mode */
    rfm69_opmode_set(RFM69_OPMODE_STANDBY);

    /* start temperature measurement */
    rfm69_reg_rw(RFM69_REG_TEMP1,
                 RFM69_MSK_TEMP_MEAS_START,
                 RFM69_SHF_TEMP_MEAS_START,
                 RFM69_TEMP_MEAS_START);

    /* wait until measurement is done */
    ts64 = pinkie_timer_get() + RFM69_TIMEOUT_MS;
    while (rfm69_reg_read(RFM69_REG_TEMP1,
                          RFM69_MSK_TEMP_MEAS_RUNNING,
                          RFM69_SHF_TEMP_MEAS_RUNNING)) {

        if (pinkie_timer_get() >= ts64) {
            pinkie_printf("temp: timeout\n");

            /* previous opmode */
            rfm69_opmode_set(opmode);

            return UINT8_MAX;
        }
    }

    /* previous opmode */
    rfm69_opmode_set(opmode);

    return ~rfm69_reg_read_raw(RFM69_REG_TEMP2);
}


/*****************************************************************************/
/** RFM69 RC Oscillator Calibration
 *
 * See datasheet chapter "RC Timer Accuracy" for details.
 */
void rfm69_rc_osc_cal(
    void
)
{
    uint64_t ts64;                              /* timeout timestamp */
    uint8_t opmode;                             /* current opmode */

    /* store current opmode */
    opmode = rfm69_opmode_get();

    /* put transceiver in standby mode */
    rfm69_opmode_set(RFM69_OPMODE_STANDBY);

    /* start temperature measurement */
    rfm69_reg_rw(RFM69_REG_OSC1,
                 RFM69_MSK_OSC1_RCCALSTART,
                 RFM69_SHF_OSC1_RCCALSTART,
                 RFM69_OSC1_RCCALSTART);

    /* wait until measurement is done */
    ts64 = pinkie_timer_get() + RFM69_TIMEOUT_MS;
    while (RFM69_OSC1_RCCALDONE != rfm69_reg_read(RFM69_REG_OSC1,
                                                  RFM69_MSK_OSC1_RCCALDONE,
                                                  RFM69_SHF_OSC1_RCCALDONE)) {

        if (pinkie_timer_get() >= ts64) {
            pinkie_printf("rc_osc_cal: timeout\n");
            break;
        }
    }

    /* previous opmode */
    rfm69_opmode_set(opmode);
}


/*****************************************************************************/
/** RFM69 Read available send time budget in ms
 *
 * Returns the available send time budget in ms that is limited through the 1
 * percent rule.
 *
 * @returns available send time budget in ms
 */
uint16_t rfm69_send_budget_ms_get(
    void
)
{
    uint64_t ts64;                              /* timestamp */

    /* calculate time left for sending */
    ts64 = (pinkie_timer_get() - rfm69_time_send_last_ms) * RFM69_TIME_BUDGET_RECOVER_MS;
    if (RFM69_TIME_BUDGET_MS < (rfm69_time_budget_ms + ts64)) {
        rfm69_time_budget_ms = RFM69_TIME_BUDGET_MS;
    } else {
        rfm69_time_budget_ms += ts64;
    }

    return rfm69_time_budget_ms;
}
