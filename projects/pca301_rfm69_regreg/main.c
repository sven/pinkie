/**
 * @brief PCA301 Main Project
 *
 * This driver was created by extensively reading source code and forum
 * comments contributed by other authors. Thank you.
 *
 * It is currently only tested on the Arduino Nano with the RFM69 transmitter
 * connected by SPI. Thanks to this guide (german)
 * https://steigerbalett.wordpress.com/2015/05/23/jeelink-clone-loten-und-mit-einer-firmware-flashen-fur-lacrosse-sensoren-in-fhem/
 * I was able to wire them together.
 *
 * Notes:
 *   - the initial RFM69 temperature offset was measured by a cheap infrared
 *     thermometer on the CPU
 *
 * Copyright (c) 2017-2018, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <pinkie.h>
#include <pinkie_stack.h>
#include <drv/radio/rfm69/radio_rfm69.h>

#include <util/delay.h>
#include <avr/interrupt.h>

#include <acyclic.h>
#include <regreg.h>
#include <regreg_acyclic.h>
#include <pca301_rfm69.h>


/*****************************************************************************/
/* Defines */
/*****************************************************************************/
#define DEVICE_ID                   0x01UL      /**< device id */
#define DEVICE_VERSION              1           /**< device version */

#define REG_BASE_PCA301             4100        /**< regreg base PCA301 */

#define REG_ATMEGA_TEMP             0           /**< ATmega temperature */
#define REG_ATMEGA_VOLT             2           /**< ATmega voltage */
#define REG_ATMEGA_MS               4           /**< ATmega millisecond timer */

#define PROJ_RFM69_REG_TEMP         114         /**< RFM69 temperature reg */
#define PROJ_RFM69_REG_RSSI         115         /**< RFM69 RSSI value reg */
#define PROJ_RFM69_REG_OSC          116         /**< RFM69 RC osc calibration reg */
#define PROJ_RFM69_REG_BUDGET       117         /**< RFM69 send time budget reg */

#define PROJ_RFM69_VAL_TEMP_CORR    -95         /**< RFM69 temperature correction */
#define PROJ_ATMEGA_VAL_TEMP_CORR   -333        /**< ATmega temperature correction */
#define PROJ_ATMEGA_VAL_VOLT_CORR   1023L       /**< ATmega voltage correction */

#define RFM69_IS_HW                 1           /**< RFM69 is HW variant flag */

#define PROJECT_PCA301_CNT          10          /**< store up to 10 devices */


/*****************************************************************************/
/* Data types */
/*****************************************************************************/
typedef struct {
    uint16_t temp;                              /**< temperature */
    uint16_t volt;                              /**< voltage */
    uint64_t ms;                                /**< milliseconds */
} __attribute__((packed)) REG_ATMEGA_T;


/**< non-volatile storage data */
typedef struct {
    uint16_t crc16;                             /**< CRC16 (must be first) */

    uint8_t flg_rfm69_is_hw;                    /**< RFM69 is HW variant flag */
    int8_t val_rfm69_temp_corr;                 /**< RFM69 temperature correction */

    int16_t val_atmega_temp_corr;               /**< ATmega temperature correction */
    int16_t val_atmega_volt_corr;               /**< ATmega voltage correction */

    PCA301_RFM69_NVS_T pca301_rfm69_nvs;        /**< PCA301 RFM69 data */
} __attribute__((packed)) PROJECT_NVS_T;


/*****************************************************************************/
/* Local prototypes */
/*****************************************************************************/
static unsigned int reg_nvs(
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
);

static unsigned int reg_atmega(
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
);

static unsigned int reg_rfm69(
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
);


/*****************************************************************************/
/* Variables */
/*****************************************************************************/
static ACYCLIC_T g_a = { 0 };                   /**< ACyCLIC handle */
static unsigned int flg_nvs_valid = 0;          /**< NVS valid flag */
static PROJECT_NVS_T data_nvs;                  /**< NVS data */
static REG_ATMEGA_T data_atmega;                /**< ATmega data */

static uint8_t data_device[5] = {               /**< device data */
    DEVICE_ID & 0xff,
    (DEVICE_ID >> 8) & 0xff,
    (DEVICE_ID >> 16) & 0xff,
    (DEVICE_ID >> 24) & 0xff,
    DEVICE_VERSION,
};

static REG_ENTRY_T reg_info_device = {          /**< device register */
    NULL,
    0,
    sizeof(data_device) - 1,
    NULL,
    data_device,
};

static REG_ENTRY_T reg_info_nvs = {             /**< NVS register */
    NULL,
    1000,
    1000 + sizeof(PROJECT_NVS_T) - 1,
    reg_nvs,
    &data_nvs,
};

static REG_ENTRY_T reg_info_atmega = {          /**< ATmega register */
    NULL,
    2000,
    2000 + 11,
    reg_atmega,
    &data_atmega,
};

static REG_ENTRY_T reg_info_rfm69 = {           /**< RFM69 register */
    NULL,
    3000,
    3000 + PROJ_RFM69_REG_BUDGET,
    reg_rfm69,
    NULL,
};


/*****************************************************************************/
/** Main
 */
int main(
    void
)
{
    int res;                                    /* result */

    /* disable interrupts */
    cli();

    /* initialize STDIO */
    res = pinkie_stdio_init();
    if (res) {
        goto _bail;
    }

    pinkie_printf("System: booting\n");

    /* initialize timer */
    pinkie_timer_init();

    /* read NVS data */
    flg_nvs_valid = pinkie_nvs_read((uint8_t *) &data_nvs, sizeof(data_nvs));
    pinkie_printf("NVS: %svalid\n", (flg_nvs_valid) ? "" : "in");

    if (!flg_nvs_valid) {
        data_nvs.val_atmega_temp_corr = PROJ_ATMEGA_VAL_TEMP_CORR;
        data_nvs.val_atmega_volt_corr = PROJ_ATMEGA_VAL_VOLT_CORR;
    }

    /* initialize SPI */
    pinkie_spi_init();

    /* create RegReg registers */
    reg_add(&reg_info_device);
    reg_add(&reg_info_nvs);
    reg_add(&reg_info_atmega);
    reg_add(&reg_info_rfm69);

    /* initialize RFM69 transmitter */
    if (!flg_nvs_valid) {
        data_nvs.flg_rfm69_is_hw = RFM69_IS_HW;
        data_nvs.val_rfm69_temp_corr = PROJ_RFM69_VAL_TEMP_CORR;
    }

    pinkie_printf("RFM69: %s power\n", (data_nvs.flg_rfm69_is_hw) ? "high" : "normal");
    rfm69_init(data_nvs.flg_rfm69_is_hw);

    /* configure interrupt for raising edge */
    EICRA |= (1 << ISC01) | (1 << ISC00);

    /* configure INT0 as input */
    DDRD &= ~(1 << DDD2);

    /* enable RFM69 interrupt */
    rfm69_int_ctrl(1);

    /* initialize PCA301 socket driver */
    pca301_rfm69_init(flg_nvs_valid, &data_nvs.pca301_rfm69_nvs);
    pca301_init(REG_BASE_PCA301);

    /* initialize CLI */
    pinkie_printf("System: ready\n");
    res = acyclic_init(&g_a);
    if (res) {
        goto _bail;
    }

    /* register reg commands */
    res = regreg_acyclic_init(&g_a);
    if (res) {
        goto _bail;
    }

    /* enable interrupts */
    sei();

    /* handle input */
    while (!g_a.flg_exit) {
        if (pinkie_stdio_avail()) {
            acyclic_input(&g_a, (uint8_t) pinkie_stdio_getc());
        }

        pca301_process();
        pca301_rfm69_process();
    }

    pinkie_stdio_exit();

_bail:
    if (res) {
        pinkie_printf("System: error\n");
    }

    return res;
}


/*****************************************************************************/
/** RegReg NVS Register Access
 */
static unsigned int reg_nvs(
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
)
{
    PINKIE_UNUSED(reg);

    if (reg_acc->write_flg) {

        /* store data in NVS */
        if (0 == reg_acc->addr_ofs) {
            pinkie_printf("NVS: write\n");
            pinkie_nvs_write((uint8_t *) &data_nvs, sizeof(data_nvs));
            return 0;
        }
        else if (1 == reg_acc->addr_ofs) {
            pinkie_printf("NVS: invalidate\n");
            pinkie_nvs_write((uint8_t *) &reg_acc->addr_ofs, sizeof(uint16_t));
            return 0;
        }
    }

    return REGREG_RES_PROCEED;
}


/*****************************************************************************/
/** RegReg ATmega Register Access
 *
 * Ideas taken from:
 *   - Temperature: http://www.avrfreaks.net/forum/328p-internal-temperature
 *   - Voltage: https://code.google.com/archive/p/tinkerit/wikis/SecretVoltmeter.wiki
 */
static unsigned int reg_atmega(
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
)
{
    ACYCLIC_UNUSED(reg);

    /* read ATmega temperature (see manual chapter "Temperature Measurement") */
    if ((REG_ATMEGA_TEMP == reg_acc->addr_ofs) ||
        (REG_ATMEGA_VOLT == reg_acc->addr_ofs)) {

        /* temp and voltage aren't writeable */
        if (reg_acc->write_flg) {
            return 1;
        }

        /* select internal 1.1V voltage reference and enable channel ADC8 */
        ADMUX = (REG_ATMEGA_TEMP == reg_acc->addr_ofs) ? ((1 << REFS1) | (1 << REFS0) | (1 << MUX3)) : ((1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1));

        /* enable ADC and set the prescaler to div factor 16 */
        ADCSRA = (1 << ADEN) | (0x06 << ADPS0);

        /* wait for ADC initialization */
        _delay_ms(10);

        /* start ADC */
        ADCSRA |= (1 << ADSC);

        /* wait until ADC conversion is done */
        while (ADCSRA & (1 << ADSC));

        /* fetch the result in mV and use the given 25 °C as reference */
        if (REG_ATMEGA_TEMP == reg_acc->addr_ofs) {
            data_atmega.temp = ADCW + data_nvs.val_atmega_temp_corr;
        } else {
            data_atmega.volt = (1100L * PROJ_ATMEGA_VAL_VOLT_CORR) / ADCW;
        }
    }
    /* read timestamp in ms */
    else if (REG_ATMEGA_MS == reg_acc->addr_ofs) {
        if (!reg_acc->write_flg) {
            data_atmega.ms = pinkie_timer_get();
        } else {
            pinkie_timer_set(data_atmega.ms);
            return 0;
        }
    }

    return REGREG_RES_PROCEED;
}


/*****************************************************************************/
/** RegReg RFM69 Raw Register Access
 */
static unsigned int reg_rfm69(
    struct REG_ENTRY_T *reg,                    /**< register info */
    struct REG_ACC_T *reg_acc                   /**< register access info */
)
{
    ACYCLIC_UNUSED(reg);

    /* inform caller that only 1 register can be read per call */
    reg_acc->data_len = 1;

    /* check for invalid register access */
    switch (reg_acc->addr_ofs) {

        /* 117: read send time budget in sec */
        case PROJ_RFM69_REG_BUDGET:
            if (reg_acc->write_flg) {
                return 1;
            }

            *reg_acc->data.write_to = (rfm69_send_budget_ms_get() / 1000);
            break;

        /* 116: calibrate the RC oscillator (write-only) */
        case PROJ_RFM69_REG_OSC:
            if (!reg_acc->write_flg) {
                return 1;
            }
            rfm69_rc_osc_cal();
            break;

        /* 115: read RSSI value (read-only) */
        case PROJ_RFM69_REG_RSSI:
            if (reg_acc->write_flg) {
                return 1;
            }

            *reg_acc->data.write_to = rfm69_rssi_value(0);
            break;

        /* 114: read temperature (read-only) */
        case PROJ_RFM69_REG_TEMP:
            if (reg_acc->write_flg) {
                return 1;
            }

            *reg_acc->data.write_to = rfm69_temp() + data_nvs.val_rfm69_temp_corr;
            break;

        /* 0 - 113: RFM69 registers */
        default:
            if (!reg_acc->write_flg) {
                *reg_acc->data.write_to = rfm69_reg_read_raw(reg_acc->addr_ofs);
            } else {
                rfm69_reg_write_raw(reg_acc->addr_ofs, *reg_acc->data.read_from);
            }
    }

    return 0;

}


/*****************************************************************************/
/** RFM69 Interrupt
 */
ISR (INT0_vect)
{
    rfm69_flg_isr = 1;
}


/*****************************************************************************/
/** Control RFM69 Interrupt
 */
void rfm69_int_ctrl(
    uint8_t on                                  /**< interrupt on */
)
{
    if (on) {
        EIMSK |= (1 << INT0);
    } else {
        EIMSK &= ~(1 << INT0);
    }
}


/*****************************************************************************/
/** Announce register content to client
 *
 * This function sends the content of the data pointer to the listening client.
 * It doesn't internally update the register so a following read can produce
 * other or no results. This depends on the caller.
 */
void reg_ann(
    uint16_t addr,                              /**< register address */
    void *data,                                 /**< data */
    unsigned int len                            /**< data length */
)
{
    unsigned int cnt;                           /* counter */

    for (cnt = 0; cnt < len; cnt++) {
        pinkie_printf("%u: 0x%"PRIx8" ()\n", addr + cnt, ((uint8_t *) data)[cnt]);
    }
}
