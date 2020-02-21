/* =============================================================================
 *
 * tm.h
 *
 * Utility defines for transactional memory
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Authors: Chi Cao Minh and Martin Trautmann
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


#ifndef TM_H
#define TM_H 1

#ifdef HAVE_CONFIG_H
# include "STAMP_config.h"
#endif

/* =============================================================================
 * Simulator Specific Interface
 *
 * MAIN(argc, argv)
 *     Declare the main function with argc being the identifier for the argument
 *     count and argv being the name for the argument string list
 *
 * MAIN_RETURN(int_val)
 *     Returns from MAIN function
 *
 * GOTO_SIM()
 *     Switch simulator to simulation mode
 *
 * GOTO_REAL()
 *     Switch simulator to non-simulation (real) mode
 *     Note: use in sequential region only
 *
 * IS_IN_SIM()
 *     Returns true if simulator is in simulation mode
 *
 * SIM_GET_NUM_CPU(var)
 *     Assigns the number of simulated CPUs to "var"
 *
 * P_MEMORY_STARTUP
 *     Start up the memory allocator system that handles malloc/free
 *     in parallel regions (but not in transactions)
 *
 * P_MEMORY_SHUTDOWN
 *     Shutdown the memory allocator system that handles malloc/free
 *     in parallel regions (but not in transactions)
 *
 * =============================================================================
 */
#  include <stdio.h>

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    /* nothing */
#  define GOTO_REAL()                   /* nothing */
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */


/* =============================================================================
 * Transactional Memory System Interface
 *
 * TM_ARG
 * TM_ARG_ALONE
 * TM_ARGDECL
 * TM_ARGDECL_ALONE
 *     Used to pass TM thread meta data to functions (see Examples below)
 *
 * TM_STARTUP(numThread)
 *     Startup the TM system (call before any other TM calls)
 *
 * TM_SHUTDOWN()
 *     Shutdown the TM system
 *
 * TM_THREAD_ENTER()
 *     Call when thread first enters parallel region
 *
 * TM_THREAD_EXIT()
 *     Call when thread exits last parallel region
 *
 * P_MALLOC(size)
 *     Allocate memory inside parallel region
 *
 * P_FREE(ptr)
 *     Deallocate memory inside parallel region
 *
 * TM_MALLOC(size)
 *     Allocate memory inside atomic block / transaction
 *
 * TM_FREE(ptr)
 *     Deallocate memory inside atomic block / transaction
 *
 * TM_BEGIN()
 *     Begin atomic block / transaction
 *
 * TM_BEGIN_RO()
 *     Begin atomic block / transaction that only reads shared data
 *
 * TM_END()
 *     End atomic block / transaction
 *
 * TM_RESTART()
 *     Restart atomic block / transaction
 *
 * TM_EARLY_RELEASE()
 *     Remove speculatively read line from the read set
 *
 * =============================================================================
 *
 * Example Usage:
 *
 *     MAIN(argc,argv)
 *     {
 *         TM_STARTUP(8);
 *         // create 8 threads and go parallel
 *         TM_SHUTDOWN();
 *     }
 *
 *     void parallel_region ()
 *     {
 *         TM_THREAD_ENTER();
 *         subfunction1(TM_ARG_ALONE);
 *         subfunction2(TM_ARG  1, 2, 3);
 *         TM_THREAD_EXIT();
 *     }
 *
 *     void subfunction1 (TM_ARGDECL_ALONE)
 *     {
 *         TM_BEGIN_RO()
 *         // ... do work that only reads shared data ...
 *         TM_END()
 *
 *         long* array = (long*)P_MALLOC(10 * sizeof(long));
 *         // ... do work ...
 *         P_FREE(array);
 *     }
 *
 *     void subfunction2 (TM_ARGDECL  long a, long b, long c)
 *     {
 *         TM_BEGIN();
 *         long* array = (long*)TM_MALLOC(a * b * c * sizeof(long));
 *         // ... do work that may read or write shared data ...
 *         TM_FREE(array);
 *         TM_END();
 *     }
 *
 * =============================================================================
 */


/* =============================================================================
 * HTM - Hardware Transactional Memory
 * =============================================================================
 */

#ifdef HTM

# include <immintrin.h>
# include <stdatomic.h>

# define HTM_STATS(status)              atomic_ullong status[9] = { 0 }
# define HTM_STATS_EXTERN(status)       extern atomic_ullong status[9]
# define HTM_STATS_PRINT(status)        HTM_STATS_EXTERN(status); printf("UNKNOWN: %llu\n_XABORT_EXPLICIT: %llu\n_XABORT_RETRY: %llu\n_XABORT_CONFLICT: %llu\n_XABORT_CAPACITY: %llu\n_XABORT_DEBUG: %llu\n_XABORT_NESTED: %llu\n_XBEGIN_STARTED: %llu\n_XBEGIN_FAILED: %llu\n", status[0], status[1], status[2], status[3], status[4], status[5], status[6], status[7], status[8])

# define HTM_BEGIN(local, global)       (({ local = _xbegin(); if (local != _XBEGIN_STARTED) { atomic_fetch_add(&global[8], 1); if (!local) atomic_fetch_add(&global[0], 1); else { if (local & _XABORT_EXPLICIT) atomic_fetch_add(&global[1], 1); if (local & _XABORT_RETRY) atomic_fetch_add(&global[2], 1); if (local & _XABORT_CONFLICT) atomic_fetch_add(&global[3], 1); if (local & _XABORT_CAPACITY) atomic_fetch_add(&global[4], 1); if (local & _XABORT_DEBUG) atomic_fetch_add(&global[5], 1); if (local & _XABORT_NESTED) atomic_fetch_add(&global[6], 1); } } local; }) == _XBEGIN_STARTED)
# define HTM_RESTART()                  _xabort(0xAA)
# define HTM_EARLY_RELEASE(var)         /* nothing */
# define HTM_TX_INIT                    unsigned tsx_status

# ifdef HTM_STM

/* HTM commits into STM, fallback is STM, no need for synchronization */
#  define HTM_END(global)               STM_END(); _xend(); atomic_fetch_add(&global[7], 1)

#  define TM_BEGIN()                    STM_BEGIN()
#  define TM_BEGIN_NOOVR()              STM_BEGIN_NOOVR()
#  define TM_END()                      STM_END()

# ifdef HTM_RETRY_ABLE
#  define HTM_RETRY(local, d)           if (local & _XABORT_RETRY) goto d
# else
#  define HTM_RETRY(local, d)           /* nothing */
# endif /* HTM_RETRY_ABLE */

#  define TM_INIT_GLOBAL                /* nothing */
#  define HTM_LOCK_READ()               STM_BEGIN()
#  define HTM_LOCK_SET()                /* nothing */
#  define HTM_LOCK_UNSET()              /* nothing */

#  define HTM_SHARED_WRITE(var, val)    TM_SHARED_WRITE(var, val)
#  define HTM_SHARED_WRITE_P(var, val)  TM_SHARED_WRITE_P(var, val)
#  define HTM_SHARED_WRITE_F(var, val)  TM_SHARED_WRITE_F(var, val)

#  define HTM_SHARED_READ(var)          TM_SHARED_READ(var)
#  define HTM_SHARED_READ_P(var)        TM_SHARED_READ_P(var)
#  define HTM_SHARED_READ_F(var)        TM_SHARED_READ_F(var)

#  define HTM_LOCAL_WRITE(var, val)     ({var = val; var;})
#  define HTM_LOCAL_WRITE_P(var, val)   ({var = val; var;})
#  define HTM_LOCAL_WRITE_F(var, val)   ({var = val; var;})

#  define HTM_MALLOC(size)              TM_MALLOC(size)
#  define HTM_FREE(ptr)                 TM_FREE(ptr)

# elif defined(HTM_DIRECT_STM)

/* HTM commits directly, fallback is STM, must ensure exclusion */
#  define HTM_END(global)               _xend(); atomic_fetch_add(&global[7], 1)

#  define TM_BEGIN()                    extern atomic_ullong htm_lock; HTM_LOCK_SET(); STM_BEGIN()
#  define TM_BEGIN_NOOVR()              extern atomic_ullong htm_lock; HTM_LOCK_SET(); STM_BEGIN_NOOVR()
#  define TM_END()                      STM_END(); HTM_LOCK_UNSET()

# ifdef HTM_RETRY_ABLE
#  define HTM_RETRY(local, d)           if (local & _XABORT_RETRY) goto d; else if (local & _XABORT_EXPLICIT) { extern atomic_ullong htm_lock; while (atomic_load(&htm_lock)) { } goto d; }
# else
#  define HTM_RETRY(local, d)           if (local & _XABORT_EXPLICIT) { extern atomic_ullong htm_lock; while (atomic_load(&htm_lock)) { } goto d; }
# endif /* HTM_RETRY_ABLE */

#  define TM_INIT_GLOBAL                atomic_ullong htm_lock __attribute__((aligned(64))) = 0
#  define HTM_LOCK_READ()               extern atomic_ullong htm_lock; if (atomic_load(&htm_lock)) HTM_RESTART()
#  define HTM_LOCK_SET()                atomic_fetch_add(&htm_lock, 1)
#  define HTM_LOCK_UNSET()              atomic_fetch_add(&htm_lock, -1)

#  define HTM_SHARED_WRITE(var, val)    ({var = val; var;})
#  define HTM_SHARED_WRITE_P(var, val)  ({var = val; var;})
#  define HTM_SHARED_WRITE_F(var, val)  ({var = val; var;})

#  define HTM_SHARED_READ(var)          (var)
#  define HTM_SHARED_READ_P(var)        (var)
#  define HTM_SHARED_READ_F(var)        (var)

#  define HTM_LOCAL_WRITE(var, val)     ({var = val; var;})
#  define HTM_LOCAL_WRITE_P(var, val)   ({var = val; var;})
#  define HTM_LOCAL_WRITE_F(var, val)   ({var = val; var;})

#  define HTM_MALLOC(size)              malloc(size)
#  define HTM_FREE(ptr)                 free(ptr)

# elif defined(HTM_DIRECT_STM_NOREC)

/* HTM commits directly, fallback is STM, must ensure exclusion between HTM and STM */
#  define HTM_END(global)               extern atomic_ullong sw_exists; if (atomic_load(&sw_exists)) { stm_inc_counter(); } _xend(); atomic_fetch_add(&global[7], 1)

#  define TM_BEGIN()                    extern atomic_ullong sw_exists; atomic_fetch_add(&sw_exists, 1); STM_BEGIN()
#  define TM_BEGIN_NOOVR()              extern atomic_ullong sw_exists; atomic_fetch_add(&sw_exists, 1); STM_BEGIN_NOOVR()
#  define TM_END()                      extern atomic_ullong sw_exists; STM_END(); atomic_fetch_sub(&sw_exists, 1)

# ifdef HTM_RETRY_ABLE
#  define HTM_RETRY(local, d)           if (local & _XABORT_RETRY) goto d
# else
#  define HTM_RETRY(local, d)           /* nothing */
# endif /* HTM_RETRY_ABLE */

#  define TM_INIT_GLOBAL                atomic_ullong sw_exists __attribute__((aligned(64))) = 0;
#  define HTM_LOCK_READ()               if (stm_get_clock() & 1) { while (1) { }; }
#  define HTM_LOCK_SET()                /* nothing */
#  define HTM_LOCK_UNSET()              /* nothing */

#  define HTM_SHARED_WRITE(var, val)    ({var = val; var;})
#  define HTM_SHARED_WRITE_P(var, val)  ({var = val; var;})
#  define HTM_SHARED_WRITE_F(var, val)  ({var = val; var;})

#  define HTM_SHARED_READ(var)          (var)
#  define HTM_SHARED_READ_P(var)        (var)
#  define HTM_SHARED_READ_F(var)        (var)

#  define HTM_LOCAL_WRITE(var, val)     ({var = val; var;})
#  define HTM_LOCAL_WRITE_P(var, val)   ({var = val; var;})
#  define HTM_LOCAL_WRITE_F(var, val)   ({var = val; var;})

#  define HTM_MALLOC(size)              malloc(size)
#  define HTM_FREE(ptr)                 free(ptr)

# elif defined(HTM_DIRECT)

#  include <stdatomic.h>

/* HTM commits directly, fallback commits directly, must ensure exclusion */
#  define HTM_END(global)               _xend(); atomic_fetch_add(&global[7], 1)

#  define TM_BEGIN()                    extern atomic_bool htm_lock; HTM_LOCK_SET()
#  define TM_BEGIN_NOOVR()              TM_BEGIN()
#  define TM_END()                      HTM_LOCK_UNSET()

# ifdef HTM_RETRY_ABLE
#  define HTM_RETRY(local, d)           if (local & _XABORT_RETRY) goto d; else if (local & _XABORT_EXPLICIT) { extern atomic_bool htm_lock; while (atomic_load(&htm_lock)) { } goto d; }
# else
#  define HTM_RETRY(local, d)           if (local & _XABORT_EXPLICIT) { extern atomic_bool htm_lock; while (atomic_load(&htm_lock)) { } goto d; }
# endif /* HTM_RETRY_ABLE */

#  define TM_INIT_GLOBAL                atomic_bool htm_lock __attribute__((aligned(64))) = 0
#  define HTM_LOCK_READ()               extern atomic_bool htm_lock; if (atomic_load(&htm_lock)) HTM_RESTART()
#  define HTM_LOCK_SET()                atomic_bool htm_zero = 0; while (!atomic_compare_exchange_weak(&htm_lock, &htm_zero, 1)) { htm_zero = 0; }
#  define HTM_LOCK_UNSET()              atomic_store(&htm_lock, 0)

#  define HTM_SHARED_WRITE(var, val)    ({var = val; var;})
#  define HTM_SHARED_WRITE_P(var, val)  ({var = val; var;})
#  define HTM_SHARED_WRITE_F(var, val)  ({var = val; var;})

#  define HTM_SHARED_READ(var)          (var)
#  define HTM_SHARED_READ_P(var)        (var)
#  define HTM_SHARED_READ_F(var)        (var)

#  define HTM_LOCAL_WRITE(var, val)     ({var = val; var;})
#  define HTM_LOCAL_WRITE_P(var, val)   ({var = val; var;})
#  define HTM_LOCAL_WRITE_F(var, val)   ({var = val; var;})

#  define HTM_MALLOC(size)              malloc(size)
#  define HTM_FREE(ptr)                 free(ptr)

# else
#  error "Must define either HTM_STM, HTM_DIRECT_STM, or HTM_DIRECT!"
# endif

#else

# define HTM_BEGIN(local, global)       0
# define HTM_END(global)                /* nothing */
# define HTM_RESTART()                  /* nothing */
# define HTM_EARLY_RELEASE(var)         /* nothing */
# define HTM_TX_INIT                    /* nothing */

# define HTM_LOCK_READ()                /* nothing */
# define HTM_LOCK_SET()                 /* nothing */
# define HTM_LOCK_UNSET()               /* nothing */

# ifndef STM_HTM
#  define HTM_RETRY(local, d)           /* nothing */
#  define HTM_STATS(status)             /* nothing */
#  define HTM_STATS_EXTERN(status)      /* nothing */
#  define HTM_STATS_PRINT(status)       /* nothing */

#  define HTM_SHARED_WRITE(var, val)    /* nothing */
#  define HTM_SHARED_WRITE_P(var, val)  /* nothing */
#  define HTM_SHARED_WRITE_F(var, val)  /* nothing */

#  define HTM_SHARED_READ(var)          0
#  define HTM_SHARED_READ_P(var)        0
#  define HTM_SHARED_READ_F(var)        0
# endif /* STM_HTM */

# define HTM_LOCAL_WRITE(var, val)      /* nothing */
# define HTM_LOCAL_WRITE_P(var, val)    /* nothing */
# define HTM_LOCAL_WRITE_F(var, val)    /* nothing */

# define HTM_MALLOC(size)               NULL
# define HTM_FREE(ptr)                  /* nothing */

#endif /* HTM */

/* =============================================================================
 * STM - Software Transactional Memory
 * =============================================================================
 */

#ifdef STM

# include <string.h>

# define TM_ARG                       /* nothing */
# define TM_ARG_ALONE                 /* nothing */
# define TM_ARGDECL                   /* nothing */
# define TM_ARGDECL_ALONE             /* nothing */

# ifndef TM_STARTUP
#  define TM_STARTUP(numThread)       STM_STARTUP(numThread)
# endif /* TM_STARTUP */
# ifndef TM_BEGIN
#  define TM_BEGIN()                  STM_BEGIN()
# endif /* TM_BEGIN */
# ifndef TM_BEGIN_NOOVR
#  define TM_BEGIN_NOOVR()            STM_BEGIN_NOOVR()
# endif /* TM_BEGIN_NOOVR */
# ifndef TM_END
#  define TM_END()                    STM_END()
# endif /* TM_END */

# if defined (OTM)
#  include <omp.h>
#  include "tl2.h"

#  define TM_CALLABLE                 /* nothing */
#  define TM_CANCELLABLE              /* nothing */
#  define TM_PURE                     /* nothing */

#  define thread_getId()              omp_get_thread_num()
#  define thread_getNumThread()       omp_get_num_threads()
#  define thread_startup(numThread)   omp_set_num_threads(numThread)
#  define thread_shutdown()           /* nothing */

#  define TM_INIT_GLOBAL              /* nothing */
#  define TM_BEGIN()                  _Pragma ("omp transaction") {
#  define TM_BEGIN_NOOVR()            TM_BEGIN()
#  define TM_BEGIN_RO()               _Pragma ("omp transaction") {
#  define TM_END()                    }
#  define TM_RESTART()                omp_abort()

#  define TM_EARLY_RELEASE(var)       /* nothing */

#  define TM_THREAD_ENTER()           /* nothing */
#  define TM_THREAD_EXIT()            /* nothing */
#  define thread_barrier_wait();      _Pragma ("omp barrier")

#  define P_MALLOC(size)              malloc(size)
#  define P_FREE(ptr)                 free(ptr)
#  define TM_MALLOC(size)             malloc(size)
#  define TM_FREE(ptr)                /* TODO: fix memory free problem with OpenTM */

# else /* OTM */

#  include <mod_mem.h>
#  include <mod_stats.h>
#  include <stm.h>

#  define TM_CALLABLE                 /* nothing */
#  define TM_CANCELLABLE              /* nothing */
#  define TM_PURE                     /* nothing */

#  ifndef ORIGINAL
#   define TM_START(ro, o)         do { \
                                          stm_tx_attr_t _a = {{0}}; \
                                          _a.read_only = ro; \
                                          _a.no_overwrite = o; \
                                          sigjmp_buf *_e = stm_start(_a); \
                                          if (_e != NULL) sigsetjmp(*_e, 0); \
                                      } while (0)
#  else
#   define TM_START(ro, o)         do { \
                                          stm_tx_attr_t _a = {{0}}; \
                                          _a.read_only = ro; \
                                          _a.no_overwrite = o; \
                                          sigjmp_buf *_e = stm_start(_a); \
                                          if (_e != NULL) sigsetjmp(*_e, 0); \
                                      } while (0)
#  endif /* ORIGINAL */

#  define STM_BEGIN()                 TM_START(0, 0)
#  define STM_BEGIN_NOOVR()           TM_START(0, 1)
#  define TM_BEGIN_RO()               TM_START(1, 0)
#  define STM_END()                   stm_commit()
#  define TM_RESTART()                stm_abort(0)

#  define TM_EARLY_RELEASE(var)       /* nothing */

#  define STM_STARTUP(t)              TM_INIT(t, NULL)

#  define TM_INIT(numThread, p)       if (sizeof(long) != sizeof(void *)) { \
                                        fprintf(stderr, "Error: unsupported long and pointer sizes\n"); \
                                        exit(1); \
                                      } \
                                      if (getenv("STM_STATS") != NULL) \
                                        setenv("TM_STATISTICS", "1", 0); \
                                      stm_init(p); \
                                      mod_mem_init();
#  define TM_SHUTDOWN()               stm_exit()

#  define TM_THREAD_ENTER()           stm_init_thread()
#  define TM_THREAD_EXIT()            stm_exit_thread()

#  define P_MALLOC(size)              malloc(size)
#  define P_FREE(ptr)                 free(ptr)
#  define TM_MALLOC(size)             stm_malloc(size)
#  define TM_FREE(ptr)                stm_free(ptr, sizeof(*ptr))

# endif /* !OTM */

#elif defined(ITM)

# define TM_ARG                       /* nothing */
# define TM_ARG_ALONE                 /* nothing */
# define TM_ARGDECL                   /* nothing */
# define TM_ARGDECL_ALONE             /* nothing */

# ifndef TM_STARTUP
#  define TM_STARTUP(numThread)       STM_STARTUP(numThread)
# endif /* TM_STARTUP */
# ifndef TM_BEGIN
#  define TM_BEGIN()                  STM_BEGIN()
# endif /* TM_BEGIN */
# ifndef TM_BEGIN_NOOVR
#  define TM_BEGIN_NOOVR()            STM_BEGIN_NOOVR()
# endif /* TM_BEGIN_NOOVR */
# ifndef TM_END
#  define TM_END()                    STM_END()
# endif /* TM_END */

# define TM_CALLABLE                 __attribute__((transaction_safe))
# define TM_CANCELLABLE              __attribute__((transaction_may_cancel_outer))
# define TM_PURE                     __attribute__((transaction_pure))

# ifndef TM_INIT_GLOBAL
#  define TM_INIT_GLOBAL              /* nothing */
# endif /* TM_INIT_GLOBAL */
# define STM_HTM_LOCK_SET()          /* nothing */
# define STM_HTM_LOCK_UNSET()        /* nothing */

# define TM_INIT(numThread, p)       /* nothing */
# define TM_START(ro, o)          __transaction_atomic [[outer]] {

# define STM_BEGIN()                 TM_START(0, 0)
# define STM_BEGIN_NOOVR()           TM_START(0, 1)
# define TM_BEGIN_RO()               TM_START(1, 0)
# define STM_END()                   }
# define TM_RESTART()                __transaction_cancel [[outer]];

# define TM_EARLY_RELEASE(var)       /* nothing */

# define STM_STARTUP(t)              TM_INIT(t, NULL)

# define TM_SHUTDOWN()               /* nothing */

# define TM_THREAD_ENTER()           /* nothing */
# define TM_THREAD_EXIT()            /* nothing */

# define P_MALLOC(size)              malloc(size)
# define P_FREE(ptr)                 free(ptr)
# define TM_MALLOC(size)             P_MALLOC(size)
# define TM_FREE(ptr)                P_FREE(ptr)

# include "thread.h"

/* =============================================================================
 * Sequential execution
 * =============================================================================
 */

#else /* SEQUENTIAL */

# include <assert.h>
# include <stdatomic.h>

# define TM_ARG                        /* nothing */
# define TM_ARG_ALONE                  /* nothing */
# define TM_ARGDECL                    /* nothing */
# define TM_ARGDECL_ALONE              /* nothing */

# define TM_CALLABLE                   /* nothing */
# define TM_CANCELLABLE                /* nothing */
# define TM_PURE                       /* nothing */

# ifndef TM_INIT_GLOBAL
#  define TM_INIT_GLOBAL               atomic_bool lock = 0;
# endif /* TM_INIT_GLOBAL */
# define TM_STARTUP(numThread)         /* nothing */
# define TM_SHUTDOWN()                 /* nothing */

# define TM_THREAD_ENTER()             /* nothing */
# define TM_THREAD_EXIT()              /* nothing */

# define P_MALLOC(size)                malloc(size)
# define P_FREE(ptr)                   free(ptr)
# define TM_MALLOC(size)               malloc(size)
# define TM_FREE(ptr)                  free(ptr)

# ifndef TM_BEGIN
#  define TM_BEGIN()                   extern atomic_bool lock; atomic_bool zero = 0; while (!atomic_compare_exchange_weak(&lock, &zero, 1)) { zero = 0; }
# endif /* TM_BEGIN */
# define TM_BEGIN_RO()                 /* nothing */
# ifndef TM_END
#  define TM_END()                     extern atomic_bool lock; atomic_store(&lock, 0)
# endif /* TM_END */
# ifndef TM_BEGIN_NOOVR
#  define TM_BEGIN_NOOVR()             extern atomic_bool lock; atomic_bool zero = 0; while (!atomic_compare_exchange_weak(&lock, &zero, 1)) { zero = 0; }
# endif /* TM_BEGIN_NOOVR */
# define TM_RESTART()                  abort()

# define TM_EARLY_RELEASE(var)         /* nothing */

#endif /* SEQUENTIAL */


/* =============================================================================
 * Transactional Memory System interface for shared memory accesses
 *
 * There are 3 flavors of each function:
 *
 * 1) no suffix: for accessing variables of size "long"
 * 2) _P suffix: for accessing variables of type "pointer"
 * 3) _F suffix: for accessing variables of type "float"
 * =============================================================================
 */
#if defined(STM)

# ifdef OTM

#  define TM_SHARED_READ(var)           (var)
#  define TM_SHARED_READ_P(var)         (var)
#  define TM_SHARED_READ_F(var)         (var)

#  define TM_SHARED_WRITE(var, val)     ({var = val; var;})
#  define TM_SHARED_WRITE_P(var, val)   ({var = val; var;})
#  define TM_SHARED_WRITE_F(var, val)   ({var = val; var;})

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})

# else /* OTM */

#  include <wrappers.h>

/* We could also map macros to the stm_(load|store)_long functions if needed */

#  define TM_SHARED_WRITE(var, val)         stm_store((stm_word_t *)(void *)&(var), (stm_word_t)val)
#  define TM_SHARED_WRITE_P(var, val)       stm_store_ptr((void **)(void *)&(var), val)
#  define TM_SHARED_WRITE_F(var, val)       stm_store_float((float *)(void *)&(var), val)
#  define TM_SHARED_WRITE_D(var, val)       stm_store_double((double *)(void *)&(var), val)

#  define TM_LOCAL_WRITE(var, val)          ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)        ({var = val; var;})
#  define TM_LOCAL_WRITE_F(var, val)        ({var = val; var;})

#  ifndef ORIGINAL
#   define TM_LOG_OP_DECLARE(o, f, r, a, m, ...) stm_op_id_t o = STM_INVALID_OPID
#   define TM_FINISH_MERGE()                stm_finish_merge()
#   define TM_LOG_FFI_DECLARE               ffi_cif fi
#   define TM_LOG_TYPE_DECLARE_INIT(v, ...) static ffi_type v = __VA_ARGS__
#   define TM_LOG_OP_INIT(o, f, r, a, m, ...) const stm_merge_policy_t o##_policy[2] = { __VA_ARGS__ }; const ffi_status o##_ffi = ffi_prep_cif(&fi, FFI_DEFAULT_ABI, !__builtin_types_compatible_p(typeof(a), typeof(&a[0])) ? sizeof(a) / sizeof(a[0]) : 0, r, a); ASSERT_FAIL(o##_ffi == FFI_OK); o = stm_new_opcode(#o, &fi, FFI_FN(f), m, o##_policy); ASSERT_FAIL(STM_VALID_OPID(o))
#   ifndef NDEBUG
#    define TM_LOG_BEGIN(o, m, ...)         int o##_ret = stm_begin_op(o, m, ##__VA_ARGS__); assert(o##_ret)
#    define TM_LOG_END(o, rv)               o##_ret = stm_end_op(o, rv); assert(o##_ret)
#   else
#    define TM_LOG_BEGIN(o, m, ...)         stm_begin_op(o, m, ##__VA_ARGS__)
#    define TM_LOG_END(o, rv)               stm_end_op(o, rv)
#   endif /* NDEBUG */

#   ifdef STM_HTM
#    include <immintrin.h>
#    include <stdatomic.h>

#    define HTM_STATS(status)               atomic_ullong status[9] = { 0 }
#    define HTM_STATS_EXTERN(status)        extern atomic_ullong status[9]
#    define HTM_STATS_PRINT(status)         HTM_STATS_EXTERN(status); printf("UNKNOWN: %llu\n_XABORT_EXPLICIT: %llu\n_XABORT_RETRY: %llu\n_XABORT_CONFLICT: %llu\n_XABORT_CAPACITY: %llu\n_XABORT_DEBUG: %llu\n_XABORT_NESTED: %llu\n_XBEGIN_STARTED: %llu\n_XBEGIN_FAILED: %llu\n", status[0], status[1], status[2], status[3], status[4], status[5], status[6], status[7], status[8])

#    ifdef HTM_RETRY_ABLE
#     define HTM_RETRY(local, d)            if (local & _XABORT_RETRY) goto d
#    else
#     define HTM_RETRY(local, d)            /* nothing */
#    endif /* HTM_RETRY_ABLE */

#    define STM_HTM_BEGIN()                 (tsx_counter++ <= STM_HTM_RETRY_COUNTER)
#    define STM_HTM_EXIT()                  /* nothing */
#    define STM_HTM_START(local, global)    local = _xbegin(); if (local != _XBEGIN_STARTED) { atomic_fetch_add(&global[8], 1); if (!local) atomic_fetch_add(&global[0], 1); else { if (local & _XABORT_EXPLICIT) atomic_fetch_add(&global[1], 1); if (local & _XABORT_RETRY) atomic_fetch_add(&global[2], 1); if (local & _XABORT_CONFLICT) atomic_fetch_add(&global[3], 1); if (local & _XABORT_CAPACITY) atomic_fetch_add(&global[4], 1); if (local & _XABORT_DEBUG) atomic_fetch_add(&global[5], 1); if (local & _XABORT_NESTED) atomic_fetch_add(&global[6], 1); } } if (local & _XABORT_CONFLICT) stm_revalidate()
#    define STM_HTM_STARTED(status)         (status == _XBEGIN_STARTED)

#    ifdef STM_HTM_RETRY_GLOBAL
#     define STM_HTM_GLOBAL_INIT            unsigned tsx_counter = 0
#     define STM_HTM_TX_INIT                unsigned tsx_status;
#    elif defined (STM_HTM_RETRY_TX)
#     define STM_HTM_GLOBAL_INIT            /* nothing */
#     define STM_HTM_TX_INIT                unsigned tsx_status; static unsigned tsx_counter = 0
#    else
#     error "STM_HTM requires STM_HTM_RETRY_GLOBAL or STM_HTM_RETRY_TX"
#    endif

#    ifdef HTM_RETRY_ABLE
#     define HTM_RETRY(local, d)            if (local & _XABORT_RETRY) goto d
#    else
#     define HTM_RETRY(local, d)            /* nothing */
#    endif /* HTM_RETRY_ABLE */

#    define TM_INIT_GLOBAL                  STM_HTM_GLOBAL_INIT
#    define STM_HTM_LOCK_READ()             /* nothing */
#    define STM_HTM_LOCK_SET()              /* nothing */
#    define STM_HTM_LOCK_UNSET()            /* nothing */

#    ifdef STM_HTM_STM
/* Executes optimized STM inside HTM, with lazy subscription */
#     define STM_HTM_END(global)            hytm_load_locks(); _xend(); atomic_fetch_add(&global[7], 1)

#     define HTM_SHARED_WRITE(var, val)     TM_SHARED_WRITE(var, val)
#     define HTM_SHARED_WRITE_P(var, val)   TM_SHARED_WRITE_P(var, val)
#     define HTM_SHARED_WRITE_F(var, val)   TM_SHARED_WRITE_F(var, val)

#     define HTM_SHARED_READ(var)           hytm_load((const stm_word_t *)(void *)&(var))
#     define HTM_SHARED_READ_P(var)         hytm_load_ptr((const void **)(void *)&(var))
#     define HTM_SHARED_READ_F(var)         hytm_load_float((const float *)(void *)&(var))

#    elif defined(STM_HTM_DIRECT)
/* Executes directly in HTM, not correct */
#     define STM_HTM_END(global)            _xend(); atomic_fetch_add(&global[7], 1)

#     define HTM_SHARED_WRITE(var, val)     ({var = val; var;})
#     define HTM_SHARED_WRITE_P(var, val)   ({var = val; var;})
#     define HTM_SHARED_WRITE_F(var, val)   ({var = val; var;})

#     define HTM_SHARED_READ(var)           (var)
#     define HTM_SHARED_READ_P(var)         (var)
#     define HTM_SHARED_READ_F(var)         (var)

#    else
#     error "STM_HTM requires STM_HTM_STM or STM_HTM_DIRECT!"
#    endif
#   else
#    ifndef TM_INIT_GLOBAL
#     define TM_INIT_GLOBAL                 /* nothing */
#    endif /* TM_INIT_GLOBAL */
#    define STM_HTM_LOCK_READ()             /* nothing */
#    define STM_HTM_LOCK_SET()              /* nothing */
#    define STM_HTM_LOCK_UNSET()            /* nothing */
#   endif /* STM_HTM */

#   define TM_SHARED_READ(var)              stm_load((const stm_word_t *)(void *)&(var))
#   define TM_SHARED_READ_P(var)            stm_load_ptr((const void **)(void *)&(var))
#   define TM_SHARED_READ_F(var)            stm_load_float((const float *)(void *)&(var))
#   define TM_SHARED_READ_D(var)            stm_load_double((const double *)(void *)&(var))

#   define TM_SHARED_READ_TAG(var, tag)     stm_load_tag((const stm_word_t *)(void *)&(var), (uintptr_t)tag)
#   define TM_SHARED_READ_TAG_P(var, tag)   stm_load_tag_ptr((const void **)(void *)&(var), (uintptr_t)tag)
#   define TM_SHARED_READ_TAG_F(var, tag)   stm_load_tag_float((const float *)(void *)&(var), (uintptr_t)tag)
#   define TM_SHARED_READ_TAG_D(var, tag)   stm_load_tag_double((const double *)(void *)&(var), (uintptr_t)tag)

#   define TM_SHARED_READ_UPDATE(r, var, val)     stm_load_update(r, (stm_word_t *)&val)
#   define TM_SHARED_READ_UPDATE_P(r, var, val)   stm_load_update_ptr(r, (void *)&val)
#   define TM_SHARED_READ_UPDATE_F(r, var, val)   stm_load_update_float(r, &val)
#   define TM_SHARED_READ_UPDATE_D(r, var, val)   stm_load_update_double(r, &val)

#   define TM_SHARED_WRITE_UPDATE(w, var, val)    stm_store_update(w, val)
#   define TM_SHARED_WRITE_UPDATE_P(w, var, val)  stm_store_update_ptr(w, val)
#   define TM_SHARED_WRITE_UPDATE_F(w, var, val)  stm_store_update_float(w, val)
#   define TM_SHARED_WRITE_UPDATE_D(w, var, val)  stm_store_update_double(w, val)

#   define TM_SHARED_READ_VALUE(r, var, val)      stm_load_value(r, (stm_word_t *)&val)
#   define TM_SHARED_READ_VALUE_P(r, var, val)    stm_load_value_ptr(r, (void *)&val)
#   define TM_SHARED_READ_VALUE_F(r, var, val)    stm_load_value_float(r, &val)
#   define TM_SHARED_READ_VALUE_D(r, var, val)    stm_load_value_double(r, &val)

#   define TM_SHARED_WRITE_VALUE(w, var, val)     stm_store_value(w, (stm_word_t *)&val)
#   define TM_SHARED_WRITE_VALUE_P(w, var, val)   stm_store_value_ptr(w, (void *)&val)
#   define TM_SHARED_WRITE_VALUE_F(w, var, val)   stm_store_value_float(w, &val)
#   define TM_SHARED_WRITE_VALUE_D(w, var, val)   stm_store_value_double(w, &val)

#   define TM_SHARED_GET_TAG(r)             (void *)stm_get_load_tag(r)
#   define TM_SHARED_SET_TAG(r, tag)        stm_set_load_tag(r, (uintptr_t)tag)

#   define TM_SHARED_DID_READ(var)          stm_did_load((stm_word_t *)&(var))
#   define TM_SHARED_DID_WRITE(var)         stm_did_store((stm_word_t *)&(var))
#   define TM_SHARED_UNDO_READ(e)           stm_undo_load(e)
#   define TM_SHARED_UNDO_WRITE(e)          stm_undo_store(e)

#   define TM_DID_MALLOC(var)               stm_did_malloc((const void *)var)
#   define TM_DID_FREE(var)                 stm_did_free((const void *)var)
#   define TM_UNDO_MALLOC(e)                stm_undo_malloc(e)
#   define TM_UNDO_FREE(e)                  stm_undo_free(e)
#  else
#   define TM_LOG_FFI_DECLARE               /* nothing */
#   define TM_LOG_TYPE_DECLARE_INIT(v, ...) /* nothing */
#   define TM_LOG_OP_DECLARE(o, f, r, a, m, ...) /* nothing */
#   define TM_LOG_OP_INIT(o, f, r, a, m, ...) /* nothing */
#   define TM_LOG_BEGIN(o, m, ...)          /* nothing */
#   define TM_LOG_END(o, rv)                /* nothing */

#   define TM_FINISH_MERGE()                /* nothing */

#   define TM_SHARED_READ(var)              stm_load((const stm_word_t *)(void *)&(var))
#   define TM_SHARED_READ_P(var)            stm_load_ptr((const void **)(void *)&(var))
#   define TM_SHARED_READ_F(var)            stm_load_float((const float *)(void *)&(var))
#   define TM_SHARED_READ_D(var)            stm_load_double((const double *)(void *)&(var))

#   define TM_SHARED_READ_TAG(var, base)    TM_SHARED_READ(var)
#   define TM_SHARED_READ_TAG_P(var, base)  TM_SHARED_READ_P(var)
#   define TM_SHARED_READ_TAG_F(var, base)  TM_SHARED_READ_F(var)

#   define STM_HTM_LOCK_READ()              /* nothing */
#   define STM_HTM_LOCK_SET()               /* nothing */
#   define STM_HTM_LOCK_UNSET()             /* nothing */
#  endif /* ORIGINAL */

# endif /* !OTM */

#else /* !STM */

# define TM_LOG_FFI_DECLARE               /* nothing */
# define TM_LOG_TYPE_DECLARE_INIT(v, ...) /* nothing */
# define TM_LOG_OP_DECLARE(o, f, r, a, m, ...) /* nothing */
# define TM_LOG_OP_INIT(o, f, r, a, m, ...) /* nothing */
# define TM_LOG_BEGIN(o, m, ...)          /* nothing */
# define TM_LOG_END(o, rv)                /* nothing */

# define TM_FINISH_MERGE()                /* nothing */

# define TM_SHARED_READ(var)              (var)
# define TM_SHARED_READ_P(var)            (var)
# define TM_SHARED_READ_F(var)            (var)

# define TM_SHARED_READ_TAG(var, base)    TM_SHARED_READ(var)
# define TM_SHARED_READ_TAG_P(var, base)  TM_SHARED_READ_P(var)
# define TM_SHARED_READ_TAG_F(var, base)  TM_SHARED_READ_F(var)

# define TM_SHARED_WRITE(var, val)        ({var = val; var;})
# define TM_SHARED_WRITE_P(var, val)      ({var = val; var;})
# define TM_SHARED_WRITE_F(var, val)      ({var = val; var;})

# define TM_SHARED_READ_UPDATE(r, var, val)     /* nothing */
# define TM_SHARED_READ_UPDATE_P(r, var, val)   /* nothing */
# define TM_SHARED_READ_UPDATE_F(r, var, val)   /* nothing */

# define TM_SHARED_WRITE_UPDATE(w, var, val)    /* nothing */
# define TM_SHARED_WRITE_UPDATE_P(w, var, val)  /* nothing */
# define TM_SHARED_WRITE_UPDATE_F(w, var, val)  /* nothing */

# define TM_SHARED_READ_VALUE(r, var, val)      /* nothing */
# define TM_SHARED_READ_VALUE_P(r, var, val)    /* nothing */
# define TM_SHARED_READ_VALUE_F(r, var, val)    /* nothing */

# define TM_SHARED_WRITE_VALUE(r, var, val)     /* nothing */
# define TM_SHARED_WRITE_VALUE_P(r, var, val)   /* nothing */
# define TM_SHARED_WRITE_VALUE_F(r, var, val)   /* nothing */

# define TM_SHARED_GET_TAG(r)             /* nothing */
# define TM_SHARED_SET_TAG(r, tag)        /* nothing */

# define TM_SHARED_DID_READ(addr)         /* nothing */
# define TM_SHARED_DID_WRITE(addr)        /* nothing */
# define TM_SHARED_UNDO_READ(e)           /* nothing */
# define TM_SHARED_UNDO_WRITE(e)          /* nothing */

# define TM_DID_MALLOC(addr)              /* nothing */
# define TM_DID_FREE(addr)                /* nothing */
# define TM_UNDO_MALLOC(e)                /* nothing */
# define TM_UNDO_FREE(e)                  /* nothing */

# define TM_LOCAL_WRITE(var, val)         ({var = val; var;})
# define TM_LOCAL_WRITE_P(var, val)       ({var = val; var;})
# define TM_LOCAL_WRITE_F(var, val)       ({var = val; var;})

# define STM_HTM_LOCK_READ()              /* nothing */
# define STM_HTM_LOCK_SET()               /* nothing */
# define STM_HTM_LOCK_UNSET()             /* nothing */

#endif /* !STM */

#ifdef NDEBUG
# define ASSERT_FAIL(x)                    if (__builtin_expect(!(x), 0)) abort()
# define ASSERT(x)                         do { (void)(x); } while (0)
#else
# define ASSERT_FAIL(x)                    assert(x)
# define ASSERT(x)                         assert(x)
#endif


#endif /* TM_H */

/* =============================================================================
 *
 * End of tm.h
 *
 * =============================================================================
 */
