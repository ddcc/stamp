/* =============================================================================
 *
 * normal.c
 * -- Implementation of normal k-means clustering algorithm
 *
 * =============================================================================
 *
 * Author:
 *
 * Wei-keng Liao
 * ECE Department, Northwestern University
 * email: wkliao@ece.northwestern.edu
 *
 *
 * Edited by:
 *
 * Jay Pisharath
 * Northwestern University.
 *
 * Chi Cao Minh
 * Stanford University
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


#include <assert.h>
#include <float.h>
#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "normal.h"
#include "random.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"
#include "util.h"

#if !defined(ORIGINAL) && defined(MERGE_NORMAL)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "normal.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_NORMAL */

double global_time = 0.0;

typedef struct {
#ifdef __x86_64__
    float f;
    float p;
#else
    float f;
#endif
} padded_float_t;

typedef struct args {
    float** feature;
    int     nfeatures;
    int     npoints;
    int     nclusters;
    int*    membership;
    float** clusters;
    long**   new_centers_len;
    padded_float_t** new_centers;
} args_t;

padded_float_t global_delta;
long global_i; /* index into task queue */

#define CHUNK 3

TM_INIT_GLOBAL;

HTM_STATS_EXTERN(global_tsx_status);

/* =============================================================================
 * work
 * =============================================================================
 */
static void
work (void* argPtr)
{
    __label__ tsx_begin_queue;

    TM_THREAD_ENTER();

    args_t* args = (args_t*)argPtr;
    float** feature         = args->feature;
    int     nfeatures       = args->nfeatures;
    int     npoints         = args->npoints;
    int     nclusters       = args->nclusters;
    int*    membership      = args->membership;
    float** clusters        = args->clusters;
    long**   new_centers_len = args->new_centers_len;
    padded_float_t** new_centers     = args->new_centers;
    padded_float_t delta = { .f = 0.0 };
    int index;
    int i;
    int j;
    int start;
    int stop;
    int myId;

#if !defined(ORIGINAL) && defined(MERGE_NORMAL)
    stm_merge_t merge(stm_merge_context_t *params) {
#ifdef TM_DEBUG
        printf("\nNORMAL_JIT addr:%p start:%i stop:%i i:%i j:%i\n", params->addr, start, stop, i, j);
#endif

        ASSERT(params->conflict.entries->type == STM_RD_VALIDATE || params->conflict.entries->type == STM_WR_VALIDATE);
        ASSERT(ENTRY_VALID(params->conflict.entries->e1));
        stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);

        if ((uintptr_t)params->addr == (uintptr_t)new_centers_len[index]) {
            /* Conflict is on on *new_centers_len[] */
            ASSERT((uintptr_t)params->addr >= (uintptr_t)new_centers_len[0] && (uintptr_t) params->addr < (uintptr_t)new_centers[nclusters - 1] + sizeof(padded_float_t) * nfeatures);
            /* Read the old and new values */
            ASSERT(STM_SAME_READ(r, TM_SHARED_DID_READ(*new_centers_len[index])));
            long old, new;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, *new_centers_len[index], old));
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, *new_centers_len[index], new));
            if (old == new)
                return STM_MERGE_OK;

            const stm_write_t w = TM_SHARED_DID_WRITE(*new_centers_len[index]);
            ASSERT_FAIL(STM_VALID_WRITE(w));
            /* Update the write */
            ASSERT((stm_get_features() & STM_FEATURE_OPLOG_FULL) != STM_FEATURE_OPLOG_FULL || STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
            /* Increment the length */
            ASSERT(STM_SAME_WRITE(w, TM_SHARED_DID_WRITE(*new_centers_len[index])));
            ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, *new_centers_len[index], new + 1));
#ifdef TM_DEBUG
            printf("NORMAL_JIT *new_centers_len[%i] read (old):%li (new):%li write (new):%li\n", index, old, new, new + 1);
#endif
            return STM_MERGE_OK;
        } else if ((uintptr_t)params->addr >= (uintptr_t)new_centers[index] && (uintptr_t) params->addr < (uintptr_t)new_centers[index] + sizeof(padded_float_t) * nfeatures) {
            /* Get the index of the conflict on alloc_memory[] */
            int f = (padded_float_t *)params->addr - new_centers[index];

            /* Conflict is on new_centers[][] */
            /* Read the old and new values */
            if (!f) {
                for (; f < nfeatures; ++f, r = stm_get_load_next(r, 1, 0)) {
                    float old, new;
                    if (!STM_VALID_READ(r))
                        break;
                    ASSERT_FAIL(stm_get_load_addr(r) == (const volatile stm_word_t *)&new_centers[index][f].f);
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_F(r, new_centers[index][f].f, old));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_F(r, new_centers[index][f].f, new));
                    if (old == new)
                        continue;

                    const stm_write_t w = TM_SHARED_DID_WRITE(new_centers[index][f]);
                    ASSERT_FAIL(STM_VALID_WRITE(w));
                    /* Update the write */
                    ASSERT((stm_get_features() & STM_FEATURE_OPLOG_FULL) != STM_FEATURE_OPLOG_FULL || STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
                    const float *feature = (float *)TM_SHARED_GET_TAG(r);
                    ASSERT_FAIL(feature);
                    /* Increment by the feature value feature[i][j]. Since this parameter can be negative, it is not monotonic. */
                    ASSERT(STM_SAME_WRITE(w, TM_SHARED_DID_WRITE(new_centers[index][f].f)));
                    ASSERT_FAIL(TM_SHARED_WRITE_UPDATE_F(w, new_centers[index][f].f, (double)new + (double)*feature));
#ifdef TM_DEBUG
                    printf("NORMAL_JIT new_centers[%i][%i] read (old):%f (new):%f, write (new):%f\n", index, f, old, new, new + *feature);
#endif
                }
            } else {
                ASSERT(STM_SAME_READ(r, TM_SHARED_DID_READ(new_centers[index][f].f)));
                float old, new;
                ASSERT_FAIL(TM_SHARED_READ_VALUE_F(r, new_centers[index][f].f, old));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_F(r, new_centers[index][f].f, new));
                if (old == new)
                    return STM_MERGE_OK;

                const stm_write_t w = TM_SHARED_DID_WRITE(new_centers[index][f]);
                ASSERT_FAIL(STM_VALID_WRITE(w));
                /* Update the write */
                ASSERT((stm_get_features() & STM_FEATURE_OPLOG_FULL) != STM_FEATURE_OPLOG_FULL || STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
                const float *feature = (float *)TM_SHARED_GET_TAG(r);
                ASSERT_FAIL(feature);
                /* Increment by the feature value feature[i][j]. Since this parameter can be negative, it is not monotonic. */
                ASSERT(STM_SAME_WRITE(w, TM_SHARED_DID_WRITE(new_centers[index][f].f)));
                ASSERT_FAIL(TM_SHARED_WRITE_UPDATE_F(w, new_centers[index][f].f, (double)new + (double)*feature));
#ifdef TM_DEBUG
                printf("NORMAL_JIT new_centers[%i][%i] read (old):%f (new):%f, write (new):%f\n", index, f, old, new, new + *feature);
#endif
            }

            return STM_MERGE_OK;
        } else if (params->addr == &global_i) {
            /* Read the old and new values */
            ASSERT(STM_SAME_READ(r, TM_SHARED_DID_READ(global_i)));
            long old, new;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, global_i, old));
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, global_i, new));
            if (old == new)
                return STM_MERGE_OK;

            start = new;
            if (new + CHUNK >= npoints) {
                TM_FINISH_MERGE();
                stm_stop(STM_ABORT_EXPLICIT);
                goto tsx_begin_queue;
            }

            const stm_write_t w = TM_SHARED_DID_WRITE(global_i);
            ASSERT_FAIL(STM_VALID_WRITE(w));
            /* Update the write */
            ASSERT((stm_get_features() & STM_FEATURE_OPLOG_FULL) != STM_FEATURE_OPLOG_FULL || STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
            /* Increment by the chunk size. */
            ASSERT(STM_SAME_WRITE(w, TM_SHARED_DID_WRITE(global_i)));
            ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, global_i, new + CHUNK));
#ifdef TM_DEBUG
            printf("NORMAL_JIT global_i read (old):%li (new):%li, write (new):%li\n", old, new, new + CHUNK);
#endif
            return STM_MERGE_OK;
        } else if (params->addr == &global_delta.f) {
            /* Read the old and new values */
            ASSERT(STM_SAME_READ(r, TM_SHARED_DID_READ(global_delta.f)));
            float old, new;
            ASSERT_FAIL(TM_SHARED_READ_VALUE_F(r, global_delta.f, old));
            ASSERT_FAIL(TM_SHARED_READ_UPDATE_F(r, global_delta.f, new));
            if (old == new)
                return STM_MERGE_OK;

            const stm_write_t w = TM_SHARED_DID_WRITE(global_delta.f);
            ASSERT_FAIL(STM_VALID_WRITE(w));
            /* Update the write */
            ASSERT((stm_get_features() & STM_FEATURE_OPLOG_FULL) != STM_FEATURE_OPLOG_FULL || STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
            /* Increment by the local delta. */
            ASSERT(STM_SAME_WRITE(w, TM_SHARED_DID_WRITE(global_delta.f)));
            ASSERT_FAIL(TM_SHARED_WRITE_UPDATE_F(w, global_delta.f, (double)new + (double)delta.f));
#ifdef TM_DEBUG
            printf("NORMAL_JIT global_delta read (old):%f (new):%f, write (new):%f\n", old, new, new + delta.f);
#endif
            return STM_MERGE_OK;
        }

# ifdef TM_DEBUG
        printf("\nNORMAL_JIT UNSUPPORTED addr:%p\n", params->addr);
# endif
        return STM_MERGE_UNSUPPORTED;
    }
#endif /* !ORIGINAL && MERGE_NORMAL */

    myId = thread_getId();

    start = myId * CHUNK;

    while (start < npoints) {
        stop = (((start + CHUNK) < npoints) ? (start + CHUNK) : npoints);
        for (i = start; i < stop; i++) {

            index = common_findNearestPoint(feature[i],
                                            nfeatures,
                                            clusters,
                                            nclusters);
            /*
             * If membership changes, increase delta by 1.
             * membership[i] cannot be changed by other threads
             */
            if (membership[i] != index) {
                delta.f += 1.0;
            }

            /* Assign the membership to object i */
            /* membership[i] can't be changed by other thread */
            membership[i] = index;

            /* Update new cluster centers : sum of objects located within */
            HTM_TX_INIT;
tsx_begin_center:
            if (HTM_BEGIN(tsx_status, global_tsx_status)) {
                HTM_LOCK_READ();
                HTM_SHARED_WRITE(*new_centers_len[index],
                                HTM_SHARED_READ(*new_centers_len[index]) + 1);
                for (j = 0; j < nfeatures; j++) {
                    HTM_SHARED_WRITE_F(
                        new_centers[index][j].f,
                        (double)HTM_SHARED_READ_F(new_centers[index][j].f) + (double)feature[i][j]
                    );
                }
                HTM_END(global_tsx_status);
            } else {
                HTM_RETRY(tsx_status, tsx_begin_center);

                TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_NORMAL)
                TM_LOG_BEGIN(NORMAL, merge);
#endif /* !ORIGINAL && MERGE_NORMAL */
                TM_SHARED_WRITE(*new_centers_len[index],
                                TM_SHARED_READ(*new_centers_len[index]) + 1);
                for (j = 0; j < nfeatures; j++) {
                    TM_SHARED_WRITE_F(
                        new_centers[index][j].f,
                        (double)TM_SHARED_READ_TAG_F(new_centers[index][j].f, (uintptr_t)&feature[i][j]) + (double)feature[i][j]
                    );
                }
                /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
                // TM_LOG_END(NORMAL, NULL);
                TM_END();
            }
        }

        /* Update task queue */
        if (start + CHUNK < npoints) {
            HTM_TX_INIT;
tsx_begin_queue:
            if (HTM_BEGIN(tsx_status, global_tsx_status)) {
                HTM_LOCK_READ();
                start = (int)HTM_SHARED_READ(global_i);
                HTM_SHARED_WRITE(global_i, start + CHUNK);
                HTM_END(global_tsx_status);
            } else {
                HTM_RETRY(tsx_status, tsx_begin_queue);

                TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_NORMAL)
                TM_LOG_BEGIN(NORMAL, merge);
#endif /* !ORIGINAL && MERGE_NORMAL */
                start = (int)TM_SHARED_READ(global_i);
                TM_SHARED_WRITE(global_i, start + CHUNK);
                /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
                // TM_LOG_END(NORMAL, NULL);
                TM_END();
            }
        } else {
            break;
        }
    }

    HTM_TX_INIT;
tsx_begin_delta:
    if (HTM_BEGIN(tsx_status, global_tsx_status)) {
        HTM_LOCK_READ();
        HTM_SHARED_WRITE_F(global_delta.f, (double)HTM_SHARED_READ_F(global_delta.f) + (double)delta.f);
        HTM_END(global_tsx_status);
    } else {
        HTM_RETRY(tsx_status, tsx_begin_delta);

        TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_NORMAL)
        TM_LOG_BEGIN(NORMAL, merge);
#endif /* !ORIGINAL && MERGE_NORMAL */
        TM_SHARED_WRITE_F(global_delta.f, (double)TM_SHARED_READ_F(global_delta.f) + (double)delta.f);
        /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
        // TM_LOG_END(NORMAL, NULL);
        TM_END();
    }

    TM_THREAD_EXIT();
}


/* =============================================================================
 * normal_exec
 * =============================================================================
 */
float**
normal_exec (int       nthreads,
             float**   feature,    /* in: [npoints][nfeatures] */
             int       nfeatures,
             int       npoints,
             int       nclusters,
             float     threshold,
             int*      membership,
             random_t* randomPtr) /* out: [npoints] */
{
    int i;
    int j;
    int loop = 0;
    long** new_centers_len; /* [nclusters]: no. of points in each cluster */
    float delta;
    float** clusters;      /* out: [nclusters][nfeatures] */
    padded_float_t** new_centers;   /* [nclusters][nfeatures] */
    void* alloc_memory = NULL;
    args_t args;
    TIMER_T start;
    TIMER_T stop;

    /* Allocate space for returning variable clusters[] */
    clusters = (float**)malloc(nclusters * sizeof(float*));
    assert(clusters);
    clusters[0] = (float*)malloc(nclusters * nfeatures * sizeof(float));
    assert(clusters[0]);
    for (i = 1; i < nclusters; i++) {
        clusters[i] = clusters[i-1] + nfeatures;
    }

    /* Randomly pick cluster centers */
    for (i = 0; i < nclusters; i++) {
        int n = (int)(random_generate(randomPtr) % npoints);
        for (j = 0; j < nfeatures; j++) {
            clusters[i][j] = feature[n][j];
        }
    }

    for (i = 0; i < npoints; i++) {
        membership[i] = -1;
    }

    /*
     * Need to initialize new_centers_len and new_centers[0] to all 0.
     * Allocate clusters on different cache lines to reduce false sharing.
     */
    {
        int cluster_size = sizeof(long) + sizeof(padded_float_t) * nfeatures;
        const int cacheLineSize = 32;
        cluster_size += (cacheLineSize-1) - ((cluster_size-1) % cacheLineSize);
        alloc_memory = calloc(nclusters, cluster_size);
        new_centers_len = (long**) malloc(nclusters * sizeof(long*));
        new_centers = (padded_float_t**) malloc(nclusters * sizeof(padded_float_t*));
        assert(alloc_memory && new_centers && new_centers_len);
        for (i = 0; i < nclusters; i++) {
            new_centers_len[i] = (long*)((char*)alloc_memory + cluster_size * i);
            new_centers[i] = (padded_float_t*)((char*)alloc_memory + cluster_size * i + sizeof(long));
        }
    }

    TIMER_READ(start);

    GOTO_SIM();

    do {
        delta = 0.0;

        args.feature         = feature;
        args.nfeatures       = nfeatures;
        args.npoints         = npoints;
        args.nclusters       = nclusters;
        args.membership      = membership;
        args.clusters        = clusters;
        args.new_centers_len = new_centers_len;
        args.new_centers     = new_centers;

        global_i = nthreads * CHUNK;
        global_delta.f = delta;

#ifdef OTM
#pragma omp parallel
        {
            work(&args);
        }
#else
        thread_start(work, &args);
#endif

        delta = global_delta.f;

        /* Replace old cluster centers with new_centers */
        for (i = 0; i < nclusters; i++) {
            for (j = 0; j < nfeatures; j++) {
                if (new_centers_len[i] > 0) {
                    clusters[i][j] = new_centers[i][j].f / *new_centers_len[i];
                }
                new_centers[i][j].f = 0.0;   /* set back to 0 */
            }
            *new_centers_len[i] = 0;   /* set back to 0 */
        }

        delta /= npoints;

    } while ((delta > threshold) && (loop++ < 500));

    GOTO_REAL();

    TIMER_READ(stop);
    global_time += TIMER_DIFF_SECONDS(start, stop);

    free(alloc_memory);
    free(new_centers);
    free(new_centers_len);

    return clusters;
}


/* =============================================================================
 *
 * End of normal.c
 *
 * =============================================================================
 */

#if !defined(ORIGINAL) && defined(MERGE_NORMAL)
__attribute__((constructor)) void normal_init() {
    TM_LOG_FFI_DECLARE;
    #define TM_LOG_OP TM_LOG_OP_INIT
    #include "normal.inc"
    #undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_NORMAL */
