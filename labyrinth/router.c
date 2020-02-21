/* =============================================================================
 *
 * router.c
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


#include <assert.h>
#include <stdlib.h>
#include "coordinate.h"
#include "grid.h"
#include "queue.h"
#include "router.h"
#include "tm.h"
#include "vector.h"


typedef enum momentum {
    MOMENTUM_ZERO = 0,
    MOMENTUM_POSX = 1,
    MOMENTUM_POSY = 2,
    MOMENTUM_POSZ = 3,
    MOMENTUM_NEGX = 4,
    MOMENTUM_NEGY = 5,
    MOMENTUM_NEGZ = 6
} momentum_t;

typedef struct point {
    long x;
    long y;
    long z;
    long value;
    momentum_t momentum;
} point_t;

point_t MOVE_POSX = { 1,  0,  0,  0, MOMENTUM_POSX};
point_t MOVE_POSY = { 0,  1,  0,  0, MOMENTUM_POSY};
point_t MOVE_POSZ = { 0,  0,  1,  0, MOMENTUM_POSZ};
point_t MOVE_NEGX = {-1,  0,  0,  0, MOMENTUM_NEGX};
point_t MOVE_NEGY = { 0, -1,  0,  0, MOMENTUM_NEGY};
point_t MOVE_NEGZ = { 0,  0, -1,  0, MOMENTUM_NEGZ};


#if !defined(ORIGINAL) && defined(MERGE_ROUTER)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "router.inc"
# undef TM_LOG_OP

stm_merge_t TMrouter_merge(stm_merge_context_t *params);
#endif /* !ORIGINAL && MERGE_ROUTER */

TM_INIT_GLOBAL;


HTM_STATS_EXTERN(global_tsx_status);


/* =============================================================================
 * router_alloc
 * =============================================================================
 */
router_t*
router_alloc (long xCost, long yCost, long zCost, long bendCost)
{
    router_t* routerPtr;

    routerPtr = (router_t*)malloc(sizeof(router_t));
    if (routerPtr) {
        routerPtr->xCost = xCost;
        routerPtr->yCost = yCost;
        routerPtr->zCost = zCost;
        routerPtr->bendCost = bendCost;
    }

    return routerPtr;
}


/* =============================================================================
 * router_free
 * =============================================================================
 */
void
router_free (router_t* routerPtr)
{
    free(routerPtr);
}


/* =============================================================================
 * HTMPexpandToNeighbor
 * =============================================================================
 */
static void
HTMPexpandToNeighbor (TM_ARG  grid_t* myGridPtr,
                   long x, long y, long z, long value, queue_t* queuePtr)
{
    if (grid_isPointValid(myGridPtr, x, y, z)) {
        long *neighborGridPointPtr = grid_getPointRef(myGridPtr, x, y, z);
#ifdef GRID_COPY
        long neighborValue = *neighborGridPointPtr;
#else
        long *globalNeighborGridPointPtr = grid_getPointRef(gridPtr, x, y, z);
        long neighborValue = HTM_SHARED_READ(*globalNeighborGridPointPtr);
#endif /* GRID_COPY */
        if (neighborValue == GRID_POINT_EMPTY) {
            (*neighborGridPointPtr) = value;
            PQUEUE_PUSH(queuePtr, (void*)neighborGridPointPtr);
        } else if (neighborValue != GRID_POINT_FULL) {
            /* We have expanded here before... is this new path better? */
            if (value < neighborValue) {

                (*neighborGridPointPtr) = value;
                PQUEUE_PUSH(queuePtr, (void*)neighborGridPointPtr);
            }
        }
    }
}


/* =============================================================================
 * TMPexpandToNeighbor
 * =============================================================================
 */
TM_CALLABLE
static void
TMPexpandToNeighbor (TM_ARG  grid_t* myGridPtr,
#ifndef GRID_COPY
                   grid_t *gridPtr,
#endif /* !GRID_COPY */
                   long x, long y, long z, long value, queue_t* queuePtr)
{
#if !defined(ORIGINAL) && defined(MERGE_ROUTER) && !defined(GRID_COPY)
    TM_LOG_BEGIN(EXPAND, NULL, myGridPtr, x, y, z, value, queuePtr);
#endif /* !ORIGINAL && MERGE_ROUTER && !GRID_COPY */

    if (grid_isPointValid(myGridPtr, x, y, z)) {
        long *neighborGridPointPtr = grid_getPointRef(myGridPtr, x, y, z);
#ifdef GRID_COPY
        long neighborValue = *neighborGridPointPtr;
#else
        long *globalNeighborGridPointPtr = grid_getPointRef(gridPtr, x, y, z);
        long neighborValue = TM_SHARED_READ(*globalNeighborGridPointPtr);
#endif /* GRID_COPY */
        if (neighborValue == GRID_POINT_EMPTY) {
            (*neighborGridPointPtr) = value;
            PQUEUE_PUSH(queuePtr, (void*)neighborGridPointPtr);
        } else if (neighborValue != GRID_POINT_FULL) {
            /* We have expanded here before... is this new path better? */
            if (value < neighborValue) {
                (*neighborGridPointPtr) = value;
                PQUEUE_PUSH(queuePtr, (void*)neighborGridPointPtr);
            }
        }
    }

#if !defined(ORIGINAL) && defined(MERGE_ROUTER) && !defined(GRID_COPY)
    TM_LOG_END(EXPAND, NULL);
#endif /* !ORIGINAL && MERGE_ROUTER && !GRID_COPY */
}


/* =============================================================================
 * TMPdoExpansion
 * =============================================================================
 */
TM_CALLABLE
static bool_t
TMPdoExpansion (router_t* routerPtr, grid_t* myGridPtr,
#ifndef GRID_COPY
              grid_t* gridPtr,
#endif /* !GRID_COPY */
              queue_t* queuePtr, coordinate_t* srcPtr, coordinate_t* dstPtr)
{
    long xCost = routerPtr->xCost;
    long yCost = routerPtr->yCost;
    long zCost = routerPtr->zCost;

    /*
     * Potential Optimization: Make 'src' the one closest to edge.
     * This will likely decrease the area of the emitted wave.
     */

    PQUEUE_CLEAR(queuePtr);
    long* srcGridPointPtr =
        grid_getPointRef(myGridPtr, srcPtr->x, srcPtr->y, srcPtr->z);
    PQUEUE_PUSH(queuePtr, (void*)srcGridPointPtr);
    grid_setPoint(myGridPtr, srcPtr->x, srcPtr->y, srcPtr->z, 0);
    grid_setPoint(myGridPtr, dstPtr->x, dstPtr->y, dstPtr->z, GRID_POINT_EMPTY);
    long* dstGridPointPtr =
        grid_getPointRef(myGridPtr, dstPtr->x, dstPtr->y, dstPtr->z);
    bool_t isPathFound = FALSE;

    while (!PQUEUE_ISEMPTY(queuePtr)) {

        long* gridPointPtr = (long*)PQUEUE_POP(queuePtr);
        if (gridPointPtr == dstGridPointPtr) {
            isPathFound = TRUE;
            break;
        }

        long x;
        long y;
        long z;
        grid_getPointIndices(myGridPtr, gridPointPtr, &x, &y, &z);
        long value = (*gridPointPtr);

        /*
         * Check 6 neighbors
         *
         * Potential Optimization: Only need to check 5 of these
         */
        TMPexpandToNeighbor(myGridPtr,
#ifndef GRID_COPY
                          gridPtr,
#endif /* !USE_PRIVITIZATION */
                          x+1, y,   z,   (value + xCost), queuePtr);
        TMPexpandToNeighbor(myGridPtr,
#ifndef GRID_COPY
                          gridPtr,
#endif /* !USE_PRIVITIZATION */
                          x-1, y,   z,   (value + xCost), queuePtr);
        TMPexpandToNeighbor(myGridPtr,
#ifndef GRID_COPY
                          gridPtr,
#endif /* !USE_PRIVITIZATION */
                          x,   y+1, z,   (value + yCost), queuePtr);
        TMPexpandToNeighbor(myGridPtr,
#ifndef GRID_COPY
                          gridPtr,
#endif /* !USE_PRIVITIZATION */
                          x,   y-1, z,   (value + yCost), queuePtr);
        TMPexpandToNeighbor(myGridPtr,
#ifndef GRID_COPY
                          gridPtr,
#endif /* !USE_PRIVITIZATION */
                          x,   y,   z+1, (value + zCost), queuePtr);
        TMPexpandToNeighbor(myGridPtr,
#ifndef GRID_COPY
                          gridPtr,
#endif /* !USE_PRIVITIZATION */
                          x,   y,   z-1, (value + zCost), queuePtr);

    } /* iterate over work queue */

#if DEBUG
    printf("Expansion (%li, %li, %li) -> (%li, %li, %li):\n",
           srcPtr->x, srcPtr->y, srcPtr->z,
           dstPtr->x, dstPtr->y, dstPtr->z);
    grid_print(myGridPtr);
#endif /*  DEBUG */

    return isPathFound;
}


/* =============================================================================
 * HTMPdoExpansion
 * =============================================================================
 */
static bool_t
HTMPdoExpansion (TM_ARG   router_t* routerPtr, grid_t* myGridPtr,
#ifndef GRID_COPY
              grid_t* gridPtr,
#endif /* !GRID_COPY */
              queue_t* queuePtr, coordinate_t* srcPtr, coordinate_t* dstPtr)
{
    long xCost = routerPtr->xCost;
    long yCost = routerPtr->yCost;
    long zCost = routerPtr->zCost;

    /*
     * Potential Optimization: Make 'src' the one closest to edge.
     * This will likely decrease the area of the emitted wave.
     */

    PQUEUE_CLEAR(queuePtr);
    long* srcGridPointPtr =
        grid_getPointRef(myGridPtr, srcPtr->x, srcPtr->y, srcPtr->z);
    PQUEUE_PUSH(queuePtr, (void*)srcGridPointPtr);
    grid_setPoint(myGridPtr, srcPtr->x, srcPtr->y, srcPtr->z, 0);
    grid_setPoint(myGridPtr, dstPtr->x, dstPtr->y, dstPtr->z, GRID_POINT_EMPTY);
    long* dstGridPointPtr =
        grid_getPointRef(myGridPtr, dstPtr->x, dstPtr->y, dstPtr->z);
    bool_t isPathFound = FALSE;

    while (!PQUEUE_ISEMPTY(queuePtr)) {

        long* gridPointPtr = (long*)PQUEUE_POP(queuePtr);
        if (gridPointPtr == dstGridPointPtr) {
            isPathFound = TRUE;
            break;
        }

        long x;
        long y;
        long z;
        grid_getPointIndices(myGridPtr, gridPointPtr, &x, &y, &z);
        long value = (*gridPointPtr);

        /*
         * Check 6 neighbors
         *
         * Potential Optimization: Only need to check 5 of these
         */
        HTMPexpandToNeighbor(myGridPtr,
                          x+1, y,   z,   (value + xCost), queuePtr);
        HTMPexpandToNeighbor(myGridPtr,
                          x-1, y,   z,   (value + xCost), queuePtr);
        HTMPexpandToNeighbor(myGridPtr,
                          x,   y+1, z,   (value + yCost), queuePtr);
        HTMPexpandToNeighbor(myGridPtr,
                          x,   y-1, z,   (value + yCost), queuePtr);
        HTMPexpandToNeighbor(myGridPtr,
                          x,   y,   z+1, (value + zCost), queuePtr);
        HTMPexpandToNeighbor(myGridPtr,
                          x,   y,   z-1, (value + zCost), queuePtr);

    } /* iterate over work queue */

#if DEBUG
    printf("Expansion (%li, %li, %li) -> (%li, %li, %li):\n",
           srcPtr->x, srcPtr->y, srcPtr->z,
           dstPtr->x, dstPtr->y, dstPtr->z);
    grid_print(myGridPtr);
#endif /*  DEBUG */

    return isPathFound;
}


/* =============================================================================
 * traceToNeighbor
 * =============================================================================
 */
TM_PURE
static void
traceToNeighbor (grid_t* myGridPtr,
                 point_t* currPtr,
                 point_t* movePtr,
                 bool_t useMomentum,
                 long bendCost,
                 point_t* nextPtr)
{
    long x = currPtr->x + movePtr->x;
    long y = currPtr->y + movePtr->y;
    long z = currPtr->z + movePtr->z;

    if (grid_isPointValid(myGridPtr, x, y, z) &&
        !grid_isPointEmpty(myGridPtr, x, y, z) &&
        !grid_isPointFull(myGridPtr, x, y, z))
    {
        long value = grid_getPoint(myGridPtr, x, y, z);
        long b = 0;
        if (useMomentum && (currPtr->momentum != movePtr->momentum)) {
            b = bendCost;
        }
        if ((value + b) <= nextPtr->value) { /* '=' favors neighbors over current */
            nextPtr->x = x;
            nextPtr->y = y;
            nextPtr->z = z;
            nextPtr->value = value;
            nextPtr->momentum = movePtr->momentum;
        }
    }
}


/* =============================================================================
 * HTMPdoTraceback
 * =============================================================================
 */
static vector_t*
HTMPdoTraceback (TM_ARG  grid_t* gridPtr, grid_t* myGridPtr,
              coordinate_t* dstPtr, long bendCost)
{
    vector_t* pointVectorPtr = HTMVECTOR_ALLOC(1);
    assert(pointVectorPtr);

    point_t next;
    next.x = dstPtr->x;
    next.y = dstPtr->y;
    next.z = dstPtr->z;
    next.value = grid_getPoint(myGridPtr, next.x, next.y, next.z);
    next.momentum = MOMENTUM_ZERO;

    while (1) {

        long* gridPointPtr = grid_getPointRef(gridPtr, next.x, next.y, next.z);
        HTMVECTOR_PUSHBACK(pointVectorPtr, (void*)gridPointPtr);
        grid_setPoint(myGridPtr, next.x, next.y, next.z, GRID_POINT_FULL);

        /* Check if we are done */
        if (next.value == 0) {
            break;
        }
        point_t curr = next;

        /*
         * Check 6 neighbors
         *
         * Potential Optimization: Only need to check 5 of these
         */
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSX, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSY, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSZ, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGX, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGY, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGZ, TRUE, bendCost, &next);

#if DEBUG
        printf("(%li, %li, %li)\n", next.x, next.y, next.z);
#endif /* DEBUG */
        /*
         * Because of bend costs, none of the neighbors may appear to be closer.
         * In this case, pick a neighbor while ignoring momentum.
         */
        if ((curr.x == next.x) &&
            (curr.y == next.y) &&
            (curr.z == next.z))
        {
            next.value = curr.value;
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSX, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSY, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSZ, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGX, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGY, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGZ, FALSE, bendCost, &next);

            if ((curr.x == next.x) &&
                (curr.y == next.y) &&
                (curr.z == next.z))
            {
                PVECTOR_FREE(pointVectorPtr);
#if DEBUG
                puts("[dead]");
#endif
                return NULL; /* cannot find path */
            }
        }
    }

#if DEBUG
    puts("");
#endif /* DEBUG */

    return pointVectorPtr;
}


/* =============================================================================
 * TMPdoTraceback
 * =============================================================================
 */
TM_CALLABLE
static vector_t*
TMPdoTraceback (TM_ARG  grid_t* gridPtr, grid_t* myGridPtr,
              coordinate_t* dstPtr, long bendCost)
{
    vector_t* pointVectorPtr = TMVECTOR_ALLOC(1);
    assert(pointVectorPtr);

    point_t next;
    next.x = dstPtr->x;
    next.y = dstPtr->y;
    next.z = dstPtr->z;
    next.value = grid_getPoint(myGridPtr, next.x, next.y, next.z);
    next.momentum = MOMENTUM_ZERO;

    while (1) {

        long* gridPointPtr = grid_getPointRef(gridPtr, next.x, next.y, next.z);
        TMVECTOR_PUSHBACK(pointVectorPtr, (void*)gridPointPtr);
        grid_setPoint(myGridPtr, next.x, next.y, next.z, GRID_POINT_FULL);

        /* Check if we are done */
        if (next.value == 0) {
            break;
        }
        point_t curr = next;

        /*
         * Check 6 neighbors
         *
         * Potential Optimization: Only need to check 5 of these
         */
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSX, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSY, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSZ, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGX, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGY, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGZ, TRUE, bendCost, &next);

#if DEBUG
        printf("(%li, %li, %li)\n", next.x, next.y, next.z);
#endif /* DEBUG */
        /*
         * Because of bend costs, none of the neighbors may appear to be closer.
         * In this case, pick a neighbor while ignoring momentum.
         */
        if ((curr.x == next.x) &&
            (curr.y == next.y) &&
            (curr.z == next.z))
        {
            next.value = curr.value;
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSX, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSY, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSZ, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGX, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGY, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGZ, FALSE, bendCost, &next);

            if ((curr.x == next.x) &&
                (curr.y == next.y) &&
                (curr.z == next.z))
            {
                PVECTOR_FREE(pointVectorPtr);
#if DEBUG
                puts("[dead]");
#endif
                return NULL; /* cannot find path */
            }
        }
    }

#if DEBUG
    puts("");
#endif /* DEBUG */

    return pointVectorPtr;
}


/* =============================================================================
 * router_solve
 * =============================================================================
 */
void
router_solve (void* argPtr)
{
    TM_THREAD_ENTER();

    router_solve_arg_t* routerArgPtr = (router_solve_arg_t*)argPtr;
    router_t* routerPtr = routerArgPtr->routerPtr;
    maze_t* mazePtr = routerArgPtr->mazePtr;
    vector_t* myPathVectorPtr = PVECTOR_ALLOC(1);
    assert(myPathVectorPtr);

    queue_t* workQueuePtr = mazePtr->workQueuePtr;
    grid_t* gridPtr = mazePtr->gridPtr;
    grid_t* myGridPtr =
        PGRID_ALLOC(gridPtr->width, gridPtr->height, gridPtr->depth);
    assert(myGridPtr);
    long bendCost = routerPtr->bendCost;
    queue_t* myExpansionQueuePtr = PQUEUE_ALLOC(-1);

    /*
     * Iterate over work list to route each path. This involves an
     * 'expansion' and 'traceback' phase for each source/destination pair.
     */
    while (1) {

        pair_t* coordinatePairPtr;
        HTM_TX_INIT;
tsx_begin_pop:
        if (HTM_BEGIN(tsx_status, global_tsx_status)) {
            HTM_LOCK_READ();
            if (HTMQUEUE_ISEMPTY(workQueuePtr)) {
                coordinatePairPtr = NULL;
            } else {
                coordinatePairPtr = (pair_t*)HTMQUEUE_POP(workQueuePtr);
            }
            HTM_END(global_tsx_status);
        } else {
            HTM_RETRY(tsx_status, tsx_begin_pop);

            TM_BEGIN();
            if (TMQUEUE_ISEMPTY(workQueuePtr)) {
                coordinatePairPtr = NULL;
            } else {
                coordinatePairPtr = (pair_t*)TMQUEUE_POP(workQueuePtr);
            }
            TM_END();
            }

        if (coordinatePairPtr == NULL) {
            break;
        }

        coordinate_t* srcPtr = coordinatePairPtr->firstPtr;
        coordinate_t* dstPtr = coordinatePairPtr->secondPtr;

        bool_t success;
        vector_t* pointVectorPtr = NULL;

#if defined(HTM) && defined(GRID_COPY)
        grid_copy(myGridPtr, gridPtr); /* ok if not most up-to-date */
#endif /* HTM && GRID_COPY */
tsx_begin_expansion:
        if (HTM_BEGIN(tsx_status, global_tsx_status)) {
            HTM_LOCK_READ();
            success = FALSE;
            if (HTMPdoExpansion(routerPtr, myGridPtr,
#ifndef GRID_COPY
                                gridPtr,
#endif /* !GRID_COPY */
                                myExpansionQueuePtr, srcPtr, dstPtr)) {
                pointVectorPtr = HTMPdoTraceback(gridPtr, myGridPtr, dstPtr, bendCost);
                if (pointVectorPtr) {
                    HTMGRID_ADDPATH(gridPtr, pointVectorPtr);
                    HTM_LOCAL_WRITE(success, TRUE);
                }
            }
            HTM_END(global_tsx_status);
        } else {
            HTM_RETRY(tsx_status, tsx_begin_expansion);

            TM_BEGIN();
            success = FALSE;
#ifdef GRID_COPY
            grid_copy(myGridPtr, gridPtr); /* ok if not most up-to-date */
#endif /* GRID_COPY */
            if (TMPdoExpansion(routerPtr, myGridPtr,
#ifndef GRID_COPY
                               gridPtr,
#endif /* !GRID_COPY */
                               myExpansionQueuePtr, srcPtr, dstPtr)) {
                pointVectorPtr = TMPdoTraceback(gridPtr, myGridPtr, dstPtr, bendCost);
                if (pointVectorPtr) {
                    TMGRID_ADDPATH(gridPtr, pointVectorPtr);
                    TM_LOCAL_WRITE(success, TRUE);
                }
            }
            TM_END();
        }

        if (success) {
            bool_t status = PVECTOR_PUSHBACK(myPathVectorPtr,
                                             (void*)pointVectorPtr);
            assert(status);
        }

        pair_free(coordinatePairPtr);
    }

    /*
     * Add my paths to global list
     */
    list_t* pathVectorListPtr = routerArgPtr->pathVectorListPtr;
    HTM_TX_INIT;
tsx_begin_insert:
    if (HTM_BEGIN(tsx_status, global_tsx_status)) {
      HTM_LOCK_READ();
      HTMLIST_INSERT(pathVectorListPtr, (void*)myPathVectorPtr);
      HTM_END(global_tsx_status);
    } else {
      HTM_RETRY(tsx_status, tsx_begin_insert);

      TM_BEGIN();
      TMLIST_INSERT(pathVectorListPtr, (void*)myPathVectorPtr);
      TM_END();
    }

    PGRID_FREE(myGridPtr);
    PQUEUE_FREE(myExpansionQueuePtr);

#if DEBUG
    puts("\nFinal Grid:");
    grid_print(gridPtr);
#endif /* DEBUG */

    TM_THREAD_EXIT();
}


/* =============================================================================
 *
 * End of router.c
 *
 * =============================================================================
 */


#if !defined(ORIGINAL) && defined(MERGE_ROUTER) && !defined(GRID_COPY)
stm_merge_t TMrouter_merge(stm_merge_context_t *params) {
    const stm_op_id_t op = stm_get_op_opcode(params->current);

    const stm_union_t *args;
    ssize_t nargs = stm_get_op_args(params->current, &args);
    if (nargs == -1)
      return STM_MERGE_UNSUPPORTED;

    if (STM_SAME_OPID(op, EXPAND)) {
        ASSERT(nargs == 7);
        const grid_t* myGridPtr = args[0].ptr;
        ASSERT(myGridPtr);
        const grid_t* gridPtr = args[1].ptr;
        ASSERT(gridPtr);
        const long x = args[2].sint;
        ASSERT(x);
        const long y = args[3].sint;
        ASSERT(y);
        const long z = args[4].sint;
        ASSERT(z);
        const long value = args[5].sint;
        ASSERT(value);
        const queue_t *queuePtr = args[6].ptr;
        ASSERT(queuePtr);
        if (params->leaf == 1) {
            ASSERT(ENTRY_VALID(params->conflict.entries->e1));
            const stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);

# ifdef TM_DEBUG
            printf("\nEXPAND addr:%p myGridPtr:%p gridPtr:%p x:%ld y:%ld z:%ld value:%ld queuePtr:%p\n", params->addr, myGridPtr, gridPtr, x, y, z, value, queuePtr);
            long old;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, *(long *)params->addr, old));
# endif
            long new;
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, *(long *)params->addr, new));
# ifdef TM_DEBUG
            printf("EXPAND gridPtr->neighborValue:%p (old):%ld (new):%ld\n", params->addr, old, new);
# endif
            long *myGridPt = myGridPtr->points + ((long *)params->addr - gridPtr->points);
# ifdef TM_DEBUG
            printf("EXPAND myGridPtr->neighborValue:%p %ld\n", myGridPt, *myGridPt);
# endif
            return *myGridPt != GRID_POINT_FULL ? STM_MERGE_OK : STM_MERGE_ABORT;
        }
    }

# ifdef TM_DEBUG
    printf("\nROUTER_MERGE UNSUPPORTED addr:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}

__attribute__((constructor)) void router_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*pllllp[], {&ffi_type_pointer, &ffi_type_slong, &ffi_type_slong, &ffi_type_slong, &ffi_type_slong, &ffi_type_pointer});
    #define TM_LOG_OP TM_LOG_OP_INIT
    #include "router.inc"
    #undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_ROUTER && !defined(GRID_COPY) */
