/* =============================================================================
 *
 * mesh.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "element.h"
#include "list.h"
#include "map.h"
#include "mesh.h"
#include "queue.h"
#include "random.h"
#include "set.h"
#include "tm.h"
#include "types.h"
#include "utility.h"


struct mesh {
    element_t* rootElementPtr;
    queue_t* initBadQueuePtr;
    long size;
    SET_T* boundarySetPtr;
};

#if !defined(ORIGINAL) && defined(MERGE_MESH)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "mesh.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_MESH */

/* =============================================================================
 * mesh_alloc
 * =============================================================================
 */
mesh_t*
mesh_alloc ()
{
    mesh_t* meshPtr = (mesh_t*)malloc(sizeof(mesh_t));

    if (meshPtr) {
        meshPtr->rootElementPtr = NULL;
        meshPtr->initBadQueuePtr = queue_alloc(-1);
        assert(meshPtr->initBadQueuePtr);
        meshPtr->size = 0;
        meshPtr->boundarySetPtr = SET_ALLOC(NULL, &element_listCompareEdge);
        assert(meshPtr->boundarySetPtr);
    }

    return meshPtr;
}


/* =============================================================================
 * mesh_free
 * =============================================================================
 */
void
mesh_free (mesh_t* meshPtr)
{
    queue_free(meshPtr->initBadQueuePtr);
    SET_FREE(meshPtr->boundarySetPtr, NULL);
    free(meshPtr);
}


/* =============================================================================
 * mesh_insert
 * =============================================================================
 */
void
mesh_insert (mesh_t* meshPtr, element_t* elementPtr, MAP_T* edgeMapPtr)
{
    /*
     * Assuming fully connected graph, we just need to record one element.
     * The root element is not needed for the actual refining, but rather
     * for checking the validity of the final mesh.
     */
    if (!meshPtr->rootElementPtr) {
        meshPtr->rootElementPtr = elementPtr;
    }

    /*
     * Record existence of each of this element's edges
     */
    long i;
    long numEdge = element_getNumEdge(elementPtr);
    for (i = 0; i < numEdge; i++) {
        edge_t* edgePtr = element_getEdge(elementPtr, i);
        if (!MAP_CONTAINS(edgeMapPtr, (void*)edgePtr)) {
            /*
             * Record existence of this edge
             */
            bool_t isSuccess;
            isSuccess =
                MAP_INSERT(edgeMapPtr, (void*)edgePtr, (void*)elementPtr);
            assert(isSuccess);
        } else {
            /*
             * Shared edge; update each element's neighborList
             */
            bool_t isSuccess;
            element_t* sharerPtr = (element_t*)MAP_FIND(edgeMapPtr, edgePtr);
            assert(sharerPtr); /* cannot be shared by >2 elements */
            element_addNeighbor(elementPtr, sharerPtr);
            element_addNeighbor(sharerPtr, elementPtr);
            isSuccess = MAP_REMOVE(edgeMapPtr, edgePtr);
            assert(isSuccess);
            isSuccess = MAP_INSERT(edgeMapPtr,
                                   edgePtr,
                                   NULL); /* marker to check >2 sharers */
            assert(isSuccess);
        }
    }

    /*
     * Check if really encroached
     */

    edge_t* encroachedPtr = element_getEncroachedPtr(elementPtr);
    if (encroachedPtr) {
        if (!SET_CONTAINS(meshPtr->boundarySetPtr, encroachedPtr)) {
            element_clearEncroached(elementPtr);
        }
    }
}


/* =============================================================================
 * HTMmesh_insert
 * =============================================================================
 */
void
HTMmesh_insert (mesh_t* meshPtr, element_t* elementPtr, MAP_T* edgeMapPtr)
{
    /*
     * Assuming fully connected graph, we just need to record one element.
     * The root element is not needed for the actual refining, but rather
     * for checking the validity of the final mesh.
     */
    if (!HTM_SHARED_READ_P(meshPtr->rootElementPtr)) {
        HTM_SHARED_WRITE_P(meshPtr->rootElementPtr, elementPtr);
    }

    /*
     * Record existence of each of this element's edges
     */
    long i;
    long numEdge = element_getNumEdge(elementPtr);
    for (i = 0; i < numEdge; i++) {
        edge_t* edgePtr = element_getEdge(elementPtr, i);
        if (!HTMMAP_CONTAINS(edgeMapPtr, (void*)edgePtr)) {
            /* Record existance of this edge */
            bool_t isSuccess;
            isSuccess =
                HTMMAP_INSERT(edgeMapPtr, (void*)edgePtr, (void*)elementPtr);
            assert(isSuccess);
        } else {
            /*
             * Shared edge; update each element's neighborList
             */
            bool_t isSuccess;
            element_t* sharerPtr = (element_t*)HTMMAP_FIND(edgeMapPtr, edgePtr);
            assert(sharerPtr); /* cannot be shared by >2 elements */
            if (!sharerPtr)
                HTM_RESTART();
            HTMELEMENT_ADDNEIGHBOR(elementPtr, sharerPtr);
            HTMELEMENT_ADDNEIGHBOR(sharerPtr, elementPtr);
            isSuccess = HTMMAP_REMOVE(edgeMapPtr, edgePtr);
            assert(isSuccess);
            isSuccess = HTMMAP_INSERT(edgeMapPtr,
                                    edgePtr,
                                    NULL); /* marker to check >2 sharers */
            assert(isSuccess);
        }
    }

    /*
     * Check if really encroached
     */

    edge_t* encroachedPtr = element_getEncroachedPtr(elementPtr);
    if (encroachedPtr) {
        if (!HTMSET_CONTAINS(meshPtr->boundarySetPtr, encroachedPtr)) {
            element_clearEncroached(elementPtr);
        }
    }
}


/* =============================================================================
 * TMmesh_insert
 * =============================================================================
 */
TM_CANCELLABLE
void
TMmesh_insert (TM_ARGDECL
               mesh_t* meshPtr, element_t* elementPtr, MAP_T* edgeMapPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_MESH)
    TM_LOG_BEGIN(MESH_INSERT, NULL, meshPtr, elementPtr, edgeMapPtr);
#endif /* !ORIGINAL && MERGE_MESH */

    /*
     * Assuming fully connected graph, we just need to record one element.
     * The root element is not needed for the actual refining, but rather
     * for checking the validity of the final mesh.
     */
    if (!TM_SHARED_READ_TAG_P(meshPtr->rootElementPtr, meshPtr)) {
        TM_SHARED_WRITE_P(meshPtr->rootElementPtr, elementPtr);
    }

    /*
     * Record existence of each of this element's edges
     */
    long i;
    long numEdge = element_getNumEdge(elementPtr);
    for (i = 0; i < numEdge; i++) {
        edge_t* edgePtr = element_getEdge(elementPtr, i);
        if (!TMMAP_CONTAINS(edgeMapPtr, (void*)edgePtr)) {
            /* Record existance of this edge */
            bool_t isSuccess;
            isSuccess =
                TMMAP_INSERT(edgeMapPtr, (void*)edgePtr, (void*)elementPtr);
            assert(isSuccess);
        } else {
            /*
             * Shared edge; update each element's neighborList
             */
            bool_t isSuccess;
            element_t* sharerPtr = (element_t*)TMMAP_FIND(edgeMapPtr, edgePtr);
            assert(sharerPtr); /* cannot be shared by >2 elements */
            if (!sharerPtr)
                TM_RESTART();
            TMELEMENT_ADDNEIGHBOR(elementPtr, sharerPtr);
            TMELEMENT_ADDNEIGHBOR(sharerPtr, elementPtr);
            isSuccess = TMMAP_REMOVE(edgeMapPtr, edgePtr);
            assert(isSuccess);
            isSuccess = TMMAP_INSERT(edgeMapPtr,
                                    edgePtr,
                                    NULL); /* marker to check >2 sharers */
            assert(isSuccess);
        }
    }

    /*
     * Check if really encroached
     */

    edge_t* encroachedPtr = element_getEncroachedPtr(elementPtr);
    if (encroachedPtr) {
        if (!TMSET_CONTAINS(meshPtr->boundarySetPtr, encroachedPtr)) {
            element_clearEncroached(elementPtr);
        }
    }

#if !defined(ORIGINAL) && defined(MERGE_MESH)
    TM_LOG_END(MESH_INSERT, NULL);
#endif /* !ORIGINAL && MERGE_MESH */
}


/* =============================================================================
 * mesh_remove
 * =============================================================================
 */
void
mesh_remove (TM_ARGDECL  mesh_t* meshPtr, element_t* elementPtr)
{
    assert(!element_isGarbage(elementPtr));

    /*
     * If removing root, a new root is selected on the next mesh_insert, which
     * always follows a call a mesh_remove.
     */
    if (meshPtr->rootElementPtr == elementPtr) {
        meshPtr->rootElementPtr = NULL;
    }

    /*
     * Remove from neighbors
     */
    list_iter_t it;
    list_t* neighborListPtr = element_getNeighborListPtr(elementPtr);
    list_iter_reset(&it, neighborListPtr);
    while (list_iter_hasNext(&it, neighborListPtr)) {
        element_t* neighborPtr =
            (element_t*)list_iter_next(&it, neighborListPtr);
        bool_t status = element_removeNeighbor(neighborPtr, elementPtr);
        assert(status);
    }

    element_setIsGarbage(elementPtr, TRUE);

    if (!element_isReferenced(elementPtr)) {
        element_free(elementPtr);
    }
}


/* =============================================================================
 * HTMmesh_remove
 * =============================================================================
 */
void
HTMmesh_remove (mesh_t* meshPtr, element_t* elementPtr)
{
    assert(!HTMELEMENT_ISGARBAGE(elementPtr));

    /*
     * If removing root, a new root is selected on the next mesh_insert, which
     * always follows a call a mesh_remove.
     */
    if ((element_t*)HTM_SHARED_READ_P(meshPtr->rootElementPtr) == elementPtr) {
        HTM_SHARED_WRITE_P(meshPtr->rootElementPtr, NULL);
    }

    /*
     * Remove from neighbors
     */
    list_iter_t it;
    list_t* neighborListPtr = element_getNeighborListPtr(elementPtr);
    HTMLIST_ITER_RESET(&it, neighborListPtr);
    while (HTMLIST_ITER_HASNEXT(&it, neighborListPtr)) {
        element_t* neighborPtr =
            (element_t*)HTMLIST_ITER_NEXT(&it, neighborListPtr);
        bool_t status = HTMELEMENT_REMOVENEIGHBOR(neighborPtr, elementPtr);
        assert(status);
    }

    HTMELEMENT_SETISGARBAGE(elementPtr, TRUE);

    if (!HTMELEMENT_ISREFERENCED(elementPtr)) {
        HTMELEMENT_FREE(elementPtr);
    }
}


/* =============================================================================
 * TMmesh_remove
 * =============================================================================
 */
TM_CALLABLE
void
TMmesh_remove (TM_ARGDECL  mesh_t* meshPtr, element_t* elementPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_MESH)
    TM_LOG_BEGIN(MESH_REMOVE, NULL, meshPtr, elementPtr);
#endif /* !ORIGINAL && MERGE_MESH */

    assert(!TMELEMENT_ISGARBAGE(elementPtr));

    /*
     * If removing root, a new root is selected on the next mesh_insert, which
     * always follows a call a mesh_remove.
     */
    if ((element_t*)TM_SHARED_READ_TAG_P(meshPtr->rootElementPtr, meshPtr) == elementPtr) {
        TM_SHARED_WRITE_P(meshPtr->rootElementPtr, NULL);
    }

    /*
     * Remove from neighbors
     */
    list_iter_t it;
    list_t* neighborListPtr = element_getNeighborListPtr(elementPtr);
    TMLIST_ITER_RESET(&it, neighborListPtr);
    while (TMLIST_ITER_HASNEXT(&it, neighborListPtr)) {
        element_t* neighborPtr =
            (element_t*)TMLIST_ITER_NEXT(&it, neighborListPtr);
        bool_t status = TMELEMENT_REMOVENEIGHBOR(neighborPtr, elementPtr);
        assert(status);
    }

    TMELEMENT_SETISGARBAGE(elementPtr, TRUE);

    if (!TMELEMENT_ISREFERENCED(elementPtr)) {
        TMELEMENT_FREE(elementPtr);
    }

#if !defined(ORIGINAL) && defined(MERGE_MESH)
    TM_LOG_END(MESH_REMOVE, NULL);
#endif /* !ORIGINAL && MERGE_MESH */
}


/* =============================================================================
 * mesh_insertBoundary
 * =============================================================================
 */
bool_t
mesh_insertBoundary (mesh_t* meshPtr, edge_t* boundaryPtr)
{
    bool_t rv = SET_INSERT(meshPtr->boundarySetPtr, boundaryPtr);
    return rv;
}


/* =============================================================================
 * HTMmesh_insertBoundary
 * =============================================================================
 */
bool_t
HTMmesh_insertBoundary (mesh_t* meshPtr, edge_t* boundaryPtr)
{
    bool_t rv = HTMSET_INSERT(meshPtr->boundarySetPtr, boundaryPtr);
    return rv;
}


/* =============================================================================
 * TMmesh_insertBoundary
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMmesh_insertBoundary (TM_ARGDECL  mesh_t* meshPtr, edge_t* boundaryPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_MESH)
    TM_LOG_BEGIN(MESH_INSERTBOUND, NULL, meshPtr, boundaryPtr);
#endif /* !ORIGINAL && MERGE_MESH */
    bool_t rv = TMSET_INSERT(meshPtr->boundarySetPtr, boundaryPtr);
#if !defined(ORIGINAL) && defined(MERGE_MESH)
    TM_LOG_END(MESH_INSERTBOUND, &rv);
#endif /* !ORIGINAL && MERGE_MESH */
    return rv;
}


/* =============================================================================
 * mesh_removeBoundary
 * =============================================================================
 */
bool_t
mesh_removeBoundary (mesh_t* meshPtr, edge_t* boundaryPtr)
{
    bool_t rv = SET_REMOVE(meshPtr->boundarySetPtr, boundaryPtr);
    return rv;
}


/* =============================================================================
 * HTMmesh_removeBoundary
 * =============================================================================
 */
bool_t
HTMmesh_removeBoundary (mesh_t* meshPtr, edge_t* boundaryPtr)
{
    bool_t rv = HTMSET_REMOVE(meshPtr->boundarySetPtr, boundaryPtr);
    return rv;
}


/* =============================================================================
 * TMmesh_removeBoundary
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMmesh_removeBoundary (TM_ARGDECL  mesh_t* meshPtr, edge_t* boundaryPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_MESH)
    TM_LOG_BEGIN(MESH_REMOVEBOUND, NULL, meshPtr, boundaryPtr);
#endif /* !ORIGINAL && MERGE_MESH */
    bool_t rv = TMSET_REMOVE(meshPtr->boundarySetPtr, boundaryPtr);
#if !defined(ORIGINAL) && defined(MERGE_MESH)
    TM_LOG_END(MESH_REMOVEBOUND, &rv);
#endif /* !ORIGINAL && MERGE_MESH */
    return rv;
}


/* =============================================================================
 * createElement
 * =============================================================================
 */
static void
createElement (mesh_t* meshPtr,
               coordinate_t* coordinates,
               long numCoordinate,
               MAP_T* edgeMapPtr)
{
    element_t* elementPtr = element_alloc(coordinates, numCoordinate);
    assert(elementPtr);

    if (numCoordinate == 2) {
        edge_t* boundaryPtr = element_getEdge(elementPtr, 0);
        bool_t status = SET_INSERT(meshPtr->boundarySetPtr, boundaryPtr);
        assert(status);
    }

    mesh_insert(meshPtr, elementPtr, edgeMapPtr);

    if (element_isBad(elementPtr)) {
        bool_t status = queue_push(meshPtr->initBadQueuePtr, (void*)elementPtr);
        assert(status);
    }
 }


/* =============================================================================
 * mesh_read
 *
 * Returns number of elements read from file
 *
 * Refer to http://www.cs.cmu.edu/~quake/triangle.html for file formats.
 * =============================================================================
 */
long
mesh_read (mesh_t* meshPtr, char* fileNamePrefix)
{
    FILE* inputFile;
    coordinate_t* coordinates;
    char fileName[256];
    long fileNameSize = sizeof(fileName) / sizeof(fileName[0]);
    char inputBuff[256];
    long inputBuffSize = sizeof(inputBuff) / sizeof(inputBuff[0]);
    long numEntry;
    long numDimension;
    long numCoordinate;
    long i;
    long numElement = 0;
    char *ret;

    MAP_T* edgeMapPtr = MAP_ALLOC(NULL, &element_mapCompareEdge);
    assert(edgeMapPtr);

    /*
     * Read .node file
     */
    snprintf(fileName, fileNameSize, "%s.node", fileNamePrefix);
    inputFile = fopen(fileName, "r");
    assert(inputFile);
    ret = fgets(inputBuff, inputBuffSize, inputFile);
    assert(ret == inputBuff);
    sscanf(inputBuff, "%li %li", &numEntry, &numDimension);
    assert(numDimension == 2); /* must be 2-D */
    numCoordinate = numEntry + 1; /* numbering can start from 1 */
    coordinates = (coordinate_t*)malloc(numCoordinate * sizeof(coordinate_t));
    assert(coordinates);
    for (i = 0; i < numEntry; i++) {
        long id;
        double x;
        double y;
        if (!fgets(inputBuff, inputBuffSize, inputFile)) {
            break;
        }
        if (inputBuff[0] == '#') {
            continue; /* TODO: handle comments correctly */
        }
        sscanf(inputBuff, "%li %lf %lf", &id, &x, &y);
        coordinates[id].x = x;
        coordinates[id].y = y;
    }
    assert(i == numEntry);
    fclose(inputFile);

    /*
     * Read .poly file, which contains boundary segments
     */
    snprintf(fileName, fileNameSize, "%s.poly", fileNamePrefix);
    inputFile = fopen(fileName, "r");
    assert(inputFile);
    ret = fgets(inputBuff, inputBuffSize, inputFile);
    assert(ret == inputBuff);
    sscanf(inputBuff, "%li %li", &numEntry, &numDimension);
    assert(numEntry == 0); /* .node file used for vertices */
    assert(numDimension == 2); /* must be edge */
    ret = fgets(inputBuff, inputBuffSize, inputFile);
    assert(ret == inputBuff);
    sscanf(inputBuff, "%li", &numEntry);
    for (i = 0; i < numEntry; i++) {
        long id;
        long a;
        long b;
        coordinate_t insertCoordinates[2];
        if (!fgets(inputBuff, inputBuffSize, inputFile)) {
            break;
        }
        if (inputBuff[0] == '#') {
            continue; /* TODO: handle comments correctly */
        }
        sscanf(inputBuff, "%li %li %li", &id, &a, &b);
        assert(a >= 0 && a < numCoordinate);
        assert(b >= 0 && b < numCoordinate);
        insertCoordinates[0] = coordinates[a];
        insertCoordinates[1] = coordinates[b];
        createElement(meshPtr, insertCoordinates, 2, edgeMapPtr);
    }
    assert(i == numEntry);
    numElement += numEntry;
    fclose(inputFile);

    /*
     * Read .ele file, which contains triangles
     */
    snprintf(fileName, fileNameSize, "%s.ele", fileNamePrefix);
    inputFile = fopen(fileName, "r");
    assert(inputFile);
    ret = fgets(inputBuff, inputBuffSize, inputFile);
    assert(ret == inputBuff);
    sscanf(inputBuff, "%li %li", &numEntry, &numDimension);
    assert(numDimension == 3); /* must be triangle */
    for (i = 0; i < numEntry; i++) {
        long id;
        long a;
        long b;
        long c;
        coordinate_t insertCoordinates[3];
        if (!fgets(inputBuff, inputBuffSize, inputFile)) {
            break;
        }
        if (inputBuff[0] == '#') {
            continue; /* TODO: handle comments correctly */
        }
        sscanf(inputBuff, "%li %li %li %li", &id, &a, &b, &c);
        assert(a >= 0 && a < numCoordinate);
        assert(b >= 0 && b < numCoordinate);
        assert(c >= 0 && c < numCoordinate);
        insertCoordinates[0] = coordinates[a];
        insertCoordinates[1] = coordinates[b];
        insertCoordinates[2] = coordinates[c];
        createElement(meshPtr, insertCoordinates, 3, edgeMapPtr);
    }
    assert(i == numEntry);
    numElement += numEntry;
    fclose(inputFile);

    free(coordinates);
    MAP_FREE(edgeMapPtr, NULL);

    return numElement;
}


/* =============================================================================
 * mesh_getBad
 * -- Returns NULL if none
 * =============================================================================
 */
element_t*
mesh_getBad (mesh_t* meshPtr)
{
    return (element_t*)queue_pop(meshPtr->initBadQueuePtr);
}


/* =============================================================================
 * mesh_shuffleBad
 * =============================================================================
 */
void
mesh_shuffleBad (mesh_t* meshPtr, random_t* randomPtr)
{
    queue_shuffle(meshPtr->initBadQueuePtr, randomPtr);
}


/* =============================================================================
 * value_free
 * =============================================================================
 */
void mesh_mapElementFree(void *k, void *v) {
    Pelement_free(k);
}


/* =============================================================================
 * mesh_check
 * =============================================================================
 */
bool_t
mesh_check (mesh_t* meshPtr, long expectedNumElement)
{
    queue_t* searchQueuePtr;
    MAP_T* visitedMapPtr;
    long numBadTriangle = 0;
    long numFalseNeighbor = 0;
    long numElement = 0;

    puts("Checking final mesh:");
    fflush(stdout);

    searchQueuePtr = queue_alloc(-1);
    assert(searchQueuePtr);
    visitedMapPtr = MAP_ALLOC(NULL, &element_mapCompare);
    assert(visitedMapPtr);

    /*
     * Do breadth-first search starting from rootElementPtr
     */
    assert(meshPtr->rootElementPtr);
    queue_push(searchQueuePtr, (void*)meshPtr->rootElementPtr);
    while (!queue_isEmpty(searchQueuePtr)) {

        element_t* currentElementPtr;
        list_iter_t it;
        list_t* neighborListPtr;
        bool_t isSuccess;

        currentElementPtr = (element_t*)queue_pop(searchQueuePtr);
        if (MAP_CONTAINS(visitedMapPtr, (void*)currentElementPtr)) {
            continue;
        }
        isSuccess = MAP_INSERT(visitedMapPtr, (void*)currentElementPtr, NULL);
        assert(isSuccess);
        if (element_isGarbage(currentElementPtr) || !element_checkAngles(currentElementPtr)) {
            numBadTriangle++;
        }
        if (element_isGarbage(currentElementPtr)) {
            continue;
        }
        neighborListPtr = element_getNeighborListPtr(currentElementPtr);

        list_iter_reset(&it, neighborListPtr);
        while (list_iter_hasNext(&it, neighborListPtr)) {
            element_t* neighborElementPtr =
                (element_t*)list_iter_next(&it, neighborListPtr);
            /*
             * Continue breadth-first search
             */
            if (!MAP_CONTAINS(visitedMapPtr, (void*)neighborElementPtr)) {
                isSuccess = queue_push(searchQueuePtr,
                                       (void*)neighborElementPtr);
                assert(isSuccess);
            }
        } /* for each neighbor */
        numElement++;

    } /* breadth-first search */

    printf("Number of elements (expected) = %li (%li)\n", numElement, expectedNumElement);
    printf("Number of bad triangles = %li\n", numBadTriangle);

    queue_free(searchQueuePtr);
    MAP_FREE(visitedMapPtr, mesh_mapElementFree);

    return ((numBadTriangle > 0 ||
             numFalseNeighbor > 0 ||
             numElement != expectedNumElement) ? FALSE : TRUE);
}


#ifdef TEST_MESH
/* =============================================================================
 * TEST_MESH
 * =============================================================================
 */


#include <stdio.h>


int
main (int argc, char* argv[])
{
    mesh_t* meshPtr;

    assert(argc == 2);

    puts("Starting tests...");

    meshPtr = mesh_alloc();
    assert(meshPtr);

    mesh_read(meshPtr, argv[1]);

    mesh_free(meshPtr);

    puts("All tests passed.");

    return 0;
}


#endif /* TEST_MESH */


/* =============================================================================
 *
 * End of mesh.c
 *
 * =============================================================================
 */

#if !defined(ORIGINAL) && defined(MERGE_MESH)
stm_merge_t TMmesh_merge(stm_merge_context_t *params) {
    const stm_op_id_t op = stm_get_op_opcode(params->current);
    const stm_op_id_t prev = params->leaf ? STM_INVALID_OPID : stm_get_op_opcode(params->previous);

    const stm_union_t *args;
    const ssize_t nargs = stm_get_op_args(params->current, &args);

    if (STM_SAME_OPID(op, MESH_REMOVE)) {
        const mesh_t* meshPtr = (nargs == 3) ? args[0].ptr : NULL;
        element_t* elementPtr = (nargs == 3) ? args[1].ptr : NULL;

        if (params->leaf == 1) {
            ASSERT(ENTRY_VALID(params->conflict.entries->e1));
            const stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);

            meshPtr = (const mesh_t *)TM_SHARED_GET_TAG(r);
            ASSERT(params->addr == &meshPtr->rootElementPtr);

# ifdef TM_DEBUG
            printf("\nMESH_REMOVE addr:%p meshPtr:%p elementPtr:%p\n", params->addr, meshPtr, elementPtr);
# endif
            const element_t *old, *new;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, meshPtr->rootElementPtr, old));
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, meshPtr->rootElementPtr, new));
# ifdef TM_DEBUG
            printf("MESH_REMOVE meshPtr->rootElementPtr:%p (old):%p (new):%p\n", &meshPtr->rootElementPtr, old, new);
# endif
            params->conflict.previous_result.ptr = (void *)old;
            params->rv.ptr = (void *)new;
            return STM_MERGE_OK;
        } else {
#ifdef MERGE_ELEMENT
            extern const stm_op_id_t ELEM_ISREFERENCED;
#endif /* MERGE_ELEMENT */
#ifdef MERGE_LIST
            extern const stm_op_id_t LIST_IT_NEXT;
#endif /* MERGE_LIST */
            if (0) {
#ifdef MERGE_ELEMENT
            } else if (STM_SAME_OPID(prev, ELEM_ISREFERENCED)) {
                elementPtr = (element_t *)params->addr;
# ifdef TM_DEBUG
                printf("\nMESH_REMOVE <- ELEM_ISREFERENCED %ld\n", params->conflict.result.sint);
# endif
                if (!params->conflict.result.sint)
                    TMELEMENT_FREE(elementPtr);
                return STM_MERGE_OK;
#endif /* MERGE_ELEMENT */
#ifdef MERGE_LIST
            } else if (STM_SAME_OPID(prev, LIST_IT_NEXT)) {
# ifdef TM_DEBUG
                printf("\nMESH_REMOVE <- LIST_IT_NEXT %p\n", params->conflict.result.ptr);
# endif
#endif /* MERGE_LIST */
            }
        }
    } else if (STM_SAME_OPID(op, MESH_INSERTBOUND) || STM_SAME_OPID(op, MESH_REMOVEBOUND)) {
        const mesh_t* meshPtr = (nargs == 2) ? args[0].ptr : NULL;
        const edge_t* boundaryPtr = (nargs == 2) ? args[1].ptr : NULL;
        ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);

        if (STM_SAME_OPID(op, MESH_INSERTBOUND)) {
# ifdef TM_DEBUG
            printf("\nMESH_INSERTBOUND addr:%p meshPtr:%p boundaryPtr:%p\n", params->addr, meshPtr, boundaryPtr);
# endif
        } else {
# ifdef MERGE_RBTREE
            extern const stm_op_id_t RBTREE_DELETE;

            ASSERT(STM_SAME_OPID(op, MESH_REMOVEBOUND));
            ASSERT(STM_SAME_OPID(prev, RBTREE_DELETE));
#  ifdef TM_DEBUG
            printf("\nMESH_REMOVEBOUND <- RBTREE_DELETE %ld\n", params->conflict.result.sint);
            printf("MESH_REMOVEBOUND rv(old):%ld rv(new):%ld\n", params->rv.sint, params->conflict.result.sint);
#  endif
            params->rv.sint = params->conflict.result.sint;
            return STM_MERGE_OK;
#endif /* MERGE_RBTREE */
        }
    } else if (STM_SAME_OPID(op, MESH_INSERT)) {
        const mesh_t* meshPtr = (nargs == 3) ? args[0].ptr : NULL;
        element_t* elementPtr = (nargs == 3) ? args[1].ptr : NULL;
        const MAP_T* edgeMapPtr = (nargs == 3) ? args[2].ptr : NULL;

        if (params->leaf == 1) {
            ASSERT(ENTRY_VALID(params->conflict.entries->e1));
            const stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);

            meshPtr = (const mesh_t *)TM_SHARED_GET_TAG(r);
            ASSERT(params->addr == &meshPtr->rootElementPtr);

# ifdef TM_DEBUG
            printf("\nMESH_INSERT addr:%p meshPtr:%p elementPtr:%p edgeMapPtr:%p\n", params->addr, meshPtr, elementPtr, edgeMapPtr);
# endif
            const element_t *old, *new;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, meshPtr->rootElementPtr, old));
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, meshPtr->rootElementPtr, new));
# ifdef TM_DEBUG
            printf("MESH_INSERT meshPtr->rootElementPtr:%p (old):%p (new):%p\n", &meshPtr->rootElementPtr, old, new);
# endif
            params->conflict.previous_result.ptr = (void *)old;
            params->rv.ptr = (void *)new;
            return STM_MERGE_OK;
        }
    }

# ifdef TM_DEBUG
    printf("\nMESH_MERGE UNSUPPORTED addr:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}

__attribute__((constructor)) void mesh_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*ppp[], {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer});
    TM_LOG_TYPE_DECLARE_INIT(*pp[], {&ffi_type_pointer, &ffi_type_pointer});
    #define TM_LOG_OP TM_LOG_OP_INIT
    #include "mesh.inc"
    #undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_MESH */
