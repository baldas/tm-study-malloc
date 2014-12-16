/* =============================================================================
 *
 * tm-tiny++.h
 *
 * Utility defines for transactional memory
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Authors: Chi Cao Minh and Martin Trautmann
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 *
 * ------------------------------------------------------------------------
 *
 * For the license of ssca2, please see ssca2/COPYRIGHT
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 *
 * ------------------------------------------------------------------------
 *
 * Unless otherwise noted, the following license applies to STAMP files:
 *
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#ifndef TM_TINYPP_H
#define TM_TINYPP_H 1

/* =============================================================================
 * TinySTM++
 * =============================================================================
 */
#   include <string.h>
/* We can't always inline STM functions. */
#define TANGER_LOADSTORE_ATTR __attribute__((nothrow))
#   include <tanger-stm-internal.h>
#   include "thread.h"

#   define TM_ARG                    tx,   //stm_tx_t * 
#   define TM_ARG_ALONE              tx //stm_tx_t *
#   define TM_ARGDECL                tanger_stm_tx_t* tx,   // STM_THREAD_T* TM_ARG
#   define TM_ARGDECL_ALONE          tanger_stm_tx_t* tx// STM_THREAD_T* TM_ARG_ALONE
#   define TM_CALLABLE                   /* nothing */


#   define TM_STARTUP(numThread)     _ITM_initializeProcess()
#   define TM_SHUTDOWN()

#   define TM_THREAD_ENTER()         tanger_stm_tx_t * tx = tanger_stm_get_tx()
#   define TM_THREAD_EXIT()

#   define P_MALLOC(size)            malloc(size)
#   define P_FREE(ptr)               free(ptr)
#   define TM_MALLOC(size)           tanger_stm_malloc(size)
#   define TM_FREE(ptr)              tanger_stm_free(ptr)

/* XXX: this will run instrumented code, even when serial mode is active */
#   define TM_BEGIN()                if(1){ _ITM_beginTransaction(pr_instrumentedCode); }
#   define TM_BEGIN_RO()             if(1){ _ITM_beginTransaction(pr_instrumentedCode); }
#   define TM_END()                  _ITM_commitTransaction()
#   define TM_RESTART()              _ITM_abortTransaction(userAbort)

#   define TM_EARLY_RELEASE(var)       /* nothing */


/* LOAD STORE */

typedef union { uint32_t i; float f;} tanger_stm_floatconv_t;
#if __WORDSIZE == 32
# define LOADINT(addr)                 ((int)tanger_stm_load32(tx, (uint32_t*)(addr)))
# define STOREINT(addr, value)         tanger_stm_store32(tx, (uint32_t*)(addr), (uint32_t) value)
# define LOADPTR(addr)                 (void*)tanger_stm_load32(tx, (uint32_t*)(addr)))
# define STOREPTR(addr, value)         tanger_stm_store32(tx, (uint32_t*)(addr), (uint32_t*)value)
# define LOADFLOAT(addr)               ({tanger_stm_floatconv_t c; c.i = tanger_stm_load32(tx, (uint32_t*)(addr)); c.f;})
# define STOREFLOAT(addr, value)       ({tanger_stm_floatconv_t c; c.f = value; tanger_stm_store32(tx, (uint32_t*)(addr), c.i);})
#elif __WORDSIZE == 64
# define LOADINT(addr)                 ((int)tanger_stm_load32(tx, (uint32_t*)(addr)))
# define STOREINT(addr, value)         tanger_stm_store32(tx, (uint32_t*)(addr), (uint32_t)(value))
# define LOADPTR(addr)                 ((void*)tanger_stm_load64(tx, (uint64_t*)(addr)))
# define STOREPTR(addr, value)         tanger_stm_store64(tx, (uint64_t*)(addr), (uint64_t)(value))
# define LOADFLOAT(addr)               ({tanger_stm_floatconv_t c; c.i = tanger_stm_load32(tx, (uint32_t*)(addr)); c.f;})
# define STOREFLOAT(addr, value)       ({tanger_stm_floatconv_t c; c.f = value; tanger_stm_store32(tx, (uint32_t*)(addr), c.i);})
#else
#error cannot determine wordsize
#endif

# define TM_SHARED_READ(var)           LOADINT(&var)
# define TM_SHARED_READ_P(var)         LOADPTR(&var)
# define TM_SHARED_READ_F(var)         LOADFLOAT(&var)

# define TM_SHARED_WRITE(var, val)     STOREINT((&var), val)
# define TM_SHARED_WRITE_P(var, val)   STOREPTR((&var), val)
# define TM_SHARED_WRITE_F(var, val)   STOREFLOAT((&var), val)

# define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
# define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
# define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})

# define TM_IFUNC_DECL                 /* nothing */
# define TM_IFUNC_CALL1(r, f, a1)      r = f(a1)
# define TM_IFUNC_CALL2(r, f, a1, a2)  r = f((a1), (a2))

#endif /* TM_TINYPP_H */


/* =============================================================================
 *
 * End of tm-tiny++.h
 *
 * =============================================================================
 */
