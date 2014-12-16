#ifndef TM_H
#define TM_H 1

#include <stdlib.h>                   /* Defines size_t. */ 

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
#define TM_PURE                       [[transaction_pure]]
#define TM_SAFE                       [[transaction_safe]]

#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             /* nothing */
#define TM_THREAD_EXIT()              /* nothing */

#define SEQ_MALLOC(size)              malloc(size)
#define SEQ_FREE(ptr)                 free(ptr)
#define P_MALLOC(size)                malloc(size)
#define P_FREE(ptr)                   free(ptr)
#define TM_MALLOC(size)               malloc(size)
#define TM_FREE(ptr)                  free(ptr)

#define TM_BEGIN()                    __transaction [[atomic]] {
#define TM_BEGIN_RO()                 __transaction [[atomic]] {
#define TM_END()                      }
#define TM_RESTART()                  _ITM_abortTransaction (_ITM_getTransaction (), 2, 0)

#define TM_EARLY_RELEASE(var)         /* nothing */

#define TM_SHARED_READ(var)           var
#define TM_SHARED_READ_P(var)         var
#define TM_SHARED_READ_F(var)         var

#define TM_SHARED_WRITE(var, val)     var = val
#define TM_SHARED_WRITE_P(var, val)   var = val
#define TM_SHARED_WRITE_F(var, val)   var = val

#define TM_LOCAL_WRITE(var, val)      var = val
#define TM_LOCAL_WRITE_P(var, val)    var = val
#define TM_LOCAL_WRITE_F(var, val)    var = val

/* Indirect function call management */
#define TM_IFUNC_DECL                 /* nothing */
static
[[transaction_pure]]
__attribute__((used))
long
indirect_call1(unsigned long (*f)(const void *), void *a)
{
  return f(a);
}
static
[[transaction_pure]]
__attribute__((used))
long
indirect_call2(long (*f)(const void *, const void *), void *a1, void *a2)
{
  return f(a1, a2);
}
#define TM_IFUNC_CALL1(r, f, a1)      r = indirect_call1(f, a1)
#define TM_IFUNC_CALL2(r, f, a1, a2)  r = indirect_call2(f, a1, a2)

/* libitm.h stuff */
#ifdef __i386__
# define ITM_REGPARM __attribute__((regparm(2)))
#else
# define ITM_REGPARM
#endif
extern
TM_PURE
void* _ITM_getTransaction() ITM_REGPARM;

extern
TM_PURE
void _ITM_abortTransaction(void *, int, void *) ITM_REGPARM __attribute__((noreturn));

/* Additional annotations */
/* strncmp can be defined as a macro (FORTIFY_LEVEL) */
#ifdef strncmp
# undef strncmp
#endif
extern
TM_PURE
int strncmp
(__const char *__s1, __const char *__s2, size_t __n);

/* Note this is safe in current STAMP applications but not safe. */
extern
TM_PURE
void *memcpy(void *dest, const void *src, size_t n);

extern
TM_PURE
void __assert_fail (__const char *__assertion, __const char *__file,
                           unsigned int __line, __const char *__function)
     __attribute__ ((__noreturn__));

/* See http://software.intel.com/en-us/forums/showthread.php?t=77909. */
extern
TM_SAFE
void* malloc(size_t);

extern
TM_SAFE
void free(void*);

#endif /* TM_H */

