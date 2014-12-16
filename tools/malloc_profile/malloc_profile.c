/******************************************************************************
 * malloc_profile.c
 *
 * This is adapted from malloc_count (header appended below).
 * I removed some stuff which were not required and added others (e.g. malloc
 * counters).
 *
 * Alexandro Baldassin.
 ******************************************************************************
 *
 *
 * malloc() allocation counter based on http://ozlabs.org/~jk/code/ and other
 * code preparing LD_PRELOAD shared objects.
 *
 ******************************************************************************
 * Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <dlfcn.h>

#include "malloc_profile.h"

/* user-defined options for output malloc()/free() operations to stderr */

static const int log_operations = 0;    /* <-- set this to 1 for log output */
static const size_t log_operations_threshold = 1024*1024;

/* option to use gcc's intrinsics to do thread-safe statistics operations */
#define THREAD_SAFE_GCC_INTRINSICS      0

#if THREAD_SAFE_GCC_INTRINSICS
#include "atomic.h" 
#include <pthread.h>
#endif

/* function pointer to the real procedures, loaded using dlsym */
typedef void* (*malloc_type)(size_t);
typedef void  (*free_type)(void*);
typedef void* (*realloc_type)(void*, size_t);
typedef int (*posix_memalign_type)(void **, size_t, size_t);

static malloc_type real_malloc = NULL;
static free_type real_free = NULL;
static realloc_type real_realloc = NULL;
static posix_memalign_type real_posix_memalign = NULL;

/* a sentinel value prefixed to each allocation */
static const size_t sentinel = 0xDEADC0DE;

/* a simple memory heap for allocations prior to dlsym loading */
#define INIT_HEAP_SIZE 1024*1024
static char init_heap[INIT_HEAP_SIZE];
static size_t init_heap_use = 0;
static const int log_operations_init_heap = 0;

/*****************************************/
/* run-time memory allocation statistics */
/*****************************************/

static region_type current_region = SEQ_REGION;
static region_type last_region = SEQ_REGION;

static long long peak = 0, curr = 0, total = 0;
static long long mallocs_peak = 0, mallocs_curr = 0;

#define NUM_BINS 10
typedef struct region_stats {
  long long mallocs;
  long long frees;
  long long mem_allocated;
  long bins[NUM_BINS];
} region_stats_t;

static region_stats_t sequential, parallel, transactional;


int bin_size[] = { 16, 32, 48, 64, 96, 128, 256, 512, 1024, 1025 };

static void inc_bin(size_t size)
{
  int bin = 0;

  if (size < 17) bin = 0;
  else if (size < 33) bin = 1;
  else if (size < 49) bin = 2;
  else if (size < 65) bin = 3;
  else if (size < 97) bin = 4;
  else if (size < 129) bin = 5;
  else if (size < 257) bin = 6;
  else if (size < 513) bin = 7;
  else if (size < 1025) bin = 8;
  else bin = 9;

  if (current_region == SEQ_REGION)
    sequential.bins[bin]++;
  else if (current_region == PAR_REGION)
    parallel.bins[bin]++;
  else transactional.bins[bin]++;

}

static void print_bin()
{
  int i;

  long long total_m = 0;

  total_m = 0;
  fprintf(stdout, "==========\n");
  fprintf(stdout, "bins: ");
  for (i=0; i<NUM_BINS-1; i++) {
    fprintf(stdout, "%d  ", bin_size[i]);
  }
  fprintf(stdout, ">1024 mallocs frees mem-allocated \n");

  fprintf(stdout, "sequential: ");
  for (i=0; i<NUM_BINS; i++) {
    fprintf(stdout, "%'ld  ", sequential.bins[i]);
    total_m += sequential.bins[i];
  }
  fprintf(stdout, "%'lld  %'lld  %'lld\n", 
      sequential.mallocs, sequential.frees, sequential.mem_allocated);

  fprintf(stdout, "parallel: ");
  for (i=0; i<NUM_BINS; i++) {
    fprintf(stdout, "%'ld  ", parallel.bins[i]);
    total_m += parallel.bins[i];
  }
  fprintf(stdout, "%'lld  %'lld  %'lld\n", 
      parallel.mallocs, parallel.frees, parallel.mem_allocated);
  
  fprintf(stdout, "transactional: ");
  for (i=0; i<NUM_BINS; i++) {
    fprintf(stdout, "%'ld  ", transactional.bins[i]);
    total_m += transactional.bins[i];
  }
  fprintf(stdout, "%'lld  %'lld  %'lld\n", 
      transactional.mallocs, transactional.frees, transactional.mem_allocated);
  
  fprintf(stdout, "==========\n");
  fprintf(stdout, "total = %'lld\n", total_m);
}

static malloc_count_callback_type callback = NULL;
static void* callback_cookie = NULL;

/* add allocation to statistics */
static void inc_count(size_t inc)
{
#if THREAD_SAFE_GCC_INTRINSICS
    long long mycurr = ATOMIC_FETCH_ADD_FULL(&curr, inc);
    mycurr += inc;
    ATOMIC_FETCH_ADD_FULL(&total, inc);
#else
    if ((curr += inc) > peak) peak = curr;
    if ((mallocs_curr+1) > mallocs_peak) mallocs_peak = mallocs_curr;
    
    total += inc;
    mallocs_curr++;

    if (current_region == SEQ_REGION) {
      sequential.mallocs++;
      sequential.mem_allocated += inc;
    }
    else if (current_region == PAR_REGION) {
      parallel.mallocs++;
      parallel.mem_allocated += inc;
    }
    else {
      transactional.mallocs++;
      transactional.mem_allocated += inc;
    }

    if (callback) 
      callback(callback_cookie, curr, sequential.mallocs+parallel.mallocs, sequential.frees+parallel.frees, transactional.mallocs, transactional.frees);
#endif
}

/* decrement allocation to statistics */
static void dec_count(size_t dec)
{
#if THREAD_SAFE_GCC_INTRINSICS
    long long mycurr = ATOMIC_FETCH_ADD_FULL(&curr, -1*dec);
    mycurr -= dec;
#else
    
    curr -= dec;
    mallocs_curr--;
    
    if (current_region == SEQ_REGION) {
      sequential.frees++;
    }
    else if (current_region == PAR_REGION) {
      parallel.frees++;
    }
    else {
      transactional.frees++;
    }

    if (callback) 
      callback(callback_cookie, curr, sequential.mallocs+parallel.mallocs, sequential.frees+parallel.frees, transactional.mallocs, transactional.frees);

#endif
}


/* We always return true - can be used by macros which require assignments */
extern int malloc_enter_region(region_type reg)
{
  last_region = current_region;
  current_region = reg;
  return 1;
}

extern void malloc_exit_current_region(void)
{
  current_region = last_region;
}

/* user function to return the currently allocated amount of memory */
extern size_t malloc_count_current(void)
{
    return curr;
}

/* user function to return the peak allocation */
extern size_t malloc_count_peak(void)
{
    return peak;
}

/* user function to reset the peak allocation to current */
extern void malloc_count_reset_peak(void)
{
    peak = curr;
}

/* user function which prints current and peak allocation to stderr */
extern void malloc_count_print_status(void)
{
    fprintf(stderr,"malloc_count ### current %'lld, peak %'lld\n",
            curr, peak);
}

/* user function to supply a memory profile callback */
void malloc_count_set_callback(malloc_count_callback_type cb, void* cookie)
{
    callback = cb;
    callback_cookie = cookie;
}

/****************************************************/
/* exported symbols that overlay the libc functions */
/****************************************************/

void *last_ret = NULL;

/* exported malloc symbol that overrides loading from libc */
extern void* malloc(size_t size)
{
    void* ret;

    if (size == 0) return NULL;
    if (real_malloc)
    {
        /* call read malloc procedure in libc */
        ret = (*real_malloc)(2*sizeof(size_t) + size);

        inc_bin(size);
        inc_count(size);
        if (log_operations && size >= log_operations_threshold) {
            fprintf(stderr,"malloc_count ### malloc(%'lld) = %p   (current %'lld)\n",
                    (long long)size, (char*)ret + 2*sizeof(size_t), curr);
        }

        /* prepend allocation size and check sentinel */
        ((size_t*)ret)[0] = size;
        ((size_t*)ret)[1] = sentinel;

        return (char*)ret + 2*sizeof(size_t);
    }
    else
    {
        if (init_heap_use + sizeof(size_t) + size > INIT_HEAP_SIZE) {
            fprintf(stderr,"malloc_count ### init heap full !!!\n");
            exit(EXIT_FAILURE);
        }

        ret = init_heap + init_heap_use;
        init_heap_use += 2*sizeof(size_t) + size;

        /* prepend allocation size and check sentinel */
        ((size_t*)ret)[0] = size;
        ((size_t*)ret)[1] = sentinel;

        if (log_operations_init_heap) {
            fprintf(stderr,"malloc_count ### malloc(%'lld) = %p   on init heap\n",
                    (long long)size, (char*)ret + 2*sizeof(size_t));
        }

        return (char*)ret + 2*sizeof(size_t);
    }
}

/* exported posix_memalign symbol that overrides loading from libc */
extern int posix_memalign (void **memptr, size_t alignment, size_t size)
{
    int ret;
    void *memret;

    if (size == 0) return 0 /*NULL */;

    if (real_posix_memalign)
    {
        /* call read posix_memalign procedure in libc */
        ret = (*real_posix_memalign)(&memret, alignment, 2*sizeof(size_t) + size);

        inc_bin(size);
        inc_count(size);
	
        if (log_operations && size >= log_operations_threshold) {
            fprintf(stderr,"malloc_count ### posix_memalign(%p, %ld, %'lld) = %d   (current %'lld)\n",
                    memptr, alignment, (long long)size, ret, curr);
        }
        
	/* prepend allocation size and check sentinel */
        ((size_t*)memret)[0] = size;
        ((size_t*)memret)[1] = sentinel;
	
	*memptr = (char *)memret + 2*sizeof(size_t);

        return ret;
    }
    else
    {
        fprintf(stderr,"posix_memalign not supported during heap initialization !!!\n");
        exit(EXIT_FAILURE);
    }
}

extern void *aligned_alloc(size_t alignment, size_t size)
{
    fprintf(stderr, "aligned_alloc not supported");
    exit(EXIT_FAILURE);
}

extern void *valloc(size_t size)
{
    fprintf(stderr, "valloc not supported");
    exit(EXIT_FAILURE);
}


/* exported free symbol that overrides loading from libc */
extern void free(void* ptr)
{
    size_t size;

    if (!ptr) return;   /* free(NULL) is no operation */

    if ((char*)ptr >= init_heap &&
        (char*)ptr <= init_heap + init_heap_use)
    {
        if (log_operations_init_heap) {
            fprintf(stderr,"malloc_count ### free(%p)   on init heap\n", ptr);
        }
        return;
    }

    if (!real_free) {
        fprintf(stderr,"malloc_count ### free(%p) outside init heap and without real_free !!!\n", ptr);
        return;
    }

    ptr = (char*)ptr - 2*sizeof(size_t);

    if (((size_t*)ptr)[1] != sentinel) {
        fprintf(stderr,"malloc_count ### free(%p) has no sentinel !!! memory corruption?\n", ptr);
    }

    size = ((size_t*)ptr)[0];
    dec_count(size);

    if (log_operations && size >= log_operations_threshold) {
        fprintf(stderr,"malloc_count ### free(%p) -> %'lld   (current %'lld)\n",
                ptr, (long long)size, curr);
    }

    (*real_free)(ptr);
}

/* exported calloc() symbol that overrides loading from libc, implemented using our malloc */
extern void* calloc(size_t nmemb, size_t size)
{
    void* ret;
    size *= nmemb;
    if (!size) return NULL;
    ret = malloc(size);
    memset(ret, 0, size);
    return ret;
}

/* exported realloc() symbol that overrides loading from libc */
extern void* realloc(void* ptr, size_t size)
{
    void* newptr;
    size_t oldsize;

    if ((char*)ptr >= (char*)init_heap &&
        (char*)ptr <= (char*)init_heap + init_heap_use)
    {
        if (log_operations_init_heap) {
            fprintf(stderr,"malloc_count ### realloc(%p) = on init heap\n", ptr);
        }

        ptr = (char*)ptr - 2*sizeof(size_t);

        if (((size_t*)ptr)[1] != sentinel) {
            fprintf(stderr,"malloc_count ### realloc(%p) has no sentinel !!! memory corruption?\n", ptr);
        }

        oldsize = ((size_t*)ptr)[0];

        if (oldsize >= size) {
            /* keep old area, just reduce the size */
            ((size_t*)ptr)[0] = size;
            return (char*)ptr + 2*sizeof(size_t);
        }
        else {
            /* allocate new area and copy data */
            ptr = (char*)ptr + 2*sizeof(size_t);
            newptr = malloc(size);
            memcpy(newptr, ptr, oldsize);
            free(ptr);
            return newptr;
        }
    }

    if (size == 0) { /* special case size == 0 -> free() */
        free(ptr);
        return NULL;
    }

    if (ptr == NULL) { /* special case ptr == 0 -> malloc() */
        return malloc(size);
    }

    ptr = (char*)ptr - 2*sizeof(size_t);

    if (((size_t*)ptr)[1] != sentinel) {
        fprintf(stderr,"malloc_count ### free(%p) has no sentinel !!! memory corruption?\n", ptr);
    }

    oldsize = ((size_t*)ptr)[0];

    dec_count(oldsize);
    inc_bin(size);
    inc_count(size);

    newptr = (*real_realloc)(ptr, 2*sizeof(size_t) + size);

    if (log_operations && size >= log_operations_threshold)
    {
        if (newptr == ptr)
            fprintf(stderr,"malloc_count ### realloc(%'lld -> %'lld) = %p   (current %'lld)\n",
                   (long long)oldsize, (long long)size, newptr, curr);
        else
            fprintf(stderr,"malloc_count ### realloc(%'lld -> %'lld) = %p -> %p   (current %'lld)\n",
                   (long long)oldsize, (long long)size, ptr, newptr, curr);
    }

    ((size_t*)newptr)[0] = size;

    return (char*)newptr + 2*sizeof(size_t);
}

static __attribute__((constructor)) void init(void)
{
    char *error;

    setlocale(LC_NUMERIC, ""); /* for better readable numbers */

    dlerror();

    real_malloc = (malloc_type)dlsym(RTLD_NEXT, "malloc");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "malloc_count ### error %s\n", error);
        exit(EXIT_FAILURE);
    }

    real_realloc = (realloc_type)dlsym(RTLD_NEXT, "realloc");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "malloc_count ### error %s\n", error);
        exit(EXIT_FAILURE);
    }

    real_free = (free_type)dlsym(RTLD_NEXT, "free");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "malloc_count ### error %s\n", error);
        exit(EXIT_FAILURE);
    }
    
    real_posix_memalign = (posix_memalign_type)dlsym(RTLD_NEXT, "posix_memalign");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "malloc_count ### error %s\n", error);
        exit(EXIT_FAILURE);
    }
}

static __attribute__((destructor)) void finish(void)
{
    fprintf(stdout,"malloc_count ### mallocs/peak: %'lld %'lld   frees: %'lld    total: %'lld   peak: %'lld   current: %'lld\n",
            sequential.mallocs+parallel.mallocs+transactional.mallocs, mallocs_peak, 
            sequential.frees+parallel.frees+transactional.frees, total, peak, curr);

    print_bin();
}


/**
 * Obsolete functions not supported
 *
 */
extern void *memalign(size_t alignment, size_t size)
{
    fprintf(stderr, "Obsolete memalign not supported");
    exit(EXIT_FAILURE);
}

extern void *pvalloc(size_t size)
{
    fprintf(stderr, "Obsolete pvalloc not supported");
    exit(EXIT_FAILURE);
}

/*****************************************************************************/
