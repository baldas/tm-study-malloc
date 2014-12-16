/* =============================================================================
 *
 * thread.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
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

#ifdef THREAD_AFFINITY
# define _GNU_SOURCE
# include <unistd.h>
# include <sched.h>
# include <errno.h>
#endif /* THREAD_AFFINITY */

#include <assert.h>
#include <stdlib.h>
#include "tm.h"
#include "thread.h"
#include "types.h"
#ifdef USE_PAPI
#include <stdio.h>
#include <string.h>   // memset
#include <papi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static THREAD_LOCAL_T    global_threadId;
static long              global_numThread       = 1;
static THREAD_BARRIER_T* global_barrierPtr      = NULL;
static long*             global_threadIds       = NULL;
static THREAD_ATTR_T     global_threadAttr;
static THREAD_T*         global_threads         = NULL;
static void            (*global_funcPtr)(void*) = NULL;
static void*             global_argPtr          = NULL;
static volatile bool_t   global_doShutdown      = FALSE;

#ifdef USE_PAPI
static int EventSet = PAPI_NULL;
static long long temp_values[4] = {0,0,0,0};
static long long final_values[4] = {0,0,0,0};
#endif


#ifdef THREAD_AFFINITY

/*
 * Base code took from 
 * http://stackoverflow.com/questions/1407786/how-to-set-cpu-affinity-of-a-particular-pthread 
 */
int stick_this_thread_to_core(int core_id) {
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (core_id < 0 || core_id >= num_cores)
        return EINVAL;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();    
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

int thread_to_core_map[] = {0,4,1,5,2,6,3,7};

#endif /* THREAD_AFFINITY */

/* =============================================================================
 * threadWait
 * -- Synchronizes all threads to start/stop parallel section
 * =============================================================================
 */
static void
threadWait (void* argPtr)
{
    long threadId = *(long*)argPtr;

#ifdef THREAD_AFFINITY
    if (stick_this_thread_to_core(thread_to_core_map[threadId])) {
        perror("Failed to set thread affinity.\n");
        exit(-1);
    }
#endif /* THREAD_AFFINITY */

    THREAD_LOCAL_SET(global_threadId, (long)threadId);

    while (1) {
        THREAD_BARRIER(global_barrierPtr, threadId); /* wait for start parallel */
        if (global_doShutdown) {
            break;
        }
        global_funcPtr(global_argPtr);
        THREAD_BARRIER(global_barrierPtr, threadId); /* wait for end parallel */
        if (threadId == 0) {
            break;
        }
    }
}


/* =============================================================================
 * thread_startup
 * -- Create pool of secondary threads
 * -- numThread is total number of threads (primary + secondaries)
 * =============================================================================
 */
void
thread_startup (long numThread)
{
    long i;

    global_numThread = numThread;
    global_doShutdown = FALSE;

    /* Set up barrier */
    assert(global_barrierPtr == NULL);
    global_barrierPtr = THREAD_BARRIER_ALLOC(numThread);
    assert(global_barrierPtr);
    THREAD_BARRIER_INIT(global_barrierPtr, numThread);

    /* Set up ids */
    THREAD_LOCAL_INIT(global_threadId);
    assert(global_threadIds == NULL);
    global_threadIds = (long*)SEQ_MALLOC(numThread * sizeof(long));
    assert(global_threadIds);
    for (i = 0; i < numThread; i++) {
        global_threadIds[i] = i;
    }

    /* Set up thread list */
    assert(global_threads == NULL);
    global_threads = (THREAD_T*)SEQ_MALLOC(numThread * sizeof(THREAD_T));
    assert(global_threads);


#ifdef USE_PAPI
/*
 * Important notice:
 * We NEED to create the PAPI events BEFORE spawning the threads!!!
 */
  fprintf(stdout, "PAPI version %d.%d\n", PAPI_VERSION_MAJOR(PAPI_VER_CURRENT), PAPI_VERSION_MINOR(PAPI_VER_CURRENT));

  int retval;
  
  PAPI_option_t opt;
  if ( ( retval = PAPI_library_init( PAPI_VER_CURRENT ) ) != PAPI_VER_CURRENT ) {
    perror("failed inheritance support\n");
    exit(-1);
  }

  if ( ( retval = PAPI_create_eventset( &EventSet ) ) != PAPI_OK ) {
    perror("failed inheritance support\n");
    exit(-1);
  }

  if ( ( retval = PAPI_assign_eventset_component( EventSet, 0 ) ) != PAPI_OK ) {
    perror("failed inheritance support\n");
    exit(-1);
  }

  memset( &opt, 0x0, sizeof ( PAPI_option_t ) );
  opt.inherit.inherit = PAPI_INHERIT_ALL;
  opt.inherit.eventset = EventSet;
  
  if ( ( retval = PAPI_set_opt( PAPI_INHERIT, &opt ) ) != PAPI_OK ) {
    if ( retval == PAPI_ECMP) {
       perror("Inherit not supported by current component.\n");
       exit(-1);
    } else {
       perror("PAPI_set_opt");
       exit(-1);
    }

  }

  unsigned int native1, native2, native3, native4;
  PAPI_event_name_to_code("LLC_REFERENCES", &native1);
  PAPI_event_name_to_code("LLC_MISSES", &native2);
  PAPI_event_name_to_code("INSTRUCTION_RETIRED", &native3);
  PAPI_event_name_to_code("UNHALTED_REFERENCE_CYCLES", &native4);
  
  if ( ( retval = PAPI_add_event( EventSet, native1 ) ) != PAPI_OK ) {
    perror("PAPI_add_event");
    exit(-1);
  }
  
  if ( ( retval = PAPI_add_event( EventSet, native2 ) ) != PAPI_OK ) {
    perror("PAPI_add_event");
    exit(-1);
  }
  
  if ( ( retval = PAPI_add_event( EventSet, native3 ) ) != PAPI_OK ) {
    perror("PAPI_add_event");
    exit(-1);
  }
  
  if ( ( retval = PAPI_add_event( EventSet, native4 ) ) != PAPI_OK ) {
    perror("PAPI_add_event");
    exit(-1);
  }
#endif

    /* Set up pool */
    THREAD_ATTR_INIT(global_threadAttr);
    for (i = 1; i < numThread; i++) {
        THREAD_CREATE(global_threads[i],
                      global_threadAttr,
                      &threadWait,
                      &global_threadIds[i]);
    }

    /*
     * Wait for primary thread to call thread_start
     */
}


/* =============================================================================
 * thread_start
 * -- Make primary and secondary threads execute work
 * -- Should only be called by primary thread
 * -- funcPtr takes one arguments: argPtr
 * =============================================================================
 */
void
thread_start (void (*funcPtr)(void*), void* argPtr)
{
    global_funcPtr = funcPtr;
    global_argPtr = argPtr;

    long threadId = 0; /* primary */
#ifdef USE_PAPI
  if ( PAPI_start( EventSet )  != PAPI_OK ) {
    perror("Failed to start PAPI measurement\n");
    exit(-1);
  }
#endif
    threadWait((void*)&threadId);
#ifdef USE_PAPI
  if ( ( PAPI_stop( EventSet, temp_values ) ) != PAPI_OK ) {
    perror("Error Accum'ing PAPI measurement\n");
    exit(1);
  }

  final_values[0] += temp_values[0];
  final_values[1] += temp_values[1];
  final_values[2] += temp_values[2];
  final_values[3] += temp_values[3];
#endif
}


/* =============================================================================
 * thread_shutdown
 * -- Primary thread kills pool of secondary threads
 * =============================================================================
 */
void
thread_shutdown ()
{
    /* Make secondary threads exit wait() */
    global_doShutdown = TRUE;
    THREAD_BARRIER(global_barrierPtr, 0);

    long numThread = global_numThread;

    long i;
    for (i = 1; i < numThread; i++) {
        THREAD_JOIN(global_threads[i]);
    }

    THREAD_BARRIER_FREE(global_barrierPtr);
    global_barrierPtr = NULL;

    SEQ_FREE(global_threadIds);
    global_threadIds = NULL;

    SEQ_FREE(global_threads);
    global_threads = NULL;

    global_numThread = 1;

#ifdef USE_PAPI
  fprintf(stdout, 
    "#accesses/misses: %llu %llu ( %.2f%% )\n", 
    final_values[0], final_values[1], ((double)final_values[1]/(double)final_values[0])*100.0);
  fprintf(stdout, "TOTINST: %llu\n", final_values[2]);
  fprintf(stdout, "UREFCYC: %llu\n", final_values[3]);
#endif
}


/* =============================================================================
 * thread_barrier_alloc
 * =============================================================================
 */
thread_barrier_t*
thread_barrier_alloc (long numThread)
{
    thread_barrier_t* barrierPtr;

    assert(numThread > 0);
    assert((numThread & (numThread - 1)) == 0); /* must be power of 2 */
    barrierPtr = (thread_barrier_t*)SEQ_MALLOC(numThread * sizeof(thread_barrier_t));
    if (barrierPtr != NULL) {
        barrierPtr->numThread = numThread;
    }

    return barrierPtr;
}


/* =============================================================================
 * thread_barrier_free
 * =============================================================================
 */
void
thread_barrier_free (thread_barrier_t* barrierPtr)
{
    SEQ_FREE(barrierPtr);
}


/* =============================================================================
 * thread_barrier_init
 * =============================================================================
 */
void
thread_barrier_init (thread_barrier_t* barrierPtr)
{
    long i;
    long numThread = barrierPtr->numThread;

    for (i = 0; i < numThread; i++) {
        barrierPtr[i].count = 0;
        THREAD_MUTEX_INIT(barrierPtr[i].countLock);
        THREAD_COND_INIT(barrierPtr[i].proceedCond);
        THREAD_COND_INIT(barrierPtr[i].proceedAllCond);
    }
}


/* =============================================================================
 * thread_barrier
 * -- Simple logarithmic barrier
 * =============================================================================
 */
void
thread_barrier (thread_barrier_t* barrierPtr, long threadId)
{
    long i = 2;
    long base = 0;
    long index;
    long numThread = barrierPtr->numThread;

    if (numThread < 2) {
        return;
    }

    do {
        index = base + threadId / i;
        if ((threadId % i) == 0) {
            THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
            barrierPtr[index].count++;
            while (barrierPtr[index].count < 2) {
                THREAD_COND_WAIT(barrierPtr[index].proceedCond,
                                 barrierPtr[index].countLock);
            }
            THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
        } else {
            THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
            barrierPtr[index].count++;
            if (barrierPtr[index].count == 2) {
                THREAD_COND_SIGNAL(barrierPtr[index].proceedCond);
            }
            while (THREAD_COND_WAIT(barrierPtr[index].proceedAllCond,
                                    barrierPtr[index].countLock) != 0)
            {
                /* wait */
            }
            THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
            break;
        }
        base = base + numThread / i;
        i *= 2;
    } while (i <= numThread);

    for (i /= 2; i > 1; i /= 2) {
        base = base - numThread / i;
        index = base + threadId / i;
        THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
        barrierPtr[index].count = 0;
        THREAD_COND_SIGNAL(barrierPtr[index].proceedAllCond);
        THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
    }
}


/* =============================================================================
 * thread_getId
 * -- Call after thread_start() to get thread ID inside parallel region
 * =============================================================================
 */
long
thread_getId()
{
    return (long)THREAD_LOCAL_GET(global_threadId);
}


/* =============================================================================
 * thread_getNumThread
 * -- Call after thread_start() to get number of threads inside parallel region
 * =============================================================================
 */
long
thread_getNumThread()
{
    return global_numThread;
}


/* =============================================================================
 * thread_barrier_wait
 * -- Call after thread_start() to synchronize threads inside parallel region
 * =============================================================================
 */
void
thread_barrier_wait()
{
#ifndef SIMULATOR
    long threadId = thread_getId();
#endif /* !SIMULATOR */
    THREAD_BARRIER(global_barrierPtr, threadId);
}


/* =============================================================================
 * TEST_THREAD
 * =============================================================================
 */
#ifdef TEST_THREAD


#include <stdio.h>
#include <unistd.h>


#define NUM_THREADS    (4)
#define NUM_ITERATIONS (3)



void
printId (void* argPtr)
{
    long threadId = thread_getId();
    long numThread = thread_getNumThread();
    long i;

    for ( i = 0; i < NUM_ITERATIONS; i++ ) {
        thread_barrier_wait();
        if (threadId == 0) {
            sleep(1);
        } else if (threadId == numThread-1) {
            usleep(100);
        }
        printf("i = %li, tid = %li\n", i, threadId);
        if (threadId == 0) {
            puts("");
        }
        fflush(stdout);
    }
}


int
main ()
{
    puts("Starting...");

    /* Run in parallel */
    thread_startup(NUM_THREADS);
    /* Start timing here */
    thread_start(printId, NULL);
    thread_start(printId, NULL);
    thread_start(printId, NULL);
    /* Stop timing here */
    thread_shutdown();

    puts("Done.");

    return 0;
}


#endif /* TEST_THREAD */

#ifdef __cplusplus
}
#endif

/* =============================================================================
 *
 * End of thread.c
 *
 * =============================================================================
 */
