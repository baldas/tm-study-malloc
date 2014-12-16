#ifndef TM_H
#define TM_H 1

#define MAIN(argc, argv)              int main (int argc, char** argv)
#define MAIN_RETURN(val)              return val

#define GOTO_SIM()                    /* nothing */
#define GOTO_REAL()                   /* nothing */
#define IS_IN_SIM()                   (0)

#define SIM_GET_NUM_CPU(var)          /* nothing */

#define P_MEMORY_STARTUP(numThread)   /* nothing */
#define P_MEMORY_SHUTDOWN()           /* nothing */

#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */
#define TM_PURE                       __attribute__((transaction_pure))
#define TM_SAFE                       __attribute__((transaction_safe))

#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             /* nothing */
#define TM_THREAD_EXIT()              /* nothing */

#define TM_BEGIN()                    __tm_atomic {
#define TM_BEGIN_RO()                 __tm_atomic {
#define TM_END()                      }
#define TM_RESTART()                  assert(0)

#define SEQ_MALLOC(size)              malloc(size)
#define SEQ_FREE(ptr)                 free(ptr)
#define P_MALLOC(size)	              malloc(size)
#define P_FREE(ptr)		      free(ptr)
#define TM_MALLOC(size)               malloc(size)
#define TM_FREE(ptr)                  free(ptr)

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

/* TODO indirect function calls to test */
#define TM_IFUNC_DECL                 /* nothing */
#define TM_IFUNC_CALL1(r, f, a1)      r = f(a1)
#define TM_IFUNC_CALL2(r, f, a1, a2)  r = f((a1), (a2))

/* just include all wrapper declarations that might be necessary */
#include <tanger-stm-std-math.h>
#include <tanger-stm-std-string.h>

/* strncmp / strcmp */
#ifdef strncmp
# undef strncmp
#endif
extern TM_PURE
int strncmp(const char *s1, const char *s2, size_t n);

/* TODO required? */
/* TM_PURE long thread_getId(void); */
//static long tanger_wrapperpure_thread_getId(void) __attribute__ ((weakref("thread_getId")));

/* For Bayes */
/*
#include "tanger-stm-internal.h"
#include <alloca.h>
void qsort(void *base, size_t nmemb, size_t size,
        int(*compar)(const void *, const void *))
__attribute__((tm_wrapper("tanger_stm_std_qsort")));

void __attribute__((weak,used)) tanger_stm_std_qsort(void *base, size_t nmemb, size_t size,
        int(*compar)(const void *, const void *))
{
    size_t s = size * nmemb;
    void* buf = alloca(s);
    _ITM_memcpyRtWn(buf, base, s);
    qsort(buf, nmemb, size, compar);
    _ITM_memcpyRnWt(base, buf, s);
}
*/

#endif /* TM_H */


