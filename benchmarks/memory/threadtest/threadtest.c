///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2000, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////


/**
 * @file threadtest.cpp
 *
 * This program does nothing but generate a number of kernel threads
 * that allocate and free memory, with a variable
 * amount of "work" (i.e. cycle wasting) in between.
*/

#ifndef _REENTRANT
#define _REENTRANT
#endif


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


//#include "fred.h"


#if defined(__cplusplus)
extern "C" {
#endif
extern void * hoardmalloc(size_t);
extern void hoardfree (void*);
extern void * hoardcalloc(size_t, size_t);
extern void * hoardrealloc(void *,size_t);
#if defined(__cplusplus)
}
#endif

#if defined(USE_HOARD)
#define malloc(x) hoardmalloc(x)
#define free(p) hoardfree(p)
#define calloc(s,n) hoardcalloc(s,n)
#define realloc(p,s) hoardrealloc(p,s)
#endif


int niterations = 50;	// Default number of iterations.
int nobjects = 30000;  // Default number of objects.
int nthreads = 1;	// Default number of threads.
int work = 0;		// Default number of loop iterations.
int size = 1;

typedef struct Foo_t {
  int x;
  int y;
} Foo;


void *worker (void *arg)
{
  int i, j;
  Foo *var;
  Foo ** a = &var;  

  /*
   * We are forcing only one object per thread in order to avoid using 
   * 'malloc' too early for metadata (it might interfere with the experiment).
   * We could allocate a small amount of automatic memory for that ...
   */
  assert(nobjects == nthreads);

  /*
  a = malloc(sizeof(Foo *) * nobjects / nthreads);
  if (a == NULL) {
    perror("malloc error");
    exit(-1);
  }
  */


  for (j = 0; j < niterations; j++) {

    for (i = 0; i < (nobjects / nthreads); i ++) {
      a[i] = malloc(sizeof(Foo)*size);
      if (a[i] == NULL) {
        perror("malloc error");
        exit(-1);
      }
     
      volatile int d; 
      for (d = 0; d < work; d++) {
	volatile int f = 1;
	f = f + f;
	f = f * f;
	f = f + f;
	f = f * f;
      }
   //   assert (a[i]);
    }
    
    for (i = 0; i < (nobjects / nthreads); i ++) {
      free(a[i]);
      volatile int d; 
      for (d = 0; d < work; d++) {
	volatile int f = 1;
	f = f + f;
	f = f * f;
	f = f + f;
	f = f * f;
      }
    }
  }


  return NULL;
}

#if defined(__sgi)
#include <ulocks.h>
#endif
  
#include <pthread.h>
#include <unistd.h>

#define MAX_THREADS 64
int main (int argc, char * argv[])
{
  pthread_t threads_id[MAX_THREADS];
  pthread_t * threads = threads_id;
  pthread_attr_t attr;
  
  if (argc >= 2) {
    nthreads = atoi(argv[1]);
    assert(nthreads <= MAX_THREADS);
  }

  if (argc >= 3) {
    niterations = atoi(argv[2]);
  }

  if (argc >= 4) {
    nobjects = atoi(argv[3]);
  }

  if (argc >= 5) {
    work = atoi(argv[4]);
  }

  if (argc >= 6) {
    size = atoi(argv[5]);
  }
    
  pthread_attr_init (&attr);
  pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);

  printf ("Running threadtest for %d threads, %d iterations, %d objects, %d work and %d size...\n", nthreads, niterations, nobjects, work, size);

  if (nthreads > MAX_THREADS) {
    printf("Max threads allowed %d\n", MAX_THREADS);
    exit(-1);
  }

  struct timeval start, stop;

  gettimeofday(&start, NULL);

  int i;
  for (i = 0; i < nthreads; i++) {
    pthread_create(&threads[i], &attr, worker, NULL);
  }

  for (i = 0; i < nthreads; i++) {
    pthread_join(threads[i], NULL);
  }

  gettimeofday(&stop, NULL);

  double t = (((double)(stop.tv_sec)  + (double)(stop.tv_usec / 1000000.0)) - \
              ((double)(start.tv_sec) + (double)(start.tv_usec / 1000000.0)));

  printf( "Time elapsed = %f\n", t);
  printf( "Throughput = %f\n", ((double)(niterations*nobjects))/t);

    
  pthread_attr_destroy (&attr);

  return 0;
}
