/**
 * @brief PINKIE - Linux Architecture
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#include <pinkie.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <inttypes.h>


/*****************************************************************************/
/* Local variables */
/*****************************************************************************/
static struct termios g_term;                   /**< terminal settings */


/*****************************************************************************/
/** PINKIE STDIO Init
 */
int pinkie_stdio_init(
    void
)
{
    int res;                                    /* result */
    struct termios term_tmp;                    /* temporary termios data */

    /* store current terminal settings */
    res = tcgetattr(STDIN_FILENO, &g_term);
    if (res) {
        fprintf(stderr, "Couldn't read termios attributes: %i (%s)\n", errno, strerror(errno));
        return 1;
    }

    /* copy current settings to allow modification */
    term_tmp = g_term;

    /* local modes
     * - disable canonical mode (line by line input)
     * - disable character echo
     */
    term_tmp.c_lflag &= ~(unsigned int) (ICANON | ECHO);

    /* set attributes */
    res = tcsetattr(STDIN_FILENO, TCSANOW, &term_tmp);
    if (res) {
        fprintf(stderr, "Couldn't set termios attributes: %i (%s)\n", errno, strerror(errno));
        return 1;
    }

    return 0;
}


/*****************************************************************************/
/** PINKIE STDIO Exit
 */
int pinkie_stdio_exit(
    void
)
{
    int res;                                    /* result */

    /* restore previous terminal settings */
    res = tcsetattr(STDIN_FILENO, TCSANOW, &g_term);
    if (res) {
        fprintf(stderr, "Couldn't set termios attributes: %i (%s)\n", errno, strerror(errno));
        return 1;
    }

    return 0;
}
