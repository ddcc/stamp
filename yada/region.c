/* =============================================================================
 *
 * region.c
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
#include "region.h"
#include "coordinate.h"
#include "element.h"
#include "list.h"
#include "map.h"
#include "queue.h"
#include "mesh.h"
#include "tm.h"


struct region {
    coordinate_t centerCoordinate;
    queue_t* expandQueuePtr;
    list_t* beforeListPtr; /* before retriangulation; list to avoid duplicates */
    list_t* borderListPtr; /* edges adjacent to region; list to avoid duplicates */
    vector_t* badVectorPtr;
};

#if !defined(ORIGINAL) && defined(MERGE_REGION)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "region.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_REGION */

HTM_STATS(global_tsx_status);

TM_INIT_GLOBAL;

/* =============================================================================
 * Pregion_alloc
 * =============================================================================
 */
region_t*
Pregion_alloc ()
{
    region_t* regionPtr;

    regionPtr = (region_t*)P_MALLOC(sizeof(region_t));
    if (regionPtr) {
        regionPtr->expandQueuePtr = PQUEUE_ALLOC(-1);
        assert(regionPtr->expandQueuePtr);
        regionPtr->beforeListPtr = PLIST_ALLOC(&element_listCompare);
        assert(regionPtr->beforeListPtr);
        regionPtr->borderListPtr = PLIST_ALLOC(&element_listCompareEdge);
        assert(regionPtr->borderListPtr);
        regionPtr->badVectorPtr = PVECTOR_ALLOC(1);
        assert(regionPtr->badVectorPtr);
    }

    return regionPtr;
}


/* =============================================================================
 * Pregion_free
 * =============================================================================
 */
void
Pregion_free (region_t* regionPtr)
{
    for (size_t i = 0; i < PVECTOR_GETSIZE(regionPtr->badVectorPtr); ++i) {
        element_t* badElementPtr = (element_t*)vector_at(regionPtr->badVectorPtr, i);
        PELEMENT_FREE(badElementPtr);
    }
    PVECTOR_FREE(regionPtr->badVectorPtr);
    PLIST_FREE(regionPtr->borderListPtr, NULL);
    PLIST_FREE(regionPtr->beforeListPtr, NULL);
    PQUEUE_FREE(regionPtr->expandQueuePtr);
    P_FREE(regionPtr);
}


/* =============================================================================
 * addToBadVector
 * =============================================================================
 */
void
addToBadVector (vector_t* badVectorPtr, element_t* badElementPtr)
{
    bool_t status = PVECTOR_PUSHBACK(badVectorPtr, (void*)badElementPtr);
    assert(status);
    element_setIsReferenced(badElementPtr, TRUE);
}


/* =============================================================================
 * HTMaddToBadVector
 * =============================================================================
 */
void
HTMaddToBadVector (vector_t* badVectorPtr, element_t* badElementPtr)
{
    bool_t status = PVECTOR_PUSHBACK(badVectorPtr, (void*)badElementPtr);
    assert(status);
    HTMELEMENT_SETISREFERENCED(badElementPtr, TRUE);
}


/* =============================================================================
 * TMaddToBadVector
 * =============================================================================
 */
void
TMaddToBadVector (TM_ARGDECL  vector_t* badVectorPtr, element_t* badElementPtr)
{
    bool_t status = PVECTOR_PUSHBACK(badVectorPtr, (void*)badElementPtr);
    assert(status);
    TMELEMENT_SETISREFERENCED(badElementPtr, TRUE);
}


/* =============================================================================
 * retriangulate
 * -- Returns net amount of elements added to mesh
 * =============================================================================
 */
long
retriangulate (element_t* elementPtr,
               region_t* regionPtr,
               mesh_t* meshPtr,
               MAP_T* edgeMapPtr)
{
    vector_t* badVectorPtr = regionPtr->badVectorPtr; /* private */
    list_t* beforeListPtr = regionPtr->beforeListPtr; /* private */
    list_t* borderListPtr = regionPtr->borderListPtr; /* private */
    list_iter_t it;
    long numDelta = 0L;

    assert(edgeMapPtr);

    coordinate_t centerCoordinate = element_getNewPoint(elementPtr);

    /*
     * Remove the old triangles
     */

    list_iter_reset(&it, beforeListPtr);
    while (list_iter_hasNext(&it, beforeListPtr)) {
        element_t* beforeElementPtr =
            (element_t*)list_iter_next(&it, beforeListPtr);
        mesh_remove(meshPtr, beforeElementPtr);
    }

    numDelta -= PLIST_GETSIZE(beforeListPtr);

    /*
     * If segment is encroached, split it in half
     */

    if (element_getNumEdge(elementPtr) == 1) {

        coordinate_t coordinates[2];

        edge_t* edgePtr = element_getEdge(elementPtr, 0);
        coordinates[0] = centerCoordinate;

        coordinates[1] = *(coordinate_t*)(edgePtr->firstPtr);
        element_t* aElementPtr = element_alloc(coordinates, 2);
        assert(aElementPtr);
        mesh_insert(meshPtr, aElementPtr, edgeMapPtr);

        coordinates[1] = *(coordinate_t*)(edgePtr->secondPtr);
        element_t* bElementPtr = element_alloc(coordinates, 2);
        assert(bElementPtr);
        mesh_insert(meshPtr, bElementPtr, edgeMapPtr);

        bool_t status;
        status = mesh_removeBoundary(meshPtr, element_getEdge(elementPtr, 0));
        assert(status);
        status = mesh_insertBoundary(meshPtr, element_getEdge(aElementPtr, 0));
        assert(status);
        status = mesh_insertBoundary(meshPtr, element_getEdge(bElementPtr, 0));
        assert(status);

        numDelta += 2;
    }

    /*
     * Insert the new triangles. These are contructed using the new
     * point and the two points from the border segment.
     */

    list_iter_reset(&it, borderListPtr);
    while (list_iter_hasNext(&it, borderListPtr)) {
        element_t* afterElementPtr;
        coordinate_t coordinates[3];
        edge_t* borderEdgePtr = (edge_t*)list_iter_next(&it, borderListPtr);
        assert(borderEdgePtr);
        coordinates[0] = centerCoordinate;
        coordinates[1] = *(coordinate_t*)(borderEdgePtr->firstPtr);
        coordinates[2] = *(coordinate_t*)(borderEdgePtr->secondPtr);
        afterElementPtr = element_alloc(coordinates, 3);
        assert(afterElementPtr);
        mesh_insert(meshPtr, afterElementPtr, edgeMapPtr);
        if (element_isBad(afterElementPtr)) {
            addToBadVector(TM_ARG  badVectorPtr, afterElementPtr);
        }
    }

    numDelta += PLIST_GETSIZE(borderListPtr);
    return numDelta;
}


/* =============================================================================
 * HTMretriangulate
 * -- Returns net amount of elements added to mesh
 * =============================================================================
 */
long
HTMretriangulate (element_t* elementPtr,
                 region_t* regionPtr,
                 mesh_t* meshPtr,
                 MAP_T* edgeMapPtr)
{
    vector_t* badVectorPtr = regionPtr->badVectorPtr; /* private */
    list_t* beforeListPtr = regionPtr->beforeListPtr; /* private */
    list_t* borderListPtr = regionPtr->borderListPtr; /* private */
    list_iter_t it;
    long numDelta = 0L;

    assert(edgeMapPtr);

    coordinate_t centerCoordinate = element_getNewPoint(elementPtr);

    /*
     * Remove the old triangles
     */

    list_iter_reset(&it, beforeListPtr);
    while (list_iter_hasNext(&it, beforeListPtr)) {
        element_t* beforeElementPtr =
            (element_t*)list_iter_next(&it, beforeListPtr);
        HTMMESH_REMOVE(meshPtr, beforeElementPtr);
    }

    numDelta -= PLIST_GETSIZE(beforeListPtr);

    /*
     * If segment is encroached, split it in half
     */

    if (element_getNumEdge(elementPtr) == 1) {

        coordinate_t coordinates[2];

        edge_t* edgePtr = element_getEdge(elementPtr, 0);
        coordinates[0] = centerCoordinate;

        coordinates[1] = *(coordinate_t*)(edgePtr->firstPtr);
        element_t* aElementPtr = HTMELEMENT_ALLOC(coordinates, 2);
        assert(aElementPtr);
        HTMMESH_INSERT(meshPtr, aElementPtr, edgeMapPtr);

        coordinates[1] = *(coordinate_t*)(edgePtr->secondPtr);
        element_t* bElementPtr = HTMELEMENT_ALLOC(coordinates, 2);
        assert(bElementPtr);
        HTMMESH_INSERT(meshPtr, bElementPtr, edgeMapPtr);

        bool_t status;
        status = HTMMESH_REMOVEBOUNDARY(meshPtr, element_getEdge(elementPtr, 0));
        assert(status);
        status = HTMMESH_INSERTBOUNDARY(meshPtr, element_getEdge(aElementPtr, 0));
        assert(status);
        status = HTMMESH_INSERTBOUNDARY(meshPtr, element_getEdge(bElementPtr, 0));
        assert(status);

        numDelta += 2;
    }

    /*
     * Insert the new triangles. These are contructed using the new
     * point and the two points from the border segment.
     */

    list_iter_reset(&it, borderListPtr);
    while (list_iter_hasNext(&it, borderListPtr)) {
        element_t* afterElementPtr;
        coordinate_t coordinates[3];
        edge_t* borderEdgePtr = (edge_t*)list_iter_next(&it, borderListPtr);
        assert(borderEdgePtr);
        coordinates[0] = centerCoordinate;
        coordinates[1] = *(coordinate_t*)(borderEdgePtr->firstPtr);
        coordinates[2] = *(coordinate_t*)(borderEdgePtr->secondPtr);
        afterElementPtr = HTMELEMENT_ALLOC(coordinates, 3);
        assert(afterElementPtr);
        HTMMESH_INSERT(meshPtr, afterElementPtr, edgeMapPtr);
        if (element_isBad(afterElementPtr)) {
            HTMaddToBadVector(TM_ARG  badVectorPtr, afterElementPtr);
        }
    }

    numDelta += PLIST_GETSIZE(borderListPtr);

    return numDelta;
}


/* =============================================================================
 * TMretriangulate
 * -- Returns net amount of elements added to mesh
 * =============================================================================
 */
TM_CANCELLABLE
long
TMretriangulate (TM_ARGDECL
                 element_t* elementPtr,
                 region_t* regionPtr,
                 mesh_t* meshPtr,
                 MAP_T* edgeMapPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_REGION)
    TM_LOG_BEGIN(REG_TRIANGULATE, NULL, elementPtr, regionPtr, meshPtr, edgeMapPtr);
#endif /* !ORIGINAL && MERGE_REGION */

    vector_t* badVectorPtr = regionPtr->badVectorPtr; /* private */
    list_t* beforeListPtr = regionPtr->beforeListPtr; /* private */
    list_t* borderListPtr = regionPtr->borderListPtr; /* private */
    list_iter_t it;
    long numDelta = 0L;

    assert(edgeMapPtr);

    coordinate_t centerCoordinate = element_getNewPoint(elementPtr);

    /*
     * Remove the old triangles
     */

    list_iter_reset(&it, beforeListPtr);
    while (list_iter_hasNext(&it, beforeListPtr)) {
        element_t* beforeElementPtr =
            (element_t*)list_iter_next(&it, beforeListPtr);
        TMMESH_REMOVE(meshPtr, beforeElementPtr);
    }

    numDelta -= PLIST_GETSIZE(beforeListPtr);

    /*
     * If segment is encroached, split it in half
     */

    if (element_getNumEdge(elementPtr) == 1) {

        coordinate_t coordinates[2];

        edge_t* edgePtr = element_getEdge(elementPtr, 0);
        coordinates[0] = centerCoordinate;

        coordinates[1] = *(coordinate_t*)(edgePtr->firstPtr);
        element_t* aElementPtr = TMELEMENT_ALLOC(coordinates, 2);
        assert(aElementPtr);
        TMMESH_INSERT(meshPtr, aElementPtr, edgeMapPtr);

        coordinates[1] = *(coordinate_t*)(edgePtr->secondPtr);
        element_t* bElementPtr = TMELEMENT_ALLOC(coordinates, 2);
        assert(bElementPtr);
        TMMESH_INSERT(meshPtr, bElementPtr, edgeMapPtr);

        bool_t status;
        status = TMMESH_REMOVEBOUNDARY(meshPtr, element_getEdge(elementPtr, 0));
        assert(status);
        status = TMMESH_INSERTBOUNDARY(meshPtr, element_getEdge(aElementPtr, 0));
        assert(status);
        status = TMMESH_INSERTBOUNDARY(meshPtr, element_getEdge(bElementPtr, 0));
        assert(status);

        numDelta += 2;
    }

    /*
     * Insert the new triangles. These are contructed using the new
     * point and the two points from the border segment.
     */

    list_iter_reset(&it, borderListPtr);
    while (list_iter_hasNext(&it, borderListPtr)) {
        element_t* afterElementPtr;
        coordinate_t coordinates[3];
        edge_t* borderEdgePtr = (edge_t*)list_iter_next(&it, borderListPtr);
        assert(borderEdgePtr);
        coordinates[0] = centerCoordinate;
        coordinates[1] = *(coordinate_t*)(borderEdgePtr->firstPtr);
        coordinates[2] = *(coordinate_t*)(borderEdgePtr->secondPtr);
        afterElementPtr = TMELEMENT_ALLOC(coordinates, 3);
        assert(afterElementPtr);
        TMMESH_INSERT(meshPtr, afterElementPtr, edgeMapPtr);
        if (element_isBad(afterElementPtr)) {
            TMaddToBadVector(TM_ARG  badVectorPtr, afterElementPtr);
        }
    }

    numDelta += PLIST_GETSIZE(borderListPtr);

#if !defined(ORIGINAL) && defined(MERGE_REGION)
    TM_LOG_END(REG_TRIANGULATE, &numDelta);
#endif /* !ORIGINAL && MERGE_REGION */
    return numDelta;
}


/* =============================================================================
 * growRegion
 * -- Return NULL if success, else pointer to encroached boundary
 * =============================================================================
 */
element_t*
growRegion (element_t* centerElementPtr,
            region_t* regionPtr,
            mesh_t* meshPtr,
            MAP_T* edgeMapPtr)
{
    element_t *rv;
    bool_t isBoundary = FALSE;

    if (element_getNumEdge(centerElementPtr) == 1) {
        isBoundary = TRUE;
    }

    list_t* beforeListPtr = regionPtr->beforeListPtr;
    list_t* borderListPtr = regionPtr->borderListPtr;
    queue_t* expandQueuePtr = regionPtr->expandQueuePtr;

    PLIST_CLEAR(beforeListPtr, NULL);
    PLIST_CLEAR(borderListPtr, NULL);
    PQUEUE_CLEAR(expandQueuePtr);

    coordinate_t centerCoordinate = element_getNewPoint(centerElementPtr);
    coordinate_t* centerCoordinatePtr = &centerCoordinate;

    PQUEUE_PUSH(expandQueuePtr, (void*)centerElementPtr);
    while (!PQUEUE_ISEMPTY(expandQueuePtr)) {

        element_t* currentElementPtr = (element_t*)PQUEUE_POP(expandQueuePtr);

        PLIST_INSERT(beforeListPtr, (void*)currentElementPtr); /* no duplicates */
        list_t* neighborListPtr = element_getNeighborListPtr(currentElementPtr);

        list_iter_t it;
        list_iter_reset(&it, neighborListPtr);
        while (list_iter_hasNext(&it, neighborListPtr)) {
            element_t* neighborElementPtr =
                (element_t*)list_iter_next(&it, neighborListPtr);
            bool_t isGarbage = element_isGarbage(neighborElementPtr); /* so we can detect conflicts */
            ASSERT_FAIL(!isGarbage);
            if (!list_find(beforeListPtr, (void*)neighborElementPtr)) {
                if (element_isInCircumCircle(neighborElementPtr, centerCoordinatePtr)) {
                    /* This is part of the region */
                    if (!isBoundary && (element_getNumEdge(neighborElementPtr) == 1)) {
                        /* Encroached on mesh boundary so split it and restart */
                        rv = neighborElementPtr;
                        goto out;
                    } else {
                        /* Continue breadth-first search */
                        bool_t isSuccess;
                        isSuccess = PQUEUE_PUSH(expandQueuePtr,
                                                (void*)neighborElementPtr);
                        assert(isSuccess);
                    }
                } else {
                    /* This element borders region; save info for retriangulation */
                    edge_t* borderEdgePtr =
                        element_getCommonEdge(neighborElementPtr, currentElementPtr);
                    if (!borderEdgePtr) {
                        abort();
                    }
                    PLIST_INSERT(borderListPtr,
                                 (void*)borderEdgePtr); /* no duplicates */
                    if (!MAP_CONTAINS(edgeMapPtr, borderEdgePtr)) {
                        MAP_INSERT(edgeMapPtr, borderEdgePtr, neighborElementPtr);
                    }
                }
            } /* not visited before */
        } /* for each neighbor */

    } /* breadth-first search */

    rv = NULL;
out:
    return rv;
}


/* =============================================================================
 * HTMgrowRegion
 * -- Return NULL if success, else pointer to encroached boundary
 * =============================================================================
 */
element_t*
HTMgrowRegion (element_t* centerElementPtr,
              region_t* regionPtr,
              mesh_t* meshPtr,
              MAP_T* edgeMapPtr)
{
    element_t *rv;
    bool_t isBoundary = FALSE;

    if (element_getNumEdge(centerElementPtr) == 1) {
        isBoundary = TRUE;
    }

    list_t* beforeListPtr = regionPtr->beforeListPtr;
    list_t* borderListPtr = regionPtr->borderListPtr;
    queue_t* expandQueuePtr = regionPtr->expandQueuePtr;

    PLIST_CLEAR(beforeListPtr, NULL);
    PLIST_CLEAR(borderListPtr, NULL);
    PQUEUE_CLEAR(expandQueuePtr);

    coordinate_t centerCoordinate = element_getNewPoint(centerElementPtr);
    coordinate_t* centerCoordinatePtr = &centerCoordinate;

    PQUEUE_PUSH(expandQueuePtr, (void*)centerElementPtr);
    while (!PQUEUE_ISEMPTY(expandQueuePtr)) {

        element_t* currentElementPtr = (element_t*)PQUEUE_POP(expandQueuePtr);

        PLIST_INSERT(beforeListPtr, (void*)currentElementPtr); /* no duplicates */
        list_t* neighborListPtr = element_getNeighborListPtr(currentElementPtr);

        list_iter_t it;
        HTMLIST_ITER_RESET(&it, neighborListPtr);
        while (HTMLIST_ITER_HASNEXT(&it, neighborListPtr)) {
            element_t* neighborElementPtr =
                (element_t*)HTMLIST_ITER_NEXT(&it, neighborListPtr);
            bool_t isGarbage = HTMELEMENT_ISGARBAGE(neighborElementPtr); /* so we can detect conflicts */
            ASSERT_FAIL(!isGarbage);
            if (!list_find(beforeListPtr, (void*)neighborElementPtr)) {
                if (element_isInCircumCircle(neighborElementPtr, centerCoordinatePtr)) {
                    /* This is part of the region */
                    if (!isBoundary && (element_getNumEdge(neighborElementPtr) == 1)) {
                        /* Encroached on mesh boundary so split it and restart */
                        rv = neighborElementPtr;
                        goto out;
                    } else {
                        /* Continue breadth-first search */
                        bool_t isSuccess;
                        isSuccess = PQUEUE_PUSH(expandQueuePtr,
                                                (void*)neighborElementPtr);
                        assert(isSuccess);
                    }
                } else {
                    /* This element borders region; save info for retriangulation */
                    edge_t* borderEdgePtr =
                        element_getCommonEdge(neighborElementPtr, currentElementPtr);
                    if (!borderEdgePtr) {
                        HTM_RESTART();
                    }
                    PLIST_INSERT(borderListPtr,
                                 (void*)borderEdgePtr); /* no duplicates */
                    if (!HTMMAP_CONTAINS(edgeMapPtr, borderEdgePtr)) {
                        HTMMAP_INSERT(edgeMapPtr, borderEdgePtr, neighborElementPtr);
                    }
                }
            } /* not visited before */
        } /* for each neighbor */

    } /* breadth-first search */

    rv = NULL;
out:
    return rv;
}


/* =============================================================================
 * TMgrowRegion
 * -- Return NULL if success, else pointer to encroached boundary
 * =============================================================================
 */
TM_CANCELLABLE
element_t*
TMgrowRegion (TM_ARGDECL
              element_t* centerElementPtr,
              region_t* regionPtr,
              mesh_t* meshPtr,
              MAP_T* edgeMapPtr)
{
    element_t *rv;
#if !defined(ORIGINAL) && defined(MERGE_REGION)
    TM_LOG_BEGIN(REG_GROW, NULL, centerElementPtr, regionPtr, meshPtr, edgeMapPtr);
#endif /* !ORIGINAL && MERGE_REGION */

    bool_t isBoundary = FALSE;

    if (element_getNumEdge(centerElementPtr) == 1) {
        isBoundary = TRUE;
    }

    list_t* beforeListPtr = regionPtr->beforeListPtr;
    list_t* borderListPtr = regionPtr->borderListPtr;
    queue_t* expandQueuePtr = regionPtr->expandQueuePtr;

    PLIST_CLEAR(beforeListPtr, NULL);
    PLIST_CLEAR(borderListPtr, NULL);
    PQUEUE_CLEAR(expandQueuePtr);

    coordinate_t centerCoordinate = element_getNewPoint(centerElementPtr);
    coordinate_t* centerCoordinatePtr = &centerCoordinate;

    PQUEUE_PUSH(expandQueuePtr, (void*)centerElementPtr);
    while (!PQUEUE_ISEMPTY(expandQueuePtr)) {

        element_t* currentElementPtr = (element_t*)PQUEUE_POP(expandQueuePtr);

        PLIST_INSERT(beforeListPtr, (void*)currentElementPtr); /* no duplicates */
        list_t* neighborListPtr = element_getNeighborListPtr(currentElementPtr);

        list_iter_t it;
        TMLIST_ITER_RESET(&it, neighborListPtr);
        while (TMLIST_ITER_HASNEXT(&it, neighborListPtr)) {
            element_t* neighborElementPtr =
                (element_t*)TMLIST_ITER_NEXT(&it, neighborListPtr);
            bool_t isGarbage = TMELEMENT_ISGARBAGE(neighborElementPtr); /* so we can detect conflicts */
            ASSERT_FAIL(!isGarbage);
            if (!list_find(beforeListPtr, (void*)neighborElementPtr)) {
                if (element_isInCircumCircle(neighborElementPtr, centerCoordinatePtr)) {
                    /* This is part of the region */
                    if (!isBoundary && (element_getNumEdge(neighborElementPtr) == 1)) {
                        /* Encroached on mesh boundary so split it and restart */
                        rv = neighborElementPtr;
                        goto out;
                    } else {
                        /* Continue breadth-first search */
                        bool_t isSuccess;
                        isSuccess = PQUEUE_PUSH(expandQueuePtr,
                                                (void*)neighborElementPtr);
                        assert(isSuccess);
                    }
                } else {
                    /* This element borders region; save info for retriangulation */
                    edge_t* borderEdgePtr =
                        element_getCommonEdge(neighborElementPtr, currentElementPtr);
                    if (!borderEdgePtr) {
                        TM_RESTART();
                    }
                    PLIST_INSERT(borderListPtr,
                                 (void*)borderEdgePtr); /* no duplicates */
                    if (!TMMAP_CONTAINS(edgeMapPtr, borderEdgePtr)) {
                        TMMAP_INSERT(edgeMapPtr, borderEdgePtr, neighborElementPtr);
                    }
                }
            } /* not visited before */
        } /* for each neighbor */

    } /* breadth-first search */

    rv = NULL;
out:
#if !defined(ORIGINAL) && defined(MERGE_REGION)
    TM_LOG_END(REG_GROW, &rv);
#endif /* !ORIGINAL && MERGE_REGION */
    return rv;
}


/* =============================================================================
 * region_refine
 * -- Returns net number of elements added to mesh
 * =============================================================================
 */
long
region_refine (region_t* regionPtr, element_t* elementPtr, mesh_t* meshPtr)
{
    long numDelta = 0L;
    MAP_T* edgeMapPtr = NULL;
    element_t* encroachElementPtr = NULL;

    if (element_isGarbage(elementPtr)) /* so we can detect conflicts */
        goto out;

    while (1) {
        edgeMapPtr = MAP_ALLOC(&element_mapHashEdge, &element_mapCompareEdge);
        assert(edgeMapPtr);
        encroachElementPtr = growRegion(TM_ARG
                                      elementPtr,
                                      regionPtr,
                                      meshPtr,
                                      edgeMapPtr);

        if (encroachElementPtr) {
            element_setIsReferenced(encroachElementPtr, TRUE);
            numDelta += region_refine(TM_ARG
                                        regionPtr,
                                        encroachElementPtr,
                                        meshPtr);
            if (element_isGarbage(elementPtr)) {
                break;
            }
        } else {
            break;
        }

        MAP_FREE(edgeMapPtr, NULL);
    }

    /*
     * Perform retriangulation.
     */

    if (!element_isGarbage(elementPtr)) {
        numDelta += retriangulate(TM_ARG
                                    elementPtr,
                                    regionPtr,
                                    meshPtr,
                                    edgeMapPtr);
    }

    MAP_FREE(edgeMapPtr, NULL); /* no need to free elements */
out:
    return numDelta;
}


/* =============================================================================
 * HTMregion_refine
 * -- Returns net number of elements added to mesh
 * =============================================================================
 */
long
HTMregion_refine (region_t* regionPtr, element_t* elementPtr, mesh_t* meshPtr)
{
long numDelta = 0L;
    MAP_T* edgeMapPtr = NULL;
    element_t* encroachElementPtr = NULL;

    if (HTMELEMENT_ISGARBAGE(elementPtr)) /* so we can detect conflicts */
        goto out;

    while (1) {
        edgeMapPtr = HTMMAP_ALLOC(&element_mapHashEdge, &element_mapCompareEdge);
        assert(edgeMapPtr);
        encroachElementPtr = HTMgrowRegion(TM_ARG
                                      elementPtr,
                                      regionPtr,
                                      meshPtr,
                                      edgeMapPtr);

        if (encroachElementPtr) {
            HTMELEMENT_SETISREFERENCED(encroachElementPtr, TRUE);
            numDelta += HTMregion_refine(TM_ARG
                                        regionPtr,
                                        encroachElementPtr,
                                        meshPtr);
            if (HTMELEMENT_ISGARBAGE(elementPtr)) {
                break;
            }
        } else {
            break;
        }

        HTMMAP_FREE(edgeMapPtr, NULL);
    }

    /*
     * Perform retriangulation.
    */
    if (!HTMELEMENT_ISGARBAGE(elementPtr)) {
        numDelta += HTMretriangulate(TM_ARG
                                    elementPtr,
                                    regionPtr,
                                    meshPtr,
                                    edgeMapPtr);
    }

    HTMMAP_FREE(edgeMapPtr, NULL); /* no need to free elements */
out:

    return numDelta;
}


/* =============================================================================
 * TMregion_refine
 * -- Returns net number of elements added to mesh
 * =============================================================================
 */
TM_CANCELLABLE
long
TMregion_refine (TM_ARGDECL
                 region_t* regionPtr, element_t* elementPtr, mesh_t* meshPtr) {
    long numDelta = 0L;
    MAP_T* edgeMapPtr = NULL;
    element_t* encroachElementPtr = NULL;

#if !defined(ORIGINAL) && defined(STM_HTM)
    int tsx = 0;
#endif /* !ORIGINAL && STM_HTM */

#if !defined(ORIGINAL) && defined(MERGE_ELEMENT) && defined(MERGE_REGION)
    stm_merge_t merge(stm_merge_context_t *params) {
        extern const stm_op_id_t ELEM_ISGARBAGE;

        /* Update from ELEM_ISGARBAGE */
        if (!params->leaf && STM_SAME_OPID(stm_get_op_opcode(params->previous), ELEM_ISGARBAGE)) {
# ifdef TM_DEBUG
            printf("\nREG_REFINE_JIT <- ELEM_ISGARBAGE %ld\n", params->conflict.result.sint);
# endif
            ASSERT (params->addr == elementPtr);
            return STM_MERGE_ABORT;
        }

# ifdef TM_DEBUG
        printf("\nREG_REFINE_JIT UNSUPPORTED addr:%p\n", params->addr);
# endif
        return STM_MERGE_UNSUPPORTED;
    }

    TM_LOG_BEGIN(REG_REFINE, NULL, regionPtr, elementPtr, meshPtr);
#endif /* !ORIGINAL && MERGE_ELEMENT && MERGE_REGION */

    if (TMELEMENT_ISGARBAGE(elementPtr)) /* so we can detect conflicts */
        goto out;

    while (1) {
        edgeMapPtr = TMMAP_ALLOC(NULL, &element_mapCompareEdge);
        assert(edgeMapPtr);

#if !defined(ORIGINAL) && defined(STM_HTM)
        if (tsx)
            encroachElementPtr = HTMgrowRegion(TM_ARG
                                            elementPtr,
                                            regionPtr,
                                            meshPtr,
                                            edgeMapPtr);
        else
#endif /* !ORIGINAL && STM_HTM */
            encroachElementPtr = TMgrowRegion(TM_ARG
                                              elementPtr,
                                              regionPtr,
                                              meshPtr,
                                              edgeMapPtr);

#if !defined(ORIGINAL) && defined(STM_HTM)
        STM_HTM_TX_INIT;
tsx_begin:
        if (!tsx && STM_HTM_BEGIN()) {
            STM_HTM_START(tsx_status, global_tsx_status);
            if (STM_HTM_STARTED(tsx_status)) {
                tsx = 1;
            } else {
                HTM_RETRY(tsx_status, tsx_begin);
                tsx = 0;
                STM_HTM_EXIT();
            }
        }

        if (tsx) {
            if (encroachElementPtr) {
                HTMELEMENT_SETISREFERENCED(encroachElementPtr, TRUE);
                numDelta += HTMregion_refine(TM_ARG
                                            regionPtr,
                                            encroachElementPtr,
                                            meshPtr);
                if (HTMELEMENT_ISGARBAGE(elementPtr)) {
                    goto next;
                }
            } else {
                goto next;
            }

            goto done;
        }
#endif /* !ORIGINAL && STM_HTM */

        if (encroachElementPtr) {
            TMELEMENT_SETISREFERENCED(encroachElementPtr, TRUE);
            numDelta += TMregion_refine(TM_ARG
                                        regionPtr,
                                        encroachElementPtr,
                                        meshPtr);
            if (TMELEMENT_ISGARBAGE(elementPtr)) {
                break;
            }
        } else {
            break;
        }

done:
        TMMAP_FREE(edgeMapPtr, NULL);
    }

next:
    /*
     * Perform retriangulation.
     */
#if !defined(ORIGINAL) && defined(STM_HTM)
    if (tsx) {
        if (!HTMELEMENT_ISGARBAGE(elementPtr)) {
            numDelta += HTMretriangulate(TM_ARG
                                        elementPtr,
                                        regionPtr,
                                        meshPtr,
                                        edgeMapPtr);
        }

        tsx = 0;
        STM_HTM_END(global_tsx_status);
        STM_HTM_EXIT();
    } else {
#endif /* !ORIGINAL && STM_HTM */
        if (!TMELEMENT_ISGARBAGE(elementPtr)) {
            numDelta += TMretriangulate(TM_ARG
                                        elementPtr,
                                        regionPtr,
                                        meshPtr,
                                        edgeMapPtr);
        }
#if !defined(ORIGINAL) && defined(STM_HTM)
    }
#endif /* !ORIGINAL && STM_HTM */

    TMMAP_FREE(edgeMapPtr, NULL); /* no need to free elements */
out:
#if !defined(ORIGINAL) && defined(MERGE_REGION)
    TM_LOG_END(REG_REFINE, &numDelta);
#endif /* !ORIGINAL && MERGE_REGION */
    return numDelta;
}


/* =============================================================================
 * Pregion_clearBad
 * =============================================================================
 */
void
Pregion_clearBad (region_t* regionPtr)
{
    PVECTOR_CLEAR(regionPtr->badVectorPtr);
}


/* =============================================================================
 * region_freeBad
 * =============================================================================
 */
void
region_freeBad (region_t* regionPtr)
{
    for (size_t i = 0; i < PVECTOR_GETSIZE(regionPtr->badVectorPtr); ++i) {
        element_t* badElementPtr = (element_t*)vector_at(regionPtr->badVectorPtr, i);
        if (!ELEMENT_ISREFERENCED(badElementPtr))
            ELEMENT_FREE(badElementPtr);
    }
}


/* =============================================================================
 * HTMregion_freeBad
 * =============================================================================
 */
void
HTMregion_freeBad (region_t* regionPtr)
{
    for (size_t i = 0; i < PVECTOR_GETSIZE(regionPtr->badVectorPtr); ++i) {
        element_t* badElementPtr = (element_t*)vector_at(regionPtr->badVectorPtr, i);
        if (!HTMELEMENT_ISREFERENCED(badElementPtr))
            HTMELEMENT_FREE(badElementPtr);
    }
}


/* =============================================================================
 * TMregion_freeBad
 * =============================================================================
 */
TM_CALLABLE
void
TMregion_freeBad (region_t* regionPtr)
{
    for (size_t i = 0; i < PVECTOR_GETSIZE(regionPtr->badVectorPtr); ++i) {
        element_t* badElementPtr = (element_t*)vector_at(regionPtr->badVectorPtr, i);
        if (!TMELEMENT_ISREFERENCED(badElementPtr))
            TMELEMENT_FREE(badElementPtr);
    }
}


/* =============================================================================
 * region_transferBad
 * =============================================================================
 */
void
region_transferBad (region_t* regionPtr, heap_t* workHeapPtr)
{
    vector_t* badVectorPtr = regionPtr->badVectorPtr;
    long numBad = PVECTOR_GETSIZE(badVectorPtr);
    long i;

    for (i = 0; i < numBad; i++) {
        element_t* badElementPtr = (element_t*)vector_at(badVectorPtr, i);
        if (ELEMENT_ISGARBAGE(badElementPtr)) {
            // TMELEMENT_FREE(badElementPtr);
        } else {
            bool_t status = HEAP_INSERT(workHeapPtr, (void*)badElementPtr);
            assert(status);
        }
    }
}


/* =============================================================================
 * HTMregion_transferBad
 * =============================================================================
 */
void
HTMregion_transferBad (region_t* regionPtr, heap_t* workHeapPtr)
{
    vector_t* badVectorPtr = regionPtr->badVectorPtr;
    long numBad = PVECTOR_GETSIZE(badVectorPtr);
    long i;

    for (i = 0; i < numBad; i++) {
        element_t* badElementPtr = (element_t*)vector_at(badVectorPtr, i);
        if (HTMELEMENT_ISGARBAGE(badElementPtr)) {
            // TMELEMENT_FREE(badElementPtr);
        } else {
            bool_t status = HTMHEAP_INSERT(workHeapPtr, (void*)badElementPtr);
            assert(status);
        }
    }
}


/* =============================================================================
 * TMregion_transferBad
 * =============================================================================
 */
TM_CALLABLE
void
TMregion_transferBad (TM_ARGDECL  region_t* regionPtr, heap_t* workHeapPtr)
{
    vector_t* badVectorPtr = regionPtr->badVectorPtr;
    long numBad = PVECTOR_GETSIZE(badVectorPtr);
    long i;

#if !defined(ORIGINAL) && defined(MERGE_REGION)
    TM_LOG_BEGIN(REG_TRANSFERBAD, NULL, regionPtr, workHeapPtr);
#endif /* !ORIGINAL && MERGE_REGION */

    for (i = 0; i < numBad; i++) {
        element_t* badElementPtr = (element_t*)vector_at(badVectorPtr, i);
        if (TMELEMENT_ISGARBAGE(badElementPtr)) {
            // TMELEMENT_FREE(badElementPtr);
        } else {
            bool_t status = TMHEAP_INSERT(workHeapPtr, (void*)badElementPtr);
            assert(status);
        }
    }

#if !defined(ORIGINAL) && defined(MERGE_REGION)
    TM_LOG_END(REG_TRANSFERBAD, NULL);
#endif /* !ORIGINAL && MERGE_REGION */
}


/* =============================================================================
 *
 * End of region.c
 *
 * =============================================================================
 */

#if !defined(ORIGINAL) && defined(MERGE_REGION)
stm_merge_t TMregion_merge(stm_merge_context_t *params) {
    extern const stm_op_id_t ELEM_ISGARBAGE;
#ifdef MERGE_LIST
    extern const stm_op_id_t LIST_IT_NEXT;
#endif /* MERGE_LIST */
#ifdef MERGE_QUEUE
    extern const stm_op_id_t QUEUE_POP;
#endif /* MERGE_QUEUE */
    const stm_op_id_t op = stm_get_op_opcode(params->current);
    const stm_op_id_t prev = params->leaf ? STM_INVALID_OPID : stm_get_op_opcode(params->previous);

    /* Delayed merge only */
    ASSERT(!STM_SAME_OP(stm_get_current_op(), params->current));

    const stm_union_t *args;
    ssize_t nargs = stm_get_op_args(params->current, &args);

    if (STM_SAME_OPID(op, REG_TRANSFERBAD)) {
        ASSERT(nargs == 2);
        const region_t* regionPtr = args[0].ptr;
        ASSERT(regionPtr);
        const heap_t* workHeapPtr = args[1].ptr;
        ASSERT(workHeapPtr);
        if (params->leaf == 1) {
# ifdef TM_DEBUG
            printf("\nREG_TRANSFERBAD addr:%p regionPtr:%p workHeapPtr:%p\n", params->addr, regionPtr, workHeapPtr);
# endif
        } else {
            if (STM_SAME_OPID(prev, ELEM_ISGARBAGE)) {
# ifdef TM_DEBUG
                printf("\nREG_TRANSFERBAD <- ELEM_ISGARBAGE %ld\n", params->conflict.result.sint);
# endif
            }
        }
    } else if (STM_SAME_OPID(op, REG_REFINE)) {
        ASSERT(nargs == 3);
        const region_t* regionPtr = args[0].ptr;
        ASSERT(regionPtr);
        const element_t* elementPtr = args[1].ptr;
        ASSERT(elementPtr);
        const mesh_t* meshPtr = args[2].ptr;
        ASSERT(meshPtr);

        if (params->leaf == 1) {
# ifdef TM_DEBUG
            printf("\nREG_REFINE addr:%p regionPtr:%p elementPtr:%p meshPtr:%p\n", params->addr, regionPtr, elementPtr, meshPtr);
# endif
        } else {
            if (STM_SAME_OPID(prev, ELEM_ISGARBAGE)) {
                const element_t *garbageElement = (const element_t *)params->addr;

# ifdef TM_DEBUG
                printf("\nREG_REFINE elementPtr:%p <- ELEM_ISGARBAGE elementPtr:%p %ld\n", elementPtr, garbageElement, params->conflict.result.sint);
# endif
            }
        }
    } else if (STM_SAME_OPID(op, REG_GROW) || STM_SAME_OPID(op, REG_TRIANGULATE)) {
        ASSERT(nargs == 4);
        const element_t* elementPtr = args[0].ptr;
        ASSERT(elementPtr);
        const region_t* regionPtr = args[1].ptr;
        ASSERT(regionPtr);
        const mesh_t* meshPtr = args[2].ptr;
        ASSERT(meshPtr);
        const MAP_T* edgeMapPtr = args[3].ptr;
        ASSERT(edgeMapPtr);

        if (STM_SAME_OPID(op, REG_GROW)) {
            if (params->leaf == 1) {
# ifdef TM_DEBUG
                printf("\nREG_GROW addr:%p elementPtr:%p regionPtr:%p meshPtr:%p edgeMapPtr:%p\n", params->addr, elementPtr, regionPtr, meshPtr, edgeMapPtr);
# endif
            } else {
                if (STM_SAME_OPID(prev, ELEM_ISGARBAGE)) {
# ifdef TM_DEBUG
                    printf("\nREG_GROW <- ELEM_ISGARBAGE %ld\n", params->conflict.result.sint);
# endif
#ifdef MERGE_QUEUE
                } else if (STM_SAME_OPID(prev, QUEUE_POP)) {
# ifdef TM_DEBUG
                    printf("\nREG_GROW <- QUEUE_POP %p\n", params->conflict.result.ptr);
# endif
#endif /* MERGE_QUEUE */
#ifdef MERGE_LIST
                } else if (STM_SAME_OPID(prev, LIST_IT_NEXT)) {
# ifdef TM_DEBUG
                    printf("\nREG_GROW <- LIST_IT_NEXT %p\n", params->conflict.result.ptr);
# endif
#endif /* MERGE_LIST */
                }
            }
        } else {
            ASSERT(params->rv.sint >= 0);
# ifdef TM_DEBUG
            printf("\nREG_TRIANGULATE addr:%p elementPtr:%p regionPtr:%p meshPtr:%p edgeMapPtr:%p\n", params->addr, elementPtr, regionPtr, meshPtr, edgeMapPtr);
# endif
        }
    }

# ifdef TM_DEBUG
    printf("\nREGION_MERGE UNSUPPORTED addr:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}

__attribute__((constructor)) void region_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*pppp[], {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer});
    TM_LOG_TYPE_DECLARE_INIT(*ppp[], {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer});
    TM_LOG_TYPE_DECLARE_INIT(*pp[], {&ffi_type_pointer, &ffi_type_pointer});
    #define TM_LOG_OP TM_LOG_OP_INIT
    #include "region.inc"
    #undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_REGION */
