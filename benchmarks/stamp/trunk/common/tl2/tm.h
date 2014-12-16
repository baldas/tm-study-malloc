#ifndef TM_H
#define TM_H 1
#include <stdio.h>

#define MAIN(argc, argv)              int main (int argc, char** argv)
#define MAIN_RETURN(val)              return val

#define GOTO_SIM()                    /* nothing */
#define GOTO_REAL()                   /* nothing */
#define IS_IN_SIM()                   (0)

#define SIM_GET_NUM_CPU(var)          /* nothing */

#define P_MEMORY_STARTUP(numThread)   /* nothing */
#define P_MEMORY_SHUTDOWN()           /* nothing */

#include <string.h>
#include <stm.h>
#include "thread.h"

#define TM_ARG                        STM_SELF,
#define TM_ARG_ALONE                  STM_SELF
#define TM_ARGDECL                    STM_THREAD_T* TM_ARG
#define TM_ARGDECL_ALONE              STM_THREAD_T* TM_ARG_ALONE
#define TM_PURE                       /* nothing */
#define TM_SAFE                       /* nothing */

#define TM_STARTUP(numThread)         STM_STARTUP()
#define TM_SHUTDOWN()                 STM_SHUTDOWN()

#define TM_THREAD_ENTER()             TM_ARGDECL_ALONE = STM_NEW_THREAD(); \
                                      STM_INIT_THREAD(TM_ARG_ALONE, thread_getId())
#define TM_THREAD_EXIT()              STM_FREE_THREAD(TM_ARG_ALONE)

#define SEQ_MALLOC(size)              malloc(size)
#define SEQ_FREE(ptr)                 free(ptr)
#define P_MALLOC(size)                malloc(size)
#define P_FREE(ptr)                   free(ptr)
#define TM_MALLOC(size)               STM_MALLOC(size)
#define TM_FREE(ptr)                  STM_FREE(ptr)

#define TM_BEGIN()                    STM_BEGIN_WR()
#define TM_BEGIN_RO()                 STM_BEGIN_RD()
#define TM_END()                      STM_END()
#define TM_RESTART()                  STM_RESTART()

#define TM_EARLY_RELEASE(var)         /* nothing */

#define TM_SHARED_READ(var)           STM_READ(var)
#define TM_SHARED_READ_P(var)         STM_READ_P(var)
#define TM_SHARED_READ_F(var)         STM_READ_F(var)

#define TM_SHARED_WRITE(var, val)     STM_WRITE((var), val)
#define TM_SHARED_WRITE_P(var, val)   STM_WRITE_P((var), val)
#define TM_SHARED_WRITE_F(var, val)   STM_WRITE_F((var), val)

#define TM_LOCAL_WRITE(var, val)      STM_LOCAL_WRITE(var, val)
#define TM_LOCAL_WRITE_P(var, val)    STM_LOCAL_WRITE_P(var, val)
#define TM_LOCAL_WRITE_F(var, val)    STM_LOCAL_WRITE_F(var, val)

#define TM_IFUNC_DECL                 /* nothing */
#define TM_IFUNC_CALL1(r, f, a1)      r = f(a1)
#define TM_IFUNC_CALL2(r, f, a1, a2)  r = f((a1), (a2))

#endif /* TM_H */

