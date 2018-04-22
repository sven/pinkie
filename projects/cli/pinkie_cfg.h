/**
 * @brief PINKIE - Configuration
 *
 * Copyright (c) 2017, Sven Bachmann <dev@mcbachmann.de>
 *
 * Licensed under the MIT license, see LICENSE for details.
 */
#ifndef PINKIE_CFG_H
#define PINKIE_CFG_H


/* Configure the highest integer width that must be supported by printf.
 *
 * Allowed values are:
 *   1 - 8-bit
 *   2 - 16-bit
 *   4 - 32-bit
 *   8 - 64-bit
 */
#define PINKIE_CFG_PRINTF_MAX_INT       8


/* Configure the highest integer width that must be supported by sscanf.
 *
 * Allowed values are:
 *   1 - 8-bit
 *   2 - 16-bit
 *   4 - 32-bit
 *   8 - 64-bit
 */
#define PINKIE_CFG_SSCANF_MAX_INT       8


#endif /* PINKIE_CFG_H */
