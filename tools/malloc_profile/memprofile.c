/******************************************************************************
 * memprofile.h
 *
 * Class to write the datafile for a memory profile plot using malloc_count.
 *
 ******************************************************************************
 * Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/


#include <stdio.h>
#include <sys/time.h>

#include "malloc_profile.h"

/**
 * MemProfile is a class which hooks into malloc_count's callback and writes a
 * heap usage profile at run-time.
 *
 * A usual application will have many heap allocations and deallocations,
 * therefore these must be aggregated to create a useful plot. This is the main
 * purposes of MemProfile. However, the "resolution" of discrete aggregation
 * intervals must be configured manually, as they highly depend on the profiled
 * application.
 */
    
/// output time resolution
static double      m_time_resolution = 0.1;
    
/// output memory resolution
static size_t      m_size_resolution = 1024;

/// function marker for multi-output
static const char* m_funcname = NULL;
    
/// output file
static FILE*       m_file;

/// start of current memprofile
static double      m_base_ts;
    
/// start memory usage of current memprofile
static size_t      m_base_mem;
    
/// start stack pointer of memprofile
static char*       m_stack_base;

/// timestamp of previous log output
static double      m_prev_ts;
    
/// memory usage of previous log output
static size_t      m_prev_mem;
    
/// maximum memory usage to previous log output
size_t             m_max;


static inline size_t absdiff(size_t a, size_t b)
{
   return (a < b) ? (b - a) : (a - b);
}

/// time is measured using gettimeofday() or omp_get_wtime()
static inline double timestamp()
{
#ifdef _OPENMP
   return omp_get_wtime();
#else
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec + tv.tv_usec / 1e6;
#endif
}

/// output a data pair (ts,mem) to log file
inline void output(double ts, unsigned long long mem, 
	 	   unsigned long long mallocs,
		   unsigned long long frees,
		   unsigned long long mallocs_tx,
		   unsigned long long frees_tx)
{
   if (m_funcname) { // more verbose output format
      fprintf(m_file, "func=%s ts=%g mem=%llu\n",
      m_funcname, ts - m_base_ts, mem);
   }
   else { // simple gnuplot output
      fprintf(m_file, "%g %llu %llu %llu %llu %llu\n",
              ts - m_base_ts, mem, mallocs, frees, mallocs_tx, frees_tx);
   }
}

/// callback invoked by malloc_count when heap usage changes.
inline void callback(size_t memcurr, size_t mallocs, size_t frees, size_t mallocs_tx, size_t frees_tx)
{
   size_t mem = (memcurr > m_base_mem) ? (memcurr - m_base_mem) : 0;

   if ((char*)&mem < m_stack_base) // add stack usage
      mem += m_stack_base - (char*)&mem;

   double ts = timestamp();
   if (m_max < mem) m_max = mem; // keep max usage to last output

   // check to output a pair
   if (ts - m_prev_ts > m_time_resolution ||
       absdiff(mem, m_prev_mem) > m_size_resolution )
   {
      output(ts, m_max, mallocs, frees, mallocs_tx, frees_tx);
      m_max = 0;
      m_prev_ts = ts;
      m_prev_mem = mem;
   }
}

void memprofile_insertnull()
{
   static int counter = 0;

   fprintf(stdout, "\nBeginRegion %d: %g\n", counter++, timestamp()-m_base_ts);
}

/// static callback for malloc_count, forwards to class method.
static void static_callback(void* cookie, size_t memcurr, size_t mallocs, size_t frees, size_t mallocs_tx, size_t frees_tx)
{
   callback(memcurr, mallocs, frees, mallocs_tx, frees_tx);
}


void memprofile_start(const char* filepath, double time_resolution, 
		size_t size_resolution, const char* funcname)
{
    m_time_resolution = time_resolution;
    m_size_resolution = size_resolution;
    m_funcname = funcname;
    m_base_ts = timestamp();
    m_base_mem = malloc_count_current();
    m_prev_ts = 0;
    m_prev_mem = 0;
    m_max = 0;

    char stack;
    m_stack_base = &stack;
    m_file = fopen(filepath, funcname ? "a" : "w");
    malloc_count_set_callback(static_callback, (void *)NULL);
}

void memprofile_end() 
{
   m_prev_ts = 0; // force flush
   m_prev_mem = 0;
   callback( malloc_count_current(), 0, 0, 0, 0 );
   malloc_count_set_callback(NULL, NULL);
   fclose(m_file);
}

