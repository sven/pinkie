/**
 * @brief Demonstration of ACyCLIC CLI in PINKIE
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <pinkie.h>
#include <acyclic.h>


/*****************************************************************************/
/* Local prototypes */
/*****************************************************************************/
uint8_t cmd_help_func(struct ACYCLIC_T *a);
uint8_t cmd_greet_func(struct ACYCLIC_T *a);
uint8_t cmd_exit_func(struct ACYCLIC_T *a);
uint8_t cmd_test_func(struct ACYCLIC_T *a);


/*****************************************************************************/
/* Commands */
/*****************************************************************************/
ACYCLIC_CMD(test_one, "one", NULL, NULL, NULL);
ACYCLIC_CMD(test, "test", NULL, &cmd_test_one, cmd_test_func);

ACYCLIC_CMD(sametext, "sametext", &cmd_test, NULL, NULL);
ACYCLIC_CMD(same, "same", &cmd_sametext, NULL, NULL);

ACYCLIC_CMD(rocky, "rocky", &cmd_same, NULL, NULL);
ACYCLIC_CMD(rockie, "rockie", &cmd_rocky, NULL, NULL);

ACYCLIC_CMD(help_speex, "speex", NULL, NULL, NULL);
ACYCLIC_CMD(help_smaxx, "smaxx", &cmd_help_speex, NULL, NULL);
ACYCLIC_CMD(help_snufz, "snufz", &cmd_help_smaxx, NULL, NULL);
ACYCLIC_CMD(help_greet, "greet", &cmd_help_snufz, NULL, NULL);
ACYCLIC_CMD(help, "help", &cmd_rockie, &cmd_help_greet, cmd_help_func);

ACYCLIC_CMD(greet_me, "me", NULL, NULL, NULL);
ACYCLIC_CMD(greet, "greet", &cmd_help, &cmd_greet_me, cmd_greet_func);

ACYCLIC_CMD(exit, "exit", &cmd_greet, NULL, cmd_exit_func);


/*****************************************************************************/
/* Variables */
/*****************************************************************************/
ACYCLIC_T g_a = { 0 };                          /**< ACyCLIC handle */


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
        res = acyclic_cmd_add(&g_a, &g_a.cmds, &cmd_exit);
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


/*****************************************************************************/
/** Help
 */
uint8_t cmd_help_func(
    struct ACYCLIC_T *a
)
{
    ACYCLIC_UNUSED(a);

    pinkie_stdio_putc('\n');
    pinkie_printf("********\n");
    pinkie_printf("* help *\n");
    pinkie_printf("********\n");
    pinkie_printf("[help]   greet - show greeting\n");
    pinkie_stdio_putc('\n');

    return 0;
}


/*****************************************************************************/
/** Test
 */
uint8_t cmd_test_func(
    struct ACYCLIC_T *a
)
{
    unsigned int cnt_arg;
    unsigned int pos_arg;

    if (a->arg_cnt) {
        pinkie_printf("args: ");
        for (cnt_arg = 0; cnt_arg < a->arg_cnt; cnt_arg++) {
            ACYCLIC_PLAT_PUTC('[');
            if (a->args[cnt_arg].cmd) {
                ACYCLIC_PLAT_PUTC('*');
            }
            for (pos_arg = 0; pos_arg < a->args[cnt_arg].len; pos_arg++) {
                ACYCLIC_PLAT_PUTC(a->args[cnt_arg].name[pos_arg]);
            }
            pinkie_printf("] ");
        }
        pinkie_stdio_putc('\n');
    }

    return 0;
}


/*****************************************************************************/
/** Greet
 */
uint8_t cmd_greet_func(
    struct ACYCLIC_T *a
)
{
    uint8_t cnt;

#if ACYCLIC_DBG == 0
    ACYCLIC_UNUSED(a);
#endif

    pinkie_stdio_putc('\n');
    pinkie_printf("*********\n");
    pinkie_printf("* greet *\n");
    pinkie_printf("*********\n");
    pinkie_printf("[greet]  hello from me\n");
    pinkie_printf("[greet]  arguments:\n");

    for (cnt = 0; cnt < a->arg_cnt; cnt++) {
        pinkie_printf("[greet]  arg: %.*s\n", a->args[cnt].len, a->args[cnt].name);
    }

    pinkie_stdio_putc('\n');

    return 0;
}


/*****************************************************************************/
/** Exit
 */
uint8_t cmd_exit_func(
    struct ACYCLIC_T *a
)
{
    ACYCLIC_UNUSED(a);

    a->flg_exit = 1;

    pinkie_stdio_putc('\n');
    pinkie_printf("********\n");
    pinkie_printf("* exit *\n");
    pinkie_printf("********\n");
    pinkie_printf("[exit]  bye\n");
    pinkie_stdio_putc('\n');

    return 0;
}
