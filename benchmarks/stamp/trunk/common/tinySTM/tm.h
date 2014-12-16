#ifndef TM_H
#define TM_H 1

#define MAIN(argc, argv)            int main (int argc, char** argv)
#define MAIN_RETURN(val)            return val

#define GOTO_SIM()                  /* nothing */
#define GOTO_REAL()                 /* nothing */
#define IS_IN_SIM()                 (0)

#define SIM_GET_NUM_CPU(var)        /* nothing */

#define P_MEMORY_STARTUP(numThread) /* nothing */
#define P_MEMORY_SHUTDOWN()         /* nothing */

#include <stm.h>
#include <mod_mem.h>
#include <mod_stats.h>

#define TM_ARG                      /* nothing */
#define TM_ARG_ALONE                /* nothing */
#define TM_ARGDECL                  /* nothing */
#define TM_ARGDECL_ALONE            /* nothing */
#define TM_SAFE                     /* nothing */
#define TM_PURE                     /* nothing */


#define TM_STARTUP(numThread)       do { \
                                      char *s; \
                                      if (stm_get_parameter("compile_flags", &s)) { \
                                        fprintf(stdout, "STM flags    : %s\n", s); \
				      } \
				      stm_init(); \
                                      mod_mem_init(0); \
                                      if (getenv("STM_STATS") != NULL) \
                                        mod_stats_init(); \
                                    } while (0)
#define TM_SHUTDOWN()               if (getenv("STM_STATS") != NULL) { \
                                      unsigned long u; \
                                      if (stm_get_stats("global_nb_commits", &u) != 0) \
                                        printf("#commits    : %lu\n", u); \
                                      if (stm_get_stats("global_nb_aborts", &u) != 0) \
                                        printf("#aborts     : %lu\n", u); \
                                      if (stm_get_stats("global_max_retries", &u) != 0) \
                                        printf("Max retries : %lu\n", u); \
                                    } \
                                    stm_exit()

#define TM_THREAD_ENTER()           stm_init_thread()
#define TM_THREAD_EXIT()            stm_exit_thread()

#define SEQ_MALLOC(size)            malloc(size)
#define SEQ_FREE(ptr)               free(ptr)
#define P_MALLOC(size)              malloc(size)
#define P_FREE(ptr)                 free(ptr)
#define TM_MALLOC(size)             stm_malloc(size)
/* Note that we only lock the first word and not the complete object */
//#define TM_FREE(ptr)                stm_free(ptr, malloc_usable_size(ptr))
//#define TM_FREE(ptr)                stm_free(ptr, tc_malloc_size(ptr))
#define TM_FREE(ptr)                stm_free(ptr, sizeof(stm_word_t))
//#define TM_FREE(ptr)		    stm_free(ptr, dlmalloc_usable_size(ptr))
//#define TM_FREE(ptr)		    stm_free(ptr, scalable_msize(ptr))
#define TM_FREE2(ptr, size)         stm_free(ptr, size)

#if STM_VERSION_NB <= 103
# define TM_START(ro)               do { \
                                      sigjmp_buf *_e; \
                                      stm_tx_attr_t _a = {0, ro}; \
                                      _e = stm_start(&_a); \
                                      sigsetjmp(*_e, 0); \
                                    } while (0)
#else /* STM_VERSION_NB > 103 */
# define TM_START(ro)               do { \
                                      sigjmp_buf *_e; \
                                      stm_tx_attr_t _a = {{.id = 0, .read_only = ro, .visible_reads = 0}}; \
                                      _e = stm_start(_a); \
                                      sigsetjmp(*_e, 0); \
                                    } while (0)
#endif /* STM_VERSION_NB > 103 */
#define TM_BEGIN()		    TM_START(0)
#define TM_BEGIN_RO()               TM_START(1)
#define TM_END()                    stm_commit()
#define TM_RESTART()                stm_abort(0)

#define TM_EARLY_RELEASE(var)       /* nothing */

#include <wrappers.h>

/* We could also map macros to the stm_(load|store)_long functions if needed */

typedef union { stm_word_t w; float f;} floatconv_t;

#define TM_SHARED_READ(var)           stm_load((volatile stm_word_t *)(void *)&(var))
#define TM_SHARED_READ_P(var)         stm_load_ptr((volatile void **)(void *)&(var))
#define TM_SHARED_READ_F(var)         stm_load_float((volatile float *)(void *)&(var))
//#define TM_SHARED_READ_P(var)         stm_load((volatile stm_word_t *)(void *)&(var))
//#define TM_SHARED_READ_F(var)         ({floatconv_t c; c.w = stm_load((volatile stm_word_t *)&(var)); c.f;})

#define TM_SHARED_WRITE(var, val)     stm_store((volatile stm_word_t *)(void *)&(var), (stm_word_t)val)
#define TM_SHARED_WRITE_P(var, val)   stm_store_ptr((volatile void **)(void *)&(var), val)
#define TM_SHARED_WRITE_F(var, val)   stm_store_float((volatile float *)(void *)&(var), val)
//#define TM_SHARED_WRITE_P(var, val)   stm_store((volatile stm_word_t *)(void *)&(var), (stm_word_t)val)
//#define TM_SHARED_WRITE_F(var, val)   ({floatconv_t c; c.f = val; stm_store((volatile stm_word_t *)&(var), c.w);})

/* TODO: test with mod_log */
#define TM_LOCAL_WRITE(var, val)      var = val
#define TM_LOCAL_WRITE_P(var, val)    var = val
#define TM_LOCAL_WRITE_F(var, val)    var = val

#define TM_IFUNC_DECL
#define TM_IFUNC_CALL1(r, f, a1)      r = f(a1)
#define TM_IFUNC_CALL2(r, f, a1, a2)  r = f((a1), (a2))

#endif /* TM_H */

