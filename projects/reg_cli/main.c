/**
 * @brief Demonstration of RegReg with ACyCLIC CLI in PINKIE
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <pinkie.h>
#include <acyclic.h>
#include <regreg.h>
#include <regreg_acyclic.h>


/*****************************************************************************/
/* Local variables */
/*****************************************************************************/
static uint8_t reg_data[0x21] = { 0 };          /**< virtual registers */
static ACYCLIC_T g_a = { 0 };                   /**< ACyCLIC handle */


/*****************************************************************************/
/* Register definitions */
/*****************************************************************************/
static REG_ENTRY_T reg_info_0000_001f = {
    NULL,
    0x0000,
    0x001f,
    NULL,
    &reg_data[0],
};

static REG_ENTRY_T reg_info_0020_0020 = {
    NULL,
    0x0020,
    0x0020,
    NULL,
    &reg_data[0x1f]
};


/*****************************************************************************/
/** Main
 */
int main(
    void
)
{
    int res;                                    /* result */

    res = pinkie_stdio_init();

    if (!res) {
        res = acyclic_init(&g_a);
    }

    if (!res) {
        res = regreg_acyclic_init(&g_a);
    }

    if (!res) {
        /* add registers */
        reg_add(&reg_info_0000_001f);
        reg_add(&reg_info_0020_0020);
    }

    /* handle input */
    if (!res) {
        while (!g_a.flg_exit) {
            acyclic_input(&g_a, (uint8_t) pinkie_stdio_getc());
        }
    }

    pinkie_stdio_exit();

    return res;
}
