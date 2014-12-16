/** 
 * @file    linux-lmsensors.c
 * @author  Daniel Lucio
 * @author  Joachim Protze
 * @author  Heike Jagode
 *          jagode@eecs.utk.edu
 *
 * @ingroup papi_components
 *
 *
 * LM_SENSORS component 
 * 
 * Tested version of lm_sensors: 3.1.1
 *
 * @brief 
 *  This file has the source code for a component that enables PAPI-C to access
 *  hardware monitoring sensors through the libsensors library. This code will
 *  dynamically create a native events table for all the sensors that can be 
 *  accesed by the libsensors library.
 *  In order to learn more about libsensors, visit: (http://www.lm-sensors.org) 
 *
 * Notes: 
 *  - I used the ACPI and MX components to write this component. A lot of the
 *    code in this file mimics what other components already do. 
 *  - The return values are scaled by 1000 because PAPI can not return decimals.
 *  - A call of PAPI_read can take up to 2 seconds while using lm_sensors!
 *  - Please remember that libsensors uses the GPL license. 
 */


/* Headers required by libsensors */
#include <sensors.h>
#include <error.h>
#include <time.h>
#include <string.h>

/* Headers required by PAPI */
#include "papi.h"
#include "papi_internal.h"
#include "papi_vector.h"
#include "papi_memory.h"

/*************************  DEFINES SECTION  ***********************************
 *******************************************************************************/
/* this number assumes that there will never be more events than indicated */
#define LM_SENSORS_MAX_COUNTERS 512
// time in usecs
#define LM_SENSORS_REFRESHTIME 200000

/** Structure that stores private information of each event */
typedef struct _lmsensors_register
{
	/* This is used by the framework.It likes it to be !=0 to do somehting */
	unsigned int selector;
	/* These are the only information needed to locate a libsensors event */
	const sensors_chip_name *name;
	int subfeat_nr;
} _lmsensors_register_t;

/*
 * The following structures mimic the ones used by other components. It is more
 * convenient to use them like that as programming with PAPI makes specific
 * assumptions for them.
 */

/** This structure is used to build the table of events */
typedef struct _lmsensors_native_event_entry
{
	_lmsensors_register_t resources;
	char name[PAPI_MAX_STR_LEN];
	char description[PAPI_MAX_STR_LEN];
	unsigned int count;
} _lmsensors_native_event_entry_t;


typedef struct _lmsensors_reg_alloc
{
	_lmsensors_register_t ra_bits;
} _lmsensors_reg_alloc_t;


typedef struct _lmsensors_control_state
{
	long_long counts[LM_SENSORS_MAX_COUNTERS];	// used for caching
	long_long lastupdate;
} _lmsensors_control_state_t;


typedef struct _lmsensors_context
{
	_lmsensors_control_state_t state;
} _lmsensors_context_t;



/*************************  GLOBALS SECTION  ***********************************
 *******************************************************************************/
/* This table contains the LM_SENSORS native events */
static _lmsensors_native_event_entry_t *lm_sensors_native_table;
/* number of events in the table*/
static int num_events = 0;



papi_vector_t _lmsensors_vector;

/******************************************************************************
 ********  BEGIN FUNCTIONS  USED INTERNALLY SPECIFIC TO THIS COMPONENT ********
 *****************************************************************************/
/*
 * Counts number of events available in this system
 */
static unsigned
detectSensors( void )
{
	unsigned id = 0;
	int chip_nr = 0;
	const sensors_chip_name *chip_name;

	/* Loop through all the chips, features, subfeatures found */
	while ( ( chip_name =
			  sensors_get_detected_chips( NULL, &chip_nr ) ) != NULL ) {
		int a = 0, b;
		const sensors_feature *feature;

		while ( ( feature = sensors_get_features( chip_name, &a ) ) ) {
			b = 0;
			while ( ( sensors_get_all_subfeatures( chip_name, feature,
												   &b ) ) ) {
				id++;
			}
		}
	}

	return id;
}


/*
 * Create the native events for particulare component (!= 0)
 */
static unsigned
createNativeEvents( void )
{
	unsigned id = 0;
	unsigned int count = 0;

	int chip_nr = 0;
	const sensors_chip_name *chip_name;

	/* component name and description */
	strcpy( _lmsensors_vector.cmp_info.short_name, "LM_SENSORS" );
	strcpy( _lmsensors_vector.cmp_info.description,
			"lm-sensors provides tools for monitoring the hardware health" );


	/* Loop through all the chips found */
	while ( ( chip_name =
			  sensors_get_detected_chips( NULL, &chip_nr ) ) != NULL ) {
	   int a, b;
	   const sensors_feature *feature;
	   const sensors_subfeature *sub;
	   char chipnamestring[PAPI_MIN_STR_LEN];

	   //	   lm_sensors_native_table[id].count = 0;

		/* get chip name from its internal representation */
	   sensors_snprintf_chip_name( chipnamestring,
					    PAPI_MIN_STR_LEN, chip_name );

	   a = 0;

	   /* Loop through all the features found */
	   while ( ( feature = sensors_get_features( chip_name, &a ) ) ) {
	      char *featurelabel;

	      if ( !( featurelabel = sensors_get_label( chip_name, feature ))) {
		 fprintf( stderr, "ERROR: Can't get label of feature %s!\n",
						 feature->name );
		 continue;
	      }

	      b = 0;

	      /* Loop through all the subfeatures found */
	      while ((sub=sensors_get_all_subfeatures(chip_name,feature,&b))) {

	         count = 0;

		 /* Save native event data */
		 sprintf( lm_sensors_native_table[id].name, "%s.%s.%s.%s",
			  _lmsensors_vector.cmp_info.short_name,
			  chipnamestring, featurelabel, sub->name );

		 strncpy( lm_sensors_native_table[id].description,
			  lm_sensors_native_table[id].name, PAPI_MAX_STR_LEN );

		 /* The selector has to be !=0 . Starts with 1 */
		 lm_sensors_native_table[id].resources.selector = id + 1;

		 /* Save the actual references to this event */
		 lm_sensors_native_table[id].resources.name = chip_name;
		 lm_sensors_native_table[id].resources.subfeat_nr = sub->number;

		 count = sub->number;

		 /* increment the table index counter */
		 id++;		 
	      }

	      //   lm_sensors_native_table[id].count = count + 1;
	      free( featurelabel );
	   }
	}

	/* Return the number of events created */
	return id;
}

/*
 * Returns the value of the event with index 'i' in lm_sensors_native_table
 * This value is scaled by 1000 to cope with the lack to return decimal numbers
 * with PAPI
 */

static long_long
getEventValue( unsigned event_id )
{
	double value;
	int res;

	res = sensors_get_value( lm_sensors_native_table[event_id].resources.name,
							 lm_sensors_native_table[event_id].resources.
							 subfeat_nr, &value );

	if ( res < 0 ) {
		fprintf( stderr, "libsensors(): Could not read event #%d!\n",
				 event_id );
		return -1;
	}

	return ( ( long_long ) ( value * 1000 ) );
}


/*****************************************************************************
 *******************  BEGIN PAPI's COMPONENT REQUIRED FUNCTIONS  *************
 *****************************************************************************/

/*
 * This is called whenever a thread is initialized
 */
int
_lmsensors_init_thread( hwd_context_t *ctx )
{
    ( void ) ctx;
    return PAPI_OK;
}


/* Initialize hardware counters, setup the function vector table
 * and get hardware information, this routine is called when the 
 * PAPI process is initialized (IE PAPI_library_init)
 */
int
_lmsensors_init_component( int cidx )
{
    int res;
    (void) cidx;

    /* Initialize libsensors library */
    if ( ( res = sensors_init( NULL ) ) != 0 ) {
       strncpy(_lmsensors_vector.cmp_info.disabled_reason,
	      "Cannot enable libsensors",PAPI_MAX_STR_LEN);
       return res;
    }

    /* Create dyanmic events table */
    num_events = detectSensors(  );
    SUBDBG("Found %d sensors\n",num_events);

    if ( ( lm_sensors_native_table =
	   calloc( num_events, sizeof ( _lmsensors_native_event_entry_t )))
				   == NULL ) {
       strncpy(_lmsensors_vector.cmp_info.disabled_reason,
	      "Could not malloc room",PAPI_MAX_STR_LEN);
       return PAPI_ENOMEM;
    }

    if ( ( unsigned ) num_events != createNativeEvents(  ) ) {
       strncpy(_lmsensors_vector.cmp_info.disabled_reason,
	      "LM_SENSOR number mismatch",PAPI_MAX_STR_LEN);
       return PAPI_ECMP;
    }

    _lmsensors_vector.cmp_info.num_native_events=num_events;
    _lmsensors_vector.cmp_info.num_cntrs=num_events;

    return PAPI_OK;
}


/*
 * Control of counters (Reading/Writing/Starting/Stopping/Setup)
 * functions
 */
int
_lmsensors_init_control_state( hwd_control_state_t *ctl )
{
	int i;

	for ( i = 0; i < num_events; i++ )
		( ( _lmsensors_control_state_t * ) ctl )->counts[i] =
			getEventValue( i );

	( ( _lmsensors_control_state_t * ) ctl )->lastupdate =
		PAPI_get_real_usec(  );
	return PAPI_OK;
}


/*
 *
 */
int
_lmsensors_start( hwd_context_t *ctx, hwd_control_state_t *ctl )
{
	( void ) ctx;
	( void ) ctl;

	return PAPI_OK;
}


/*
 *
 */
int
_lmsensors_stop( hwd_context_t *ctx, hwd_control_state_t *ctl )
{
    ( void ) ctx;
    ( void ) ctl;

    return PAPI_OK;
}


/*
 *
 */
int
_lmsensors_read( hwd_context_t *ctx, hwd_control_state_t *ctl,
		 long_long ** events, int flags )
{
    ( void ) ctx;
    ( void ) flags;
    long long start = PAPI_get_real_usec(  );
    int i;
 
    _lmsensors_control_state_t *control=(_lmsensors_control_state_t *)ctl;

    if ( start - control->lastupdate > 200000 ) {	// cache refresh
       
       for ( i = 0; i < num_events; i++ ) {
	   control->counts[i] = getEventValue( i );
       }
       control->lastupdate = PAPI_get_real_usec(  );
    }

    *events = control->counts;
    return PAPI_OK;
}


int
_lmsensors_shutdown_component( void )
{

	/* Call the libsensors cleaning function before leaving */
	sensors_cleanup(  );

	return PAPI_OK;
}

int
_lmsensors_shutdown_thread( hwd_context_t *ctx )
{
    ( void ) ctx;

    return PAPI_OK;
}



/* This function sets various options in the component
 * The valid codes being passed in are PAPI_SET_DEFDOM,
 * PAPI_SET_DOMAIN, PAPI_SETDEFGRN, PAPI_SET_GRANUL * and PAPI_SET_INHERIT
 */
int
_lmsensors_ctl( hwd_context_t *ctx, int code, _papi_int_option_t *option )
{
    ( void ) ctx;
    ( void ) code;
    ( void ) option;
    return PAPI_OK;
}


int
_lmsensors_update_control_state( hwd_control_state_t *ctl,
				 NativeInfo_t * native, 
				 int count,
				 hwd_context_t *ctx )
{
    int i, index;
    ( void ) ctx;
    ( void ) ctl;

    for ( i = 0; i < count; i++ ) {
	index = native[i].ni_event;
	native[i].ni_position =
			lm_sensors_native_table[index].resources.selector - 1;
    }
    return PAPI_OK;
}


/*
 * This function has to set the bits needed to count different domains
 * In particular: PAPI_DOM_USER, PAPI_DOM_KERNEL PAPI_DOM_OTHER
 * By default return PAPI_EINVAL if none of those are specified
 * and PAPI_OK with success
 * PAPI_DOM_USER is only user context is counted
 * PAPI_DOM_KERNEL is only the Kernel/OS context is counted
 * PAPI_DOM_OTHER  is Exception/transient mode (like user TLB misses)
 * PAPI_DOM_ALL   is all of the domains
 */
int
_lmsensors_set_domain( hwd_control_state_t *ctl, int domain )
{
    int found = 0;
    ( void ) ctl;
	
	if ( PAPI_DOM_USER & domain )
		found = 1;

	if ( PAPI_DOM_KERNEL & domain )
		found = 1;

	if ( PAPI_DOM_OTHER & domain )
		found = 1;

	if ( !found )
		return ( PAPI_EINVAL );

	return ( PAPI_OK );
}


/*
 *
 */
int
_lmsensors_reset( hwd_context_t *ctx, hwd_control_state_t *ctl )
{
    ( void ) ctx;
    ( void ) ctl;
    return PAPI_OK;
}


/*
 * Native Event functions
 */
int
_lmsensors_ntv_enum_events( unsigned int *EventCode, int modifier )
{

	switch ( modifier ) {
	case PAPI_ENUM_FIRST:
		*EventCode = 0;

		return PAPI_OK;
		break;

	case PAPI_ENUM_EVENTS:
	{
		int index = *EventCode;

		if ( index < num_events - 1 ) {
			*EventCode = *EventCode + 1;
			return PAPI_OK;
		} else
			return PAPI_ENOEVNT;

		break;
	}
	default:
		return PAPI_EINVAL;
	}
	return PAPI_EINVAL;
}

/*
 *
 */
int
_lmsensors_ntv_code_to_name( unsigned int EventCode, char *name, int len )
{
	int index = EventCode;

	if (index>=0 && index<num_events) {
	   strncpy( name, lm_sensors_native_table[index].name, len );
	}

	return PAPI_OK;
}

/*
 *
 */
int
_lmsensors_ntv_code_to_descr( unsigned int EventCode, char *name, int len )
{
	int index = EventCode;

	if (index>=0 && index<num_events) {
	   strncpy( name, lm_sensors_native_table[index].description, len );
	}
	return PAPI_OK;
}



/*
 *
 */
papi_vector_t _lmsensors_vector = {
   .cmp_info = {
        /* component information (unspecified values are initialized to 0) */
	.name = "lmsensors",
	.short_name = "lmsensors",
	.version = "5.0",
	.description = "Linux LMsensor statistics",
	.num_mpx_cntrs = LM_SENSORS_MAX_COUNTERS,
	.num_cntrs = LM_SENSORS_MAX_COUNTERS,
	.default_domain = PAPI_DOM_USER,
	//.available_domains = PAPI_DOM_USER,
	.default_granularity = PAPI_GRN_THR,
	.available_granularities = PAPI_GRN_THR,
	.hardware_intr_sig = PAPI_INT_SIGNAL,

	/* component specific cmp_info initializations */
	.fast_real_timer = 0,
	.fast_virtual_timer = 0,
	.attach = 0,
	.attach_must_ptrace = 0,
	.available_domains = PAPI_DOM_USER | PAPI_DOM_KERNEL,
  },

        /* sizes of framework-opaque component-private structures */
	.size = {
	   .context = sizeof ( _lmsensors_context_t ),
	   .control_state = sizeof ( _lmsensors_control_state_t ),
	   .reg_value = sizeof ( _lmsensors_register_t ),
	   .reg_alloc = sizeof ( _lmsensors_reg_alloc_t ),
  },
	/* function pointers in this component */
     .init_thread =          _lmsensors_init_thread,
     .init_component =       _lmsensors_init_component,
     .init_control_state =   _lmsensors_init_control_state,
     .start =                _lmsensors_start,
     .stop =                 _lmsensors_stop,
     .read =                 _lmsensors_read,
     .shutdown_thread =      _lmsensors_shutdown_thread,
     .shutdown_component =   _lmsensors_shutdown_component,
     .ctl =                  _lmsensors_ctl,
     .update_control_state = _lmsensors_update_control_state,
     .set_domain =           _lmsensors_set_domain,
     .reset =                _lmsensors_reset,
	
     .ntv_enum_events =      _lmsensors_ntv_enum_events,
     .ntv_code_to_name =     _lmsensors_ntv_code_to_name,
     .ntv_code_to_descr =    _lmsensors_ntv_code_to_descr,
};
