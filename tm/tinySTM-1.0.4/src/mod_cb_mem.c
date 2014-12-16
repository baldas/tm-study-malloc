/*
 * File:
 *   mod_cb_mem.c
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *   Patrick Marlier <patrick.marlier@unine.ch>
 * Description:
 *   Module for user callback and for dynamic memory management.
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
 *
 * This program has a dual license and can also be distributed
 * under the terms of the MIT license.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "mod_cb.h"
#include "mod_mem.h"

/* TODO use stm_internal.h for faster accesses */
#include "stm.h"
#include "utils.h"
#include "gc.h"

/*
 * After some time I got bored (and confused!) about using #ifdefs
 * to implement different optimizations and profilings. Therefore,
 * I decided to split the module into 3 parts:
 *   1) a prologue with common types and global variables
 *   2) a specific implementation for the static functions (varies with each
 *     optimization)
 *   3) an epilogue with the API calls (which will call the static inline
 *     functions in 2).
 *
 * Only the file in 2) need to be changed when a new optimization is implemented.
 */


// 1)
#include "mod_cb_mem.prologue"

// 2)
#ifdef PROFILE_ALLOC
# include "mod_cb_mem.profile"
#elif PREALLOC_MEMORY
# include "mod_cb_mem.prealloc"
#elif ALLOC_OPT_CACHE
# include "mod_cb_mem.cache"
#else  // original, in case none of the defines match
# include "mod_cb_mem.original"
#endif

// 3)
#include "mod_cb_mem.epilogue"


