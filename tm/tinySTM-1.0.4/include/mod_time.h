/*
 * File:
 *   mod_time.h
 * Author(s):
 *   Alexandro Baldassin <alex@rc.unesp.br>
 * Description:
 *   Module for collecting useful and abort time.
 *
 * Copyright (c) 2007-2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file
 *   Module for collecting useful and abort time. This module holds
 *   the number of ticks spent executing successful and failed
 *   transactions.
 * @author
 *   Alexandro Baldassin <alex@rc.unesp.br>
 * @date
 *   2007-2012
 */

#ifndef _MOD_TIME_H_
# define _MOD_TIME_H_

# include "stm.h"
# include "mod_mem.h"

# ifdef __cplusplus
extern "C" {
# endif

//void print_mod_time_stats();
int stm_get_time_stats(const char *name, void *val);


/**
 * Initialize the module.  This function must be called once, from the
 * main thread, after initializing the STM library and before
 * performing any transactional operation.
 */
void mod_time_init();

# ifdef __cplusplus
}
# endif

#endif /* _MOD_TIME_H_ */
