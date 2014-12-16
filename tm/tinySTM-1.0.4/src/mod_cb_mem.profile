
#include "atomic.h"

/*
* Profile: generates the number of times the system malloc() and free() are called,
* organized by class sizes.
*
* Note: this will probably only work with 64bit machines.
*/

/* number of size categories, in chuncks of 8 bytes (64bits machines) */
#define NUM_CATEGORIES 128

/* max number of freed blocks that can be stored per transaction */
#define MAX_BLOCK_SIZE 256
/*****************************************************************************/


typedef struct mod_cb_info {
  unsigned short commit_size;           /* Array size for commit callbacks */
  unsigned short commit_nb;             /* Number of commit callbacks */
  mod_cb_entry_t *commit;               /* Commit callback entries */
  unsigned short abort_size;            /* Array size for abort callbacks */
  unsigned short abort_nb;              /* Number of abort callbacks */
  mod_cb_entry_t *abort;                /* Abort callback entries */


/*****************************************************************************/
/* 
  These arrays store a counter with the number of times the following events occurred:
  (i) mallocs commited, (ii) mallocs aborted, (iii) frees committed, and (iv) frees
  aborted
*/
  unsigned long mallocs_committed[NUM_CATEGORIES+1];
  unsigned long mallocs_aborted[NUM_CATEGORIES+1];
  unsigned long frees_committed[NUM_CATEGORIES+1];
  unsigned long frees_aborted[NUM_CATEGORIES+1];

  /* max values */
  unsigned long max_mallocs_committed;
  unsigned long max_mallocs_aborted;
  unsigned long max_frees_committed;
  unsigned long max_frees_aborted;

  /* temporary list to store the categories for mallocs and frees */
  unsigned long current_malloc_list[MAX_BLOCK_SIZE];
  unsigned long malloc_list_head;
  unsigned long current_free_list[MAX_BLOCK_SIZE];
  unsigned long free_list_head;


  /* transactions committed and aborted */
  unsigned long commits;
  unsigned long aborts;
  
 /* how many transactions committed/aborted and performed some (de)allocation */
  unsigned long commits_with_mallocs;
  unsigned long aborts_with_mallocs;
  unsigned long commits_with_frees;
  unsigned long aborts_with_frees;
} mod_cb_info_t;

/* max number of mallocs (both committed and aborted) issued by a single thread*/
static unsigned long mallocs_max[NUM_CATEGORIES+1];  

/* These are system-wide variables that are filled during finalization */

static unsigned long mallocs_committed[NUM_CATEGORIES+1];
static unsigned long mallocs_aborted[NUM_CATEGORIES+1];
static unsigned long frees_committed[NUM_CATEGORIES+1];
static unsigned long frees_aborted[NUM_CATEGORIES+1];
  
static unsigned long max_mallocs_committed;
static unsigned long max_mallocs_aborted;
static unsigned long max_frees_committed;
static unsigned long max_frees_aborted;


static unsigned long total_commits;
static unsigned long total_aborts;

static unsigned long total_commits_with_mallocs;
static unsigned long total_aborts_with_mallocs;
static unsigned long total_commits_with_frees;
static unsigned long total_aborts_with_frees;
/*****************************************************************************/


static INLINE void
mod_cb_add_on_abort(mod_cb_info_t *icb, void (*f)(void *arg), void *arg)
{
  if (unlikely(icb->abort_nb >= icb->abort_size)) {
    icb->abort_size *= 2;
    icb->abort = xrealloc(icb->abort, sizeof(mod_cb_entry_t) * icb->abort_size);
    if (icb->abort == NULL) {
      perror("realloc error");
      exit(1);
    }
  }
  icb->abort[icb->abort_nb].f = f;
  icb->abort[icb->abort_nb].arg = arg;
  icb->abort_nb++;
}


static INLINE void
mod_cb_add_on_commit(mod_cb_info_t *icb, void (*f)(void *arg), void *arg)
{
  if (unlikely(icb->commit_nb >= icb->commit_size)) {
    icb->commit_size *= 2;
    icb->commit = xrealloc(icb->commit, sizeof(mod_cb_entry_t) * icb->commit_size);
    if (icb->commit == NULL) {
      perror("realloc error");
      exit(1);
    }
  }
  icb->commit[icb->commit_nb].f = f;
  icb->commit[icb->commit_nb].arg = arg;
  icb->commit_nb++;
}



/* ################################################################### *
 * MEMORY ALLOCATION FUNCTIONS
 * ################################################################### */


/* ################################################################### *
 * INT_STM_MALLOC
 * ################################################################### */
static INLINE void *
int_stm_malloc(struct stm_tx *tx, size_t size)
{
  /* Memory will be freed upon abort */
  mod_cb_info_t *icb;
  void *addr;

  assert(mod_cb.key >= 0);
  icb = (mod_cb_info_t *)stm_get_specific(mod_cb.key);
  assert(icb != NULL);

  /* Round up size */
  if (sizeof(stm_word_t) == 4) {
    size = (size + 3) & ~(size_t)0x03;
  } else {
    size = (size + 7) & ~(size_t)0x07;
  }


/*****************************************************************************/
/* Note that 'size' is rounded up to a multiple of 8 in a 64-bit machine */
  int category = size >> 3;
  if (unlikely(category > NUM_CATEGORIES))
  {
    perror("invalid category\n");
    exit(1);
  }
//  icb->mallocs[category]++;
/*****************************************************************************/


  if (unlikely((addr = malloc(size)) == NULL)) {
    perror("malloc");
    exit(1);
  }

  
/*****************************************************************************/
/*
 * We store the category for each allocation in a list so that we can figure
 * out how many allocations (and size) will be undo due to abort
 */
  icb->current_malloc_list[icb->malloc_list_head++] = category;
  if (unlikely(icb->malloc_list_head >= MAX_BLOCK_SIZE)) {
    perror("malloc_list overflow");
    exit(1);
  }
/*****************************************************************************/

  mod_cb_add_on_abort(icb, free, addr);

  return addr;
}


/* ################################################################### *
 * INT_STM_CALLOC
 * ################################################################### */
static inline
void *int_stm_calloc(struct stm_tx *tx, size_t nm, size_t size)
{
  /* Memory will be freed upon abort */
  mod_cb_info_t *icb;
  void *addr;

  assert(mod_cb.key >= 0);
  icb = (mod_cb_info_t *)stm_get_specific(mod_cb.key);
  assert(icb != NULL);

  /* Round up size */
  if (sizeof(stm_word_t) == 4) {
    size = (size + 3) & ~(size_t)0x03;
  } else {
    size = (size + 7) & ~(size_t)0x07;
  }


/*****************************************************************************/
  int category = size >> 3;
  if (unlikely(category > NUM_CATEGORIES))
  {
    perror("invalid category\n");
    exit(1);
  }
//  icb->mallocs[category]++;
/*****************************************************************************/


  if ((addr = calloc(nm, size)) == NULL) {
    perror("calloc");
    exit(1);
  }
  

/*****************************************************************************/
/*
 * We store the category for each allocation in a list so that we can figure
 * out how many allocations (and size) will be undo due to abort
 */
/*****************************************************************************/


  mod_cb_add_on_abort(icb, free, addr);

  return addr;
}


#ifdef EPOCH_GC
static void
epoch_free(void *addr)
{
  if (mod_cb.use_gc) {
    /* TODO use tx->end could be also used */
    stm_word_t t = stm_get_clock();
    gc_free(addr, t);
  } else {
    free(addr);
  }
}
#endif /* EPOCH_GC */


/* ################################################################### *
 * INT_STM_FREE2
 * ################################################################### */
static inline
void int_stm_free2(struct stm_tx *tx, void *addr, size_t idx, size_t size)
{
  /* Memory disposal is delayed until commit */
  mod_cb_info_t *icb;

  assert(mod_cb.key >= 0);
  icb = (mod_cb_info_t *)stm_get_specific(mod_cb.key);
  assert(icb != NULL);


  /* TODO: if block allocated in same transaction => no need to overwrite */
  if (size > 0) {
    stm_word_t *a;
    /* Overwrite to prevent inconsistent reads */
    if (sizeof(stm_word_t) == 4) {
      idx = (idx + 3) >> 2;
      size = (size + 3) >> 2;
    } else {
      idx = (idx + 7) >> 3;
      size = (size + 7) >> 3;
    }


/*****************************************************************************/
    if (unlikely(size > NUM_CATEGORIES))
    {
      perror("invalid category\n");
      exit(1);
    }
    /*
     * we need to store the categories for each speculative free() 
     */
    icb->current_free_list[icb->free_list_head++] = size;
    if (unlikely(icb->free_list_head >= MAX_BLOCK_SIZE)) {
      perror("free_list overflow");
      exit(1);
    }
/*****************************************************************************/


    a = (stm_word_t *)addr + idx;
    while (size-- > 0) {
      /* Acquire lock and update version number */
      stm_store2_tx(tx, a++, 0, 0);
    }
  }


  /* Schedule for removal */
#ifdef EPOCH_GC
  mod_cb_add_on_commit(icb, epoch_free, addr);
#else /* ! EPOCH_GC */
  mod_cb_add_on_commit(icb, free, addr);
#endif /* ! EPOCH_GC */
}




/*
 * Called upon transaction commit.
 */
/* ################################################################### *
 * ON_COMMIT
 * ################################################################### */
static void mod_cb_on_commit(void *arg)
{
  mod_cb_info_t *icb;

  icb = (mod_cb_info_t *)stm_get_specific(mod_cb.key);
  assert(icb != NULL);


/*****************************************************************************/
  icb->commits++;
  if (icb->free_list_head != 0)
    icb->commits_with_frees++;
  if (icb->malloc_list_head != 0)
    icb->commits_with_mallocs++;

  /* update max number of mallocs committed if necessary */
  if (icb->malloc_list_head > icb->max_mallocs_committed)
    icb->max_mallocs_committed = icb->malloc_list_head;

  /* same for max frees */
  if (icb->free_list_head > icb->max_frees_committed)
    icb->max_frees_committed = icb->free_list_head;

  /* update the list (per category) of mallocs and frees committed */
  int i;
  for (i=0; i<icb->malloc_list_head; i++)
    icb->mallocs_committed[icb->current_malloc_list[i]]++;
  for (i=0; i<icb->free_list_head; i++)
    icb->frees_committed[icb->current_free_list[i]]++;
  
  /* reset pointers */
  icb->malloc_list_head = 0;
  icb->free_list_head = 0;
/*****************************************************************************/  
  

  /* Call commit callback */
  while (icb->commit_nb > 0) {
    icb->commit_nb--;
    icb->commit[icb->commit_nb].f(icb->commit[icb->commit_nb].arg);
  }
  /* Reset abort callback */
  icb->abort_nb = 0;
}


/*
 * Called upon transaction abort.
 */
/* ################################################################### *
 * ON_ABORT
 * ################################################################### */
static void mod_cb_on_abort(void *arg)
{
  mod_cb_info_t *icb;

  icb = (mod_cb_info_t *)stm_get_specific(mod_cb.key);
  assert(icb != NULL);

  
/*****************************************************************************/  
  icb->aborts++;
  if (icb->free_list_head != 0)
    icb->aborts_with_frees++;
  if (icb->malloc_list_head != 0)
    icb->aborts_with_mallocs++;
  

  /* update max number of mallocs aborted if necessary */
  if (icb->malloc_list_head > icb->max_mallocs_aborted)
    icb->max_mallocs_aborted = icb->malloc_list_head;

  /* same for max frees */
  if (icb->free_list_head > icb->max_frees_aborted)
    icb->max_frees_aborted = icb->free_list_head;

  /* update the list (per category) of mallocs and frees aborted */
  int i;
  for (i=0; i<icb->malloc_list_head; i++)
    icb->mallocs_aborted[icb->current_malloc_list[i]]++;
  for (i=0; i<icb->free_list_head; i++)
    icb->frees_aborted[icb->current_free_list[i]]++;
  
  /* reset pointers */
  icb->malloc_list_head = 0;
  icb->free_list_head = 0;
/*****************************************************************************/  


  /* Call abort callback */
  while (icb->abort_nb > 0) {
    icb->abort_nb--;
    icb->abort[icb->abort_nb].f(icb->abort[icb->abort_nb].arg);
  }
  /* Reset commit callback */
  icb->commit_nb = 0;
}

/*
 * Called upon thread creation.
 */
/* ################################################################### *
 * ON_THREAD_INIT
 * ################################################################### */
static void mod_cb_on_thread_init(void *arg)
{
  mod_cb_info_t *icb;

  if ((icb = (mod_cb_info_t *)xmalloc(sizeof(mod_cb_info_t))) == NULL)
    goto err_malloc;
  icb->commit_nb = icb->abort_nb = 0;
  icb->commit_size = icb->abort_size = DEFAULT_CB_SIZE;
  icb->commit = xmalloc(sizeof(mod_cb_entry_t) * icb->commit_size);
  icb->abort = xmalloc(sizeof(mod_cb_entry_t) * icb->abort_size);
  if (unlikely(icb->commit == NULL || icb->abort == NULL))
    goto err_malloc;


/*****************************************************************************/
  /* Initialize the transaction-based data structures */
  int i;
  for (i=0; i<NUM_CATEGORIES+1; i++) {
    icb->mallocs_committed[i] = 0;
    icb->mallocs_aborted[i] = 0;
    icb->frees_committed[i] = 0;
    icb->frees_aborted[i] = 0;
  }

  /* max values */
  icb->max_mallocs_committed = 0;
  icb->max_mallocs_aborted = 0;
  icb->max_frees_committed = 0;
  icb->max_frees_aborted = 0;

  icb->malloc_list_head = 0;
  icb->free_list_head = 0;
  
  
  icb->commits = 0;
  icb->aborts = 0;
  
  icb->commits_with_mallocs = 0;
  icb->aborts_with_mallocs = 0;
  icb->commits_with_frees = 0;
  icb->aborts_with_frees = 0;
/*****************************************************************************/


  stm_set_specific(mod_cb.key, icb);

  return;
 err_malloc:
   perror("malloc");
   exit(1);
}


/*****************************************************************************/  
void print_mod_mem_stats()
{
  int i;
  fprintf(stdout, "Total tx commits/aborts: %lu %lu\n", total_commits, total_aborts);
  fprintf(stdout, "Total tx commits/aborts with malloc: %lu %lu\n", total_commits_with_mallocs, total_aborts_with_mallocs);
  fprintf(stdout, "Total tx commits/aborts with free: %lu %lu\n", total_commits_with_frees, total_aborts_with_frees);
  fprintf(stdout, "Max sequences: committed/aborted mallocs: %lu %lu - committed/aborted frees: %lu %lu\n", 
                  max_mallocs_committed, max_mallocs_aborted, max_frees_committed, max_frees_aborted);
  
  unsigned long total_mallocs_committed = 0, total_mallocs_aborted = 0, total_frees_committed = 0, total_frees_aborted = 0;
  /* we do not consider class 0 */
  for (i=1; i<NUM_CATEGORIES+1; i++) {
    total_mallocs_committed += mallocs_committed[i];
    total_mallocs_aborted += mallocs_aborted[i];
    total_frees_committed += frees_committed[i];
    total_frees_aborted += frees_aborted[i];
  }

  fprintf(stdout, "total:\n \
         mallocs_committed = %lu\n \
         mallocs_aborted   = %lu\n \
         frees_committed   = %lu\n \
         frees_aborted     = %lu\n", 
          total_mallocs_committed, total_mallocs_aborted, total_frees_committed, total_frees_aborted);

  fprintf(stdout, "  cat -  max_mallocs_single_thread - mallocs_committed - mallocs_aborted - frees_committed - frees_aborted\n");
  for (i=1; i<NUM_CATEGORIES+1; i++) {
    fprintf(stdout, "- %d %lu %lu %lu %lu %lu\n", i, mallocs_max[i], mallocs_committed[i],
                      mallocs_aborted[i], frees_committed[i], frees_aborted[i]); 
  }

  /*
   * Note that class 0 is not used - we just make sure that that holds
   */
  if (mallocs_max[0] != 0 || frees_committed[0] != 0 || frees_aborted[0] != 0) {
    fprintf(stdout, "Class 0 should be zero!\n");
    exit(1);
  }
  
}
/*****************************************************************************/  


/*
 * Called upon thread deletion.
 */
/* ################################################################### *
 * ON_THREAD_EXIT
 * ################################################################### */
static void mod_cb_on_thread_exit(void *arg)
{
  mod_cb_info_t *icb;

  icb = (mod_cb_info_t *)stm_get_specific(mod_cb.key);
  assert(icb != NULL);


/*****************************************************************************/
  unsigned long max, local_max;
  int i;
  /* for each category (note that cat 0 is meaningless) ... */
  for (i=0; i<NUM_CATEGORIES+1; i++) {

  /* get the max number of malloc()s for each category */
    local_max = icb->mallocs_committed[i] + icb->mallocs_aborted[i];
  retry_max:
    max = ATOMIC_LOAD(&mallocs_max[i]);
    if (local_max > max) {
      if (ATOMIC_CAS_FULL(&mallocs_max[i], max, local_max) == 0)
        goto retry_max;
    }
  /* update global variables with local ones */
    ATOMIC_FETCH_ADD_FULL(&mallocs_committed[i], icb->mallocs_committed[i]);
    ATOMIC_FETCH_ADD_FULL(&mallocs_aborted[i], icb->mallocs_aborted[i]);
    ATOMIC_FETCH_ADD_FULL(&frees_committed[i], icb->frees_committed[i]);
    ATOMIC_FETCH_ADD_FULL(&frees_aborted[i], icb->frees_aborted[i]);
  }
    

retry_max_mallocs_committed:
  max = ATOMIC_LOAD(&max_mallocs_committed);
  if (icb->max_mallocs_committed > max) {
    if (ATOMIC_CAS_FULL(&max_mallocs_committed, max, icb->max_mallocs_committed) == 0)
      goto retry_max_mallocs_committed;
  } 

retry_max_mallocs_aborted:
  max = ATOMIC_LOAD(&max_mallocs_aborted);
  if (icb->max_mallocs_aborted > max) {
    if (ATOMIC_CAS_FULL(&max_mallocs_aborted, max, icb->max_mallocs_aborted) == 0)
      goto retry_max_mallocs_aborted;
  } 

retry_max_frees_committed:
  max = ATOMIC_LOAD(&max_frees_committed);
  if (icb->max_frees_committed > max) {
    if (ATOMIC_CAS_FULL(&max_frees_committed, max, icb->max_frees_committed) == 0)
      goto retry_max_frees_committed;
  } 

retry_max_frees_aborted:
  max = ATOMIC_LOAD(&max_frees_aborted);
  if (icb->max_frees_aborted > max) {
    if (ATOMIC_CAS_FULL(&max_frees_aborted, max, icb->max_frees_aborted) == 0)
      goto retry_max_frees_aborted;
  } 

    
  ATOMIC_FETCH_ADD_FULL(&total_commits, icb->commits);
  ATOMIC_FETCH_ADD_FULL(&total_aborts, icb->aborts);
  
  ATOMIC_FETCH_ADD_FULL(&total_commits_with_mallocs, icb->commits_with_mallocs);
  ATOMIC_FETCH_ADD_FULL(&total_aborts_with_mallocs, icb->aborts_with_mallocs);
  ATOMIC_FETCH_ADD_FULL(&total_commits_with_frees, icb->commits_with_frees);
  ATOMIC_FETCH_ADD_FULL(&total_aborts_with_frees, icb->aborts_with_frees);
/*****************************************************************************/
   

  xfree(icb->abort);
  xfree(icb->commit);
  xfree(icb);
}


/* ################################################################### *
 * CB_MEM_INIT
 * ################################################################### */
static INLINE void
mod_cb_mem_init(void)
{
  if (mod_cb.key >= 0)
    goto already_init;

  stm_register(mod_cb_on_thread_init, mod_cb_on_thread_exit, NULL, NULL, mod_cb_on_commit, mod_cb_on_abort, NULL);
  mod_cb.key = stm_create_specific();
  if (unlikely(mod_cb.key < 0)) {
    fprintf(stderr, "Cannot create specific key\n");
    exit(1);
  }


/*****************************************************************************/
  /* initialize global variables */
  int i;
  for (i=0; i<NUM_CATEGORIES+1; i++) {
    mallocs_max[i] = 0;  
    mallocs_committed[i] = 0;
    mallocs_aborted[i] = 0;
    frees_committed[i] = 0;
    frees_aborted[i] = 0;
  }
  
  max_mallocs_committed = 0;
  max_mallocs_aborted = 0;
  max_frees_committed = 0;
  max_frees_aborted = 0;


  total_commits = 0;
  total_aborts = 0;

  total_commits_with_mallocs = 0;
  total_aborts_with_mallocs = 0;
  total_commits_with_frees = 0;
  total_aborts_with_frees = 0;
/*****************************************************************************/


 already_init:
  return;
}


void mod_mem_init(int use_gc)
{
  mod_cb_mem_init();
#ifdef EPOCH_GC
  mod_cb.use_gc = use_gc;
#endif /* EPOCH_GC */
}
