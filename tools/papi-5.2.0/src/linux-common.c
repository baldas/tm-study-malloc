/*
* File:    linux-common.c
*/

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <syscall.h>
#include <sys/utsname.h>
#include <sys/time.h>

#include "papi.h"
#include "papi_internal.h"
#include "papi_vector.h"

#include "linux-memory.h"
#include "linux-common.h"
#include "linux-timer.h"

#include "x86_cpuid_info.h"

PAPI_os_info_t _papi_os_info;

/* The locks used by Linux */

#if defined(USE_PTHREAD_MUTEXES)
pthread_mutex_t _papi_hwd_lock_data[PAPI_MAX_LOCK];
#else
volatile unsigned int _papi_hwd_lock_data[PAPI_MAX_LOCK];
#endif


static int _linux_init_locks(void) {

   int i;

   for ( i = 0; i < PAPI_MAX_LOCK; i++ ) {
#if defined(USE_PTHREAD_MUTEXES)
       pthread_mutex_init(&_papi_hwd_lock_data[i],NULL);
#else
       _papi_hwd_lock_data[i] = MUTEX_OPEN;
#endif
   }

   return PAPI_OK;
}

	
int
_linux_detect_hypervisor(char *virtual_vendor_name) {

	int retval=0;

#if defined(__i386__)||defined(__x86_64__)
	retval=_x86_detect_hypervisor(virtual_vendor_name);
#else
	(void) virtual_vendor_name;
#endif
	
	return retval;
}


#define _PATH_SYS_SYSTEM "/sys/devices/system"
#define _PATH_SYS_CPU0	 _PATH_SYS_SYSTEM "/cpu/cpu0"

static char pathbuf[PATH_MAX] = "/";


static char *
search_cpu_info( FILE * f, char *search_str, char *line )
{
	/* This function courtesy of Rudolph Berrendorf! */
	/* See the home page for the German version of PAPI. */
	char *s;

	while ( fgets( line, 256, f ) != NULL ) {
		if ( strstr( line, search_str ) != NULL ) {
			/* ignore all characters in line up to : */
			for ( s = line; *s && ( *s != ':' ); ++s );
			if ( *s )
				return s;
		}
	}
	return NULL;
}

static void
decode_vendor_string( char *s, int *vendor )
{
	if ( strcasecmp( s, "GenuineIntel" ) == 0 )
		*vendor = PAPI_VENDOR_INTEL;
	else if ( ( strcasecmp( s, "AMD" ) == 0 ) ||
			  ( strcasecmp( s, "AuthenticAMD" ) == 0 ) )
		*vendor = PAPI_VENDOR_AMD;
	else if ( strcasecmp( s, "IBM" ) == 0 )
		*vendor = PAPI_VENDOR_IBM;
	else if ( strcasecmp( s, "Cray" ) == 0 )
		*vendor = PAPI_VENDOR_CRAY;
	else if ( strcasecmp( s, "ARM" ) == 0 )
		*vendor = PAPI_VENDOR_ARM;
	else if ( strcasecmp( s, "MIPS" ) == 0 )
		*vendor = PAPI_VENDOR_MIPS;
	else if ( strcasecmp( s, "SiCortex" ) == 0 )
		*vendor = PAPI_VENDOR_MIPS;
	else
		*vendor = PAPI_VENDOR_UNKNOWN;
}

static FILE *
xfopen( const char *path, const char *mode )
{
	FILE *fd = fopen( path, mode );
	if ( !fd )
		err( EXIT_FAILURE, "error: %s", path );
	return fd;
}

static FILE *
path_vfopen( const char *mode, const char *path, va_list ap )
{
	vsnprintf( pathbuf, sizeof ( pathbuf ), path, ap );
	return xfopen( pathbuf, mode );
}


static int
path_sibling( const char *path, ... )
{
	int c;
	long n;
	int result = 0;
	char s[2];
	FILE *fp;
	va_list ap;
	va_start( ap, path );
	fp = path_vfopen( "r", path, ap );
	va_end( ap );

	while ( ( c = fgetc( fp ) ) != EOF ) {
		if ( isxdigit( c ) ) {
			s[0] = ( char ) c;
			s[1] = '\0';
			for ( n = strtol( s, NULL, 16 ); n > 0; n /= 2 ) {
				if ( n % 2 )
					result++;
			}
		}
	}

	fclose( fp );
	return result;
}

static int
path_exist( const char *path, ... )
{
	va_list ap;
	va_start( ap, path );
	vsnprintf( pathbuf, sizeof ( pathbuf ), path, ap );
	va_end( ap );
	return access( pathbuf, F_OK ) == 0;
}

int
_linux_get_cpu_info( PAPI_hw_info_t *hwinfo, int *cpuinfo_mhz )
{
    int tmp, retval = PAPI_OK;
    char maxargs[PAPI_HUGE_STR_LEN], *t, *s;
    float mhz = 0.0;
    FILE *f;

    if ( ( f = fopen( "/proc/cpuinfo", "r" ) ) == NULL ) {
       PAPIERROR( "fopen(/proc/cpuinfo) errno %d", errno );
       return PAPI_ESYS;
    }

	/* All of this information maybe overwritten by the component */

        /* MHZ */
    rewind( f );
    s = search_cpu_info( f, "clock", maxargs );
    if ( !s ) {
       rewind( f );
       s = search_cpu_info( f, "cpu MHz", maxargs );
    }
    if ( s ) {
       sscanf( s + 1, "%f", &mhz );
    }
    *cpuinfo_mhz = mhz;

       /* Vendor Name and Vendor Code */
    rewind( f );
    s = search_cpu_info( f, "vendor_id", maxargs );
    if ( s && ( t = strchr( s + 2, '\n' ) ) ) {
       *t = '\0';
       strcpy( hwinfo->vendor_string, s + 2 );
    } else {
       rewind( f );
       s = search_cpu_info( f, "vendor", maxargs );
       if ( s && ( t = strchr( s + 2, '\n' ) ) ) {
	  *t = '\0';
	  strcpy( hwinfo->vendor_string, s + 2 );
       } else {
	  rewind( f );
	  s = search_cpu_info( f, "system type", maxargs );
	  if ( s && ( t = strchr( s + 2, '\n' ) ) ) {
	     *t = '\0';
	     s = strtok( s + 2, " " );
	     strcpy( hwinfo->vendor_string, s );
	  } else {
	     rewind( f );
	     s = search_cpu_info( f, "platform", maxargs );
	     if ( s && ( t = strchr( s + 2, '\n' ) ) ) {
		*t = '\0';
	        s = strtok( s + 2, " " );
		if ( ( strcasecmp( s, "pSeries" ) == 0 ) ||
		     ( strcasecmp( s, "PowerMac" ) == 0 ) ) {
		   strcpy( hwinfo->vendor_string, "IBM" );
		}
	     } else {
		rewind( f );
		s = search_cpu_info( f, "CPU implementer", maxargs );
		if ( s ) {
		   strcpy( hwinfo->vendor_string, "ARM" );
		}
	     }
	  }
       }
    }

    if ( strlen( hwinfo->vendor_string ) ) {
       decode_vendor_string( hwinfo->vendor_string, &hwinfo->vendor );
    }

	/* Revision */
    rewind( f );
    s = search_cpu_info( f, "stepping", maxargs );
    if ( s ) {
       sscanf( s + 1, "%d", &tmp );
       hwinfo->revision = ( float ) tmp;
       hwinfo->cpuid_stepping = tmp;
    } else {
       rewind( f );
       s = search_cpu_info( f, "revision", maxargs );
       if ( s ) {
	  sscanf( s + 1, "%d", &tmp );
	  hwinfo->revision = ( float ) tmp;
	  hwinfo->cpuid_stepping = tmp;
       }
    }

       /* Model Name */
    rewind( f );
    s = search_cpu_info( f, "model name", maxargs );
    if ( s && ( t = strchr( s + 2, '\n' ) ) ) {
       *t = '\0';
       strcpy( hwinfo->model_string, s + 2 );
    } else {
       rewind( f );
       s = search_cpu_info( f, "family", maxargs );
       if ( s && ( t = strchr( s + 2, '\n' ) ) ) {
	  *t = '\0';
	  strcpy( hwinfo->model_string, s + 2 );
       } else {
	  rewind( f );
	  s = search_cpu_info( f, "cpu model", maxargs );
	  if ( s && ( t = strchr( s + 2, '\n' ) ) ) {
	     *t = '\0';
	     strtok( s + 2, " " );
	     s = strtok( NULL, " " );
	     strcpy( hwinfo->model_string, s );
	  } else {
	     rewind( f );
	     s = search_cpu_info( f, "cpu", maxargs );
	     if ( s && ( t = strchr( s + 2, '\n' ) ) ) {
		*t = '\0';
		/* get just the first token */
		s = strtok( s + 2, " " );
		strcpy( hwinfo->model_string, s );
	     }
	  }
       }
    }

       /* Family */
    rewind( f );
    s = search_cpu_info( f, "family", maxargs );
    if ( s ) {
       sscanf( s + 1, "%d", &tmp );
       hwinfo->cpuid_family = tmp;
    } else {
       rewind( f );
       s = search_cpu_info( f, "cpu family", maxargs );
       if ( s ) {
	  sscanf( s + 1, "%d", &tmp );
	  hwinfo->cpuid_family = tmp;
	}
    }

	/* CPU Model */
    rewind( f );
    s = search_cpu_info( f, "model", maxargs );
    if ( s ) {
       sscanf( s + 1, "%d", &tmp );
       hwinfo->model = tmp;
       hwinfo->cpuid_model = tmp;
    }


	/* The following members are set using the same methodology */
	/* used in lscpu.                                           */

	/* Total number of CPUs */
	/* The following line assumes totalcpus was initialized to zero! */
	while ( path_exist( _PATH_SYS_SYSTEM "/cpu/cpu%d", hwinfo->totalcpus ) )
		hwinfo->totalcpus++;

	/* Number of threads per core */
	if ( path_exist( _PATH_SYS_CPU0 "/topology/thread_siblings" ) )
		hwinfo->threads =
			path_sibling( _PATH_SYS_CPU0 "/topology/thread_siblings" );

	/* Number of cores per socket */
	if ( path_exist( _PATH_SYS_CPU0 "/topology/core_siblings" ) &&
		 hwinfo->threads > 0 )
		hwinfo->cores =
			path_sibling( _PATH_SYS_CPU0 "/topology/core_siblings" ) /
			hwinfo->threads;

	/* Number of NUMA nodes */
	/* The following line assumes nnodes was initialized to zero! */
	while ( path_exist( _PATH_SYS_SYSTEM "/node/node%d", hwinfo->nnodes ) )
		hwinfo->nnodes++;

	/* Number of CPUs per node */
	hwinfo->ncpu =
		hwinfo->nnodes >
		1 ? hwinfo->totalcpus / hwinfo->nnodes : hwinfo->totalcpus;

	/* Number of sockets */
	if ( hwinfo->threads > 0 && hwinfo->cores > 0 )
		hwinfo->sockets = hwinfo->totalcpus / hwinfo->cores / hwinfo->threads;

#if 0
	int *nodecpu;
	/* cpumap data is not currently part of the _papi_hw_info struct */
        nodecpu = malloc( (unsigned int) hwinfo->nnodes * sizeof(int) );
	if ( nodecpu ) {
	   int i;
	   for ( i = 0; i < hwinfo->nnodes; ++i ) {
	       nodecpu[i] = path_sibling( 
                             _PATH_SYS_SYSTEM "/node/node%d/cpumap", i );
	   }
	} else {
		PAPIERROR( "malloc failed for variable not currently used" );
	}
#endif


	/* Fixup missing Megahertz Value */
	/* This is missing from cpuinfo on ARM and MIPS */
     if (*cpuinfo_mhz < 1.0) {
	rewind( f );

	s = search_cpu_info( f, "BogoMIPS", maxargs );
	if ((!s) || (sscanf( s + 1, "%f", &mhz ) != 1)) {
	   INTDBG("Mhz detection failed. Please edit file %s at line %d.\n",
		     __FILE__,__LINE__);
	}

	if (hwinfo->vendor == PAPI_VENDOR_MIPS) {
	    /* MIPS has 2x clock multiplier */
	    *cpuinfo_mhz = 2*(((int)mhz)+1);

	    /* Also update version info on MIPS */
	    rewind( f );
	    s = search_cpu_info( f, "cpu model", maxargs );
	    s = strstr(s+1," V")+2;
	     strtok(s," ");
	    sscanf(s, "%f ", &hwinfo->revision );
	}
	else {
	    /* In general bogomips is proportional to number of CPUs */
	    if (hwinfo->totalcpus) {
	       if (mhz!=0) *cpuinfo_mhz = mhz / hwinfo->totalcpus;
	    }
	}
     }

    fclose( f );

    return retval;
}

int
_linux_get_mhz( int *sys_min_mhz, int *sys_max_mhz ) {

  FILE *fff;
  int result;

  /* Try checking for min MHz */
  /* Assume cpu0 exists */
  fff=fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq","r");
  if (fff==NULL) return PAPI_EINVAL;
  result=fscanf(fff,"%d",sys_min_mhz);
  fclose(fff);
  if (result!=1) return PAPI_EINVAL;

  fff=fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq","r");
  if (fff==NULL) return PAPI_EINVAL;
  result=fscanf(fff,"%d",sys_max_mhz);
  fclose(fff);
  if (result!=1) return PAPI_EINVAL;

  return PAPI_OK;

}

int
_linux_get_system_info( papi_mdi_t *mdi ) {

	int retval;

	char maxargs[PAPI_HUGE_STR_LEN];
	pid_t pid;

	int cpuinfo_mhz,sys_min_khz,sys_max_khz;

	/* Software info */

	/* Path and args */

	pid = getpid(  );
	if ( pid < 0 ) {
		PAPIERROR( "getpid() returned < 0" );
		return PAPI_ESYS;
	}
	mdi->pid = pid;

	sprintf( maxargs, "/proc/%d/exe", ( int ) pid );
	if ( readlink( maxargs, mdi->exe_info.fullname, PAPI_HUGE_STR_LEN ) < 0 ) {
		PAPIERROR( "readlink(%s) returned < 0", maxargs );
		return PAPI_ESYS;
	}

	/* Careful, basename can modify it's argument */

	strcpy( maxargs, mdi->exe_info.fullname );
	strcpy( mdi->exe_info.address_info.name, basename( maxargs ) );

	SUBDBG( "Executable is %s\n", mdi->exe_info.address_info.name );
	SUBDBG( "Full Executable is %s\n", mdi->exe_info.fullname );

	/* Executable regions, may require reading /proc/pid/maps file */

	retval = _linux_update_shlib_info( mdi );
	SUBDBG( "Text: Start %p, End %p, length %d\n",
			mdi->exe_info.address_info.text_start,
			mdi->exe_info.address_info.text_end,
			( int ) ( mdi->exe_info.address_info.text_end -
					  mdi->exe_info.address_info.text_start ) );
	SUBDBG( "Data: Start %p, End %p, length %d\n",
			mdi->exe_info.address_info.data_start,
			mdi->exe_info.address_info.data_end,
			( int ) ( mdi->exe_info.address_info.data_end -
					  mdi->exe_info.address_info.data_start ) );
	SUBDBG( "Bss: Start %p, End %p, length %d\n",
			mdi->exe_info.address_info.bss_start,
			mdi->exe_info.address_info.bss_end,
			( int ) ( mdi->exe_info.address_info.bss_end -
					  mdi->exe_info.address_info.bss_start ) );

	/* PAPI_preload_option information */

	strcpy( mdi->preload_info.lib_preload_env, "LD_PRELOAD" );
	mdi->preload_info.lib_preload_sep = ' ';
	strcpy( mdi->preload_info.lib_dir_env, "LD_LIBRARY_PATH" );
	mdi->preload_info.lib_dir_sep = ':';

	/* Hardware info */

	retval = _linux_get_cpu_info( &mdi->hw_info, &cpuinfo_mhz );
	if ( retval )
		return retval;

	/* Handle MHz */

	retval = _linux_get_mhz( &sys_min_khz, &sys_max_khz );
	if ( retval ) {

	   mdi->hw_info.cpu_max_mhz=cpuinfo_mhz;
	   mdi->hw_info.cpu_min_mhz=cpuinfo_mhz;

	   /*
	   mdi->hw_info.mhz=cpuinfo_mhz;
	   mdi->hw_info.clock_mhz=cpuinfo_mhz;
	   */
	}
	else {
	   mdi->hw_info.cpu_max_mhz=sys_max_khz/1000;
	   mdi->hw_info.cpu_min_mhz=sys_min_khz/1000;

	   /*
	   mdi->hw_info.mhz=sys_max_khz/1000;
	   mdi->hw_info.clock_mhz=sys_max_khz/1000;
	   */
	}

	/* Set Up Memory */

	retval = _linux_get_memory_info( &mdi->hw_info, mdi->hw_info.model );
	if ( retval )
		return retval;

	SUBDBG( "Found %d %s(%d) %s(%d) CPUs at %d Mhz.\n",
			mdi->hw_info.totalcpus,
			mdi->hw_info.vendor_string,
			mdi->hw_info.vendor, 
		        mdi->hw_info.model_string, 
		        mdi->hw_info.model,
		        mdi->hw_info.cpu_max_mhz);

	/* Get virtualization info */
	mdi->hw_info.virtualized=_linux_detect_hypervisor(mdi->hw_info.virtual_vendor_string);

	return PAPI_OK;
}

int 
_papi_hwi_init_os(void) {

    int major=0,minor=0,sub=0;
    char *ptr;
    struct utsname uname_buffer;

    /* Initialize the locks */
    _linux_init_locks();

    /* Get the kernel info */
    uname(&uname_buffer);

    SUBDBG("Native kernel version %s\n",uname_buffer.release);

    strncpy(_papi_os_info.name,uname_buffer.sysname,PAPI_MAX_STR_LEN);

#ifdef ASSUME_KERNEL
    strncpy(_papi_os_info.version,ASSUME_KERNEL,PAPI_MAX_STR_LEN);
    SUBDBG("Assuming kernel version %s\n",_papi_os_info.name);
#else
    strncpy(_papi_os_info.version,uname_buffer.release,PAPI_MAX_STR_LEN);
#endif

    ptr=strtok(_papi_os_info.version,".");
    if (ptr!=NULL) major=atoi(ptr);

    ptr=strtok(NULL,".");
    if (ptr!=NULL) minor=atoi(ptr);

    ptr=strtok(NULL,".");
    if (ptr!=NULL) sub=atoi(ptr);

   _papi_os_info.os_version=LINUX_VERSION(major,minor,sub);

   _papi_os_info.itimer_sig = PAPI_INT_MPX_SIGNAL;
   _papi_os_info.itimer_num = PAPI_INT_ITIMER;
   _papi_os_info.itimer_ns = PAPI_INT_MPX_DEF_US * 1000;
   _papi_os_info.itimer_res_ns = 1;
   _papi_os_info.clock_ticks = sysconf( _SC_CLK_TCK );

   /* Get Linux-specific system info */
   _linux_get_system_info( &_papi_hwi_system_info );

   return PAPI_OK;
}



int _linux_detect_nmi_watchdog() {

  int watchdog_detected=0,watchdog_value=0;
  FILE *fff;

  fff=fopen("/proc/sys/kernel/nmi_watchdog","r");
  if (fff!=NULL) {
     if (fscanf(fff,"%d",&watchdog_value)==1) {
        if (watchdog_value>0) watchdog_detected=1;
     }
     fclose(fff);
  }

  return watchdog_detected;
}

papi_os_vector_t _papi_os_vector = {
  .get_memory_info =   _linux_get_memory_info,
  .get_dmem_info =     _linux_get_dmem_info,
  .get_real_cycles =   _linux_get_real_cycles,
  .update_shlib_info = _linux_update_shlib_info,
  .get_system_info =   _linux_get_system_info,


#if defined(HAVE_CLOCK_GETTIME)
  .get_real_usec =  _linux_get_real_usec_gettime,
#elif defined(HAVE_GETTIMEOFDAY)
  .get_real_usec =  _linux_get_real_usec_gettimeofday,
#else
  .get_real_usec =  _linux_get_real_usec_cycles,
#endif


#if defined(USE_PROC_PTTIMER)
  .get_virt_usec =   _linux_get_virt_usec_pttimer,
#elif defined(HAVE_CLOCK_GETTIME_THREAD)
  .get_virt_usec =   _linux_get_virt_usec_gettime,
#elif defined(HAVE_PER_THREAD_TIMES)
  .get_virt_usec =   _linux_get_virt_usec_times,
#elif defined(HAVE_PER_THREAD_GETRUSAGE)
  .get_virt_usec =   _linux_get_virt_usec_rusage,
#endif


#if defined(HAVE_CLOCK_GETTIME)
  .get_real_nsec =  _linux_get_real_nsec_gettime,
#endif

#if defined(HAVE_CLOCK_GETTIME_THREAD)
  .get_virt_nsec =   _linux_get_virt_nsec_gettime,
#endif


};
