#ifndef TM_H
#define TM_H 1

# define MAIN(argc, argv)              int main (int argc, char** argv)
# define MAIN_RETURN(val)              return val

# define GOTO_SIM()                    /* nothing */
# define GOTO_REAL()                   /* nothing */
#define IS_IN_SIM()                   (0)

#define SIM_GET_NUM_CPU(var)          /* nothing */

#define P_MEMORY_STARTUP(numThread)   /* nothing */
#define P_MEMORY_SHUTDOWN()           /* nothing */

#include <assert.h>

#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_PURE                       /* nothing */
#define TM_SAFE                       /* nothing */

#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             /* nothing */
#define TM_THREAD_EXIT()              /* nothing */

#define SEQ_MALLOC(size)              malloc(size)
#define SEQ_FREE(ptr)                 free(ptr)
#define P_MALLOC(size)                malloc(size)
#define P_FREE(ptr)                   free(ptr)
#ifndef MALLOC_COUNT
# define TM_MALLOC(size)               malloc(size)
# define TM_FREE(ptr)                  free(ptr)
# define TM_FREE2(ptr, size)           free(ptr)
#else
# define TM_MALLOC(size)               malloc_enter_region(TX_REGION) ? malloc(size):0; malloc_exit_current_region()
# define TM_FREE(ptr)                  malloc_enter_region(TX_REGION); free(ptr); malloc_exit_current_region()
# define TM_FREE2(ptr, size)           malloc_enter_region(TX_REGION); free(ptr); malloc_exit_current_region()
#endif /* MALLOC_COUNT */

#include <pthread.h>
static pthread_mutex_t gmutex = PTHREAD_MUTEX_INITIALIZER;

#define TM_BEGIN()                    pthread_mutex_lock(&gmutex)
#define TM_BEGIN_RO()                 pthread_mutex_lock(&gmutex)
#define TM_END()                      pthread_mutex_unlock(&gmutex)
#define TM_RESTART()                  assert(0)

#define TM_EARLY_RELEASE(var)         /* nothing */

#define TM_SHARED_READ(var)           (var)
#define TM_SHARED_READ_P(var)         (var)
#define TM_SHARED_READ_F(var)         (var)

#define TM_SHARED_WRITE(var, val)     var = val
#define TM_SHARED_WRITE_P(var, val)   var = val
#define TM_SHARED_WRITE_F(var, val)   var = val

#define TM_LOCAL_WRITE(var, val)      var = val
#define TM_LOCAL_WRITE_P(var, val)    var = val
#define TM_LOCAL_WRITE_F(var, val)    var = val

#define TM_IFUNC_DECL                 /* nothing */
#define TM_IFUNC_CALL1(r, f, a1)      r = f(a1)
#define TM_IFUNC_CALL2(r, f, a1, a2)  r = f((a1), (a2))

#endif /* TM_H */

