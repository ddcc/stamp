/* =============================================================================
 *
 * list.c
 * -- Sorted singly linked list
 * -- Options: -DLIST_NO_DUPLICATES (default: allow duplicates)
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


#include <stdlib.h>
#include <assert.h>
#include "list.h"
#include "types.h"
#include "tm.h"

#ifndef ORIGINAL
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "list.inc"
# undef TM_LOG_OP

#define TMLIST_BAD_ADDR                 (void *)-1
#endif /* ORIGINAL */

/* =============================================================================
 * DECLARATION OF TM_CALLABLE FUNCTIONS
 * =============================================================================
 */

TM_CALLABLE
static list_node_t*
TMfindPrevious (TM_ARGDECL  list_t* listPtr, void* dataPtr);

TM_CALLABLE
static void
TMfreeList (TM_ARGDECL  list_node_t* nodePtr, TM_CALLABLE void (*TMfreeData)(void *));

TM_CALLABLE
void
TMlist_free (TM_ARGDECL  list_t* listPtr, TM_CALLABLE void (*TMfreeData)(void *));

/* =============================================================================
 * list_compare
 * -- Default compare function
 * =============================================================================
 */
long
list_compare (const void* a, const void* b)
{
    return ((long)a - (long)b);
}

/* =============================================================================
 * list_iter_reset
 * =============================================================================
 */
void
list_iter_reset (list_iter_t* itPtr, list_t* listPtr)
{
    *itPtr = &(listPtr->head);
}


/* =============================================================================
 * HTMlist_iter_reset
 * =============================================================================
 */
void
HTMlist_iter_reset (list_iter_t* itPtr, list_t* listPtr)
{
    TM_LOCAL_WRITE_P(*itPtr, &(listPtr->head));
}


/* =============================================================================
 * TMlist_iter_reset
 * =============================================================================
 */
TM_CALLABLE
void
TMlist_iter_reset (TM_ARGDECL  list_iter_t* itPtr, list_t* listPtr)
{
    TM_LOCAL_WRITE_P(*itPtr, &(listPtr->head));
}


/* =============================================================================
 * list_iter_hasNext
 * =============================================================================
 */
bool_t
list_iter_hasNext (list_iter_t* itPtr, list_t* listPtr)
{
    return (((*itPtr)->nextPtr != NULL) ? TRUE : FALSE);
}


/* =============================================================================
 * HTMlist_iter_hasNext
 * =============================================================================
 */
bool_t
HTMlist_iter_hasNext (list_iter_t* itPtr, list_t* listPtr)
{
    bool_t rv;
    list_iter_t next = (list_iter_t)HTM_SHARED_READ_P((*itPtr)->nextPtr);
    rv = (next != NULL) ? TRUE : FALSE;
    return rv;
}


/* =============================================================================
 * TMlist_iter_hasNext
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMlist_iter_hasNext (TM_ARGDECL  list_iter_t* itPtr, list_t* listPtr)
{
    bool_t rv;
#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_IT_HASNEXT, NULL, *itPtr, listPtr);
#endif /* ORIGINAL */
    list_iter_t next = (list_iter_t)TM_SHARED_READ_TAG_P((*itPtr)->nextPtr, (uintptr_t)*itPtr);
    rv = (next != NULL) ? TRUE : FALSE;
#ifndef ORIGINAL
    TM_LOG_END(LIST_IT_HASNEXT, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * list_iter_next
 * =============================================================================
 */
void*
list_iter_next (list_iter_t* itPtr, list_t* listPtr)
{
    *itPtr = (*itPtr)->nextPtr;

    return (*itPtr)->dataPtr;
}


/* =============================================================================
 * HTMlist_iter_next
 * =============================================================================
 */
void*
HTMlist_iter_next (list_iter_t* itPtr, list_t* listPtr)
{
    list_iter_t next = (list_iter_t)HTM_SHARED_READ_P((*itPtr)->nextPtr);
    HTM_LOCAL_WRITE_P(*itPtr, next);

    void *r = HTM_SHARED_READ_P(next->dataPtr);
    return r;
}


/* =============================================================================
 * TMlist_iter_next
 * =============================================================================
 */
TM_CALLABLE
void*
TMlist_iter_next (TM_ARGDECL  list_iter_t* itPtr, list_t* listPtr)
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_IT_NEXT, NULL, *itPtr, listPtr);
#endif /* ORIGINAL */
    list_iter_t next = (list_iter_t)TM_SHARED_READ_TAG_P((*itPtr)->nextPtr, (uintptr_t)*itPtr);
    TM_LOCAL_WRITE_P(*itPtr, next);

    void *r = TM_SHARED_READ_TAG_P(next->dataPtr, (uintptr_t)next);
#ifndef ORIGINAL
    if (__builtin_expect(r == TMLIST_BAD_ADDR, 0))
        TM_RESTART();
    TM_LOG_END(LIST_IT_NEXT, &r);
#endif /* ORIGINAL */
    return r;
}


/* =============================================================================
 * allocNode
 * -- Returns NULL on failure
 * =============================================================================
 */
static list_node_t*
allocNode (void* dataPtr)
{
    list_node_t* nodePtr = (list_node_t*)malloc(sizeof(list_node_t));
    if (nodePtr == NULL) {
        return NULL;
    }

    nodePtr->dataPtr = dataPtr;
    nodePtr->nextPtr = NULL;

    return nodePtr;
}


/* =============================================================================
 * PallocNode
 * -- Returns NULL on failure
 * =============================================================================
 */
static list_node_t*
PallocNode (void* dataPtr)
{
    list_node_t* nodePtr = (list_node_t*)P_MALLOC(sizeof(list_node_t));
    if (nodePtr == NULL) {
        return NULL;
    }

    nodePtr->dataPtr = dataPtr;
    nodePtr->nextPtr = NULL;

    return nodePtr;
}


/* =============================================================================
 * HTMallocNode
 * -- Returns NULL on failure
 * =============================================================================
 */
static list_node_t*
HTMallocNode (void* dataPtr)
{
    list_node_t* nodePtr = (list_node_t*)HTM_MALLOC(sizeof(list_node_t));
    if (nodePtr == NULL) {
        return NULL;
    }

    nodePtr->dataPtr = dataPtr;
    nodePtr->nextPtr = NULL;

    return nodePtr;
}


/* =============================================================================
 * TMallocNode
 * -- Returns NULL on failure
 * =============================================================================
 */
static list_node_t*
TMallocNode (TM_ARGDECL  void* dataPtr)
{
    list_node_t* nodePtr = (list_node_t*)TM_MALLOC(sizeof(list_node_t));
    if (nodePtr == NULL) {
        return NULL;
    }

    nodePtr->dataPtr = dataPtr;
    nodePtr->nextPtr = NULL;

    return nodePtr;
}


/* =============================================================================
 * list_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
list_t*
list_alloc (long (*compare)(const void*, const void*))
{
    list_t* listPtr = (list_t*)malloc(sizeof(list_t));
    if (listPtr == NULL) {
        return NULL;
    }

    listPtr->head.dataPtr = NULL;
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;

    list_setCompare(listPtr, compare ? compare : list_compare);

    return listPtr;
}


/* =============================================================================
 * Plist_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
list_t*
Plist_alloc (long (*compare)(const void*, const void*))
{
    list_t* listPtr = (list_t*)P_MALLOC(sizeof(list_t));
    if (listPtr == NULL) {
        return NULL;
    }

    listPtr->head.dataPtr = NULL;
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;

    list_setCompare(listPtr, compare ? compare : list_compare);

    return listPtr;
}


/* =============================================================================
 * HTMlist_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
list_t*
HTMlist_alloc (long (*compare)(const void*, const void*))
{
    list_t* listPtr = (list_t*)HTM_MALLOC(sizeof(list_t));
    if (listPtr == NULL)
        goto out;

    listPtr->head.dataPtr = NULL;
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;
    list_setCompare(listPtr, compare ? compare : list_compare);

out:
    return listPtr;
}


/* =============================================================================
 * TMlist_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
TM_CALLABLE
list_t*
TMlist_alloc (TM_ARGDECL  long (*compare)(const void*, const void*))
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_ALLOC, NULL, compare);
#endif /* ORIGINAL */

    list_t* listPtr = (list_t*)TM_MALLOC(sizeof(list_t));
    if (listPtr == NULL)
        goto out;

    listPtr->head.dataPtr = NULL;
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;
    list_setCompare(listPtr, compare ? compare : list_compare);

out:
#ifndef ORIGINAL
    TM_LOG_END(LIST_ALLOC, &listPtr);
#endif /* ORIGINAL */
    return listPtr;
}


/* =============================================================================
 * freeNode
 * =============================================================================
 */
static void
freeNode (list_node_t* nodePtr)
{
    free(nodePtr);
}


/* =============================================================================
 * PfreeNode
 * =============================================================================
 */
static void
PfreeNode (list_node_t* nodePtr)
{
    P_FREE(nodePtr);
}


/* =============================================================================
 * HTMfreeNode
 * =============================================================================
 */
static void
HTMfreeNode (list_node_t* nodePtr)
{
    HTM_FREE(nodePtr);
}


/* =============================================================================
 * TMfreeNode
 * =============================================================================
 */
static void
TMfreeNode (TM_ARGDECL  list_node_t* nodePtr)
{
#if !defined(ORIGINAL) && defined(MERGE_LIST)
    TM_SHARED_WRITE_P(nodePtr->dataPtr, TMLIST_BAD_ADDR);
    TM_SHARED_WRITE_P(nodePtr->nextPtr, TMLIST_BAD_ADDR);
#endif /* !ORIGINAL && MERGE_LIST */
    TM_FREE(nodePtr);
}


#if !defined(ORIGINAL) && defined(MERGE_LIST)
/* =============================================================================
 * TMfreeNodeUndo
 * =============================================================================
 */
static stm_merge_t
TMfreeNodeUndo (TM_ARGDECL  const list_node_t* nodePtr)
{
    void *val;
    stm_write_t w;
    stm_free_t f;

    w = TM_SHARED_DID_WRITE(nodePtr->dataPtr);
    if (STM_VALID_WRITE(w)) {
        ASSERT(TM_SHARED_WRITE_VALUE_P(w, nodePtr->dataPtr, val) && val == TMLIST_BAD_ADDR);
        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
    }

    w = TM_SHARED_DID_WRITE(nodePtr->nextPtr);
    if (STM_VALID_WRITE(w)) {
        ASSERT(TM_SHARED_WRITE_VALUE_P(w, nodePtr->nextPtr, val) && val == TMLIST_BAD_ADDR);
        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
    }

    f = TM_DID_FREE(nodePtr);
    if (STM_VALID_FREE(f))
        ASSERT_FAIL(TM_UNDO_FREE(f));

    return STM_MERGE_OK;
}
#endif /* !ORIGINAL && MERGE_LIST */


/* =============================================================================
 * freeList
 * =============================================================================
 */
static void
freeList (list_node_t* nodePtr, void (*freeData)(void *))
{
    list_node_t *nextPtr;

    while (nodePtr) {
        nextPtr = nodePtr->nextPtr;
        if (freeData)
            freeData(nodePtr->dataPtr);
        freeNode(nodePtr);
        nodePtr = nextPtr;
    }
}


/* =============================================================================
 * PfreeList
 * =============================================================================
 */
static void
PfreeList (list_node_t* nodePtr, void (*PfreeData)(void *))
{
    while (nodePtr) {
        list_node_t *nextPtr = nextPtr = nodePtr->nextPtr;
        if (PfreeData)
            PfreeData(nodePtr->dataPtr);
        PfreeNode(nodePtr);
        nodePtr = nextPtr;
    }
}


/* =============================================================================
 * HTMfreeList
 * =============================================================================
 */
static void
HTMfreeList (list_node_t* nodePtr, void (*HTMfreeData)(void *))
{
    while (nodePtr) {
        list_node_t *nextPtr = HTM_SHARED_READ_P(nodePtr->nextPtr);
        if (HTMfreeData)
            HTMfreeData(HTM_SHARED_READ_P(nodePtr->dataPtr));
        HTMfreeNode(nodePtr);
        nodePtr = nextPtr;
    }
}


/* =============================================================================
 * TMfreeList
 * =============================================================================
 */
TM_CALLABLE
static void
TMfreeList (TM_ARGDECL  list_node_t* nodePtr, TM_CALLABLE void (*TMfreeData)(void *))
{
    while (nodePtr) {
        list_node_t *nextPtr = TM_SHARED_READ_TAG_P(nodePtr->nextPtr, nodePtr);
        if (TMfreeData)
            TMfreeData(TM_SHARED_READ_TAG_P(nodePtr->dataPtr, nodePtr));
        TMfreeNode(nodePtr);
        nodePtr = nextPtr;
    }
}


/* =============================================================================
 * list_free
 * =============================================================================
 */
void
list_free (list_t* listPtr, void (*freeData)(void *))
{
    freeList(listPtr->head.nextPtr, freeData);
    free(listPtr);
}


/* =============================================================================
 * Plist_free
 * =============================================================================
 */
void
Plist_free (list_t* listPtr, void (*PfreeData)(void *))
{
    PfreeList(listPtr->head.nextPtr, PfreeData);
    P_FREE(listPtr);
}


/* =============================================================================
 * HTMlist_free
 * =============================================================================
 */
void
HTMlist_free (list_t* listPtr, void (*HTMfreeData)(void *))
{
    list_node_t* nextPtr = (list_node_t*)HTM_SHARED_READ_P(listPtr->head.nextPtr);
    HTMfreeList(nextPtr, HTMfreeData);
    HTM_FREE(listPtr);
}


/* =============================================================================
 * TMlist_free
 * =============================================================================
 */
TM_CALLABLE
void
TMlist_free (TM_ARGDECL  list_t* listPtr, TM_CALLABLE void (*TMfreeData)(void *))
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_FREE, NULL, listPtr);
#endif /* ORIGINAL */
    list_node_t* nextPtr = (list_node_t*)TM_SHARED_READ_TAG_P(listPtr->head.nextPtr, (uintptr_t)listPtr);
    TMfreeList(TM_ARG  nextPtr, TMfreeData);
#if !defined(ORIGINAL) && defined(MERGE_LIST)
    TM_SHARED_WRITE_P(listPtr->head.nextPtr, TMLIST_BAD_ADDR);
#endif /* !ORIGINAL && MERGE_LIST */
    TM_FREE(listPtr);
#ifndef ORIGINAL
    TM_LOG_END(LIST_FREE, NULL);
#endif /* ORIGINAL */
}


/* =============================================================================
 * list_isEmpty
 * -- Return TRUE if list is empty, else FALSE
 * =============================================================================
 */
bool_t
list_isEmpty (list_t* listPtr)
{
    return (listPtr->head.nextPtr == NULL);
}


/* =============================================================================
 * TMlist_isEmpty
 * -- Return TRUE if list is empty, else FALSE
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMlist_isEmpty (TM_ARGDECL  list_t* listPtr)
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_ISEMPTY, NULL, listPtr);
#endif /* ORIGINAL */
    bool_t rv = ((void*)TM_SHARED_READ_TAG_P(listPtr->head.nextPtr, (uintptr_t)listPtr) == NULL) ?
            TRUE : FALSE;
#ifndef ORIGINAL
    TM_LOG_END(LIST_ISEMPTY, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * list_getSize
 * -- Returns the size of the list
 * =============================================================================
 */
long
list_getSize (list_t* listPtr)
{
    return listPtr->size;
}


/* =============================================================================
 * HTMlist_getSize
 * -- Returns the size of the list
 * =============================================================================
 */
long
HTMlist_getSize (list_t* listPtr)
{
    return HTM_SHARED_READ(listPtr->size);
}


/* =============================================================================
 * TMlist_getSize
 * -- Returns the size of the list
 * =============================================================================
 */
TM_CALLABLE
long
TMlist_getSize (TM_ARGDECL  list_t* listPtr)
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_GETSZ, NULL, listPtr);
#endif /* ORIGINAL */
    long rv = TM_SHARED_READ_TAG(listPtr->size, (uintptr_t)listPtr);
#ifndef ORIGINAL
    TM_LOG_END(LIST_GETSZ, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * findPrevious
 * =============================================================================
 */
static list_node_t*
findPrevious (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr = &(listPtr->head);
    list_node_t* nodePtr = prevPtr->nextPtr;

    for (; nodePtr != NULL; nodePtr = nodePtr->nextPtr) {
        if (listPtr->compare(nodePtr->dataPtr, dataPtr) >= 0) {
            return prevPtr;
        }
        prevPtr = nodePtr;
    }

    return prevPtr;
}


/* =============================================================================
 * HTMfindPrevious
 * =============================================================================
 */
static list_node_t*
HTMfindPrevious (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr = &(listPtr->head);
    list_node_t* nodePtr;

    for (nodePtr = (list_node_t*)HTM_SHARED_READ_P(prevPtr->nextPtr);
         nodePtr != NULL;
         nodePtr = (list_node_t*)HTM_SHARED_READ_P(nodePtr->nextPtr))
    {
        if (listPtr->compare(HTM_SHARED_READ_P(nodePtr->dataPtr), dataPtr) >= 0)
            goto out;
        prevPtr = nodePtr;
    }

out:
    return prevPtr;
}


/* =============================================================================
 * TMfindPrevious
 * =============================================================================
 */
TM_CALLABLE
static list_node_t*
TMfindPrevious (TM_ARGDECL  list_t* listPtr, void* dataPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_LIST)
    __label__ replay;
#endif /* !ORIGINAL && MERGE_LIST */

    list_node_t* prevPtr = &(listPtr->head);
    list_node_t* nodePtr;

#ifndef ORIGINAL
# ifdef MERGE_LIST
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Conflict occurred directly inside LIST_PREVIOUS */
        ASSERT(params->leaf == 1);
        ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->current), LIST_PREVIOUS));

# ifdef TM_DEBUG
        printf("LIST_PREVIOUS_JIT listPtr:%p dataPtr:%p rv:%p\n", listPtr, dataPtr, params->rv.ptr);
# endif
        prevPtr = params->rv.ptr;

        TM_FINISH_MERGE();
        goto replay;
    }
# endif /* MERGE_LIST */

    TM_LOG_BEGIN(LIST_PREVIOUS,
# ifdef MERGE_LIST
        merge
# else
        NULL
# endif /* MERGE_LIST */
        , listPtr, dataPtr);
#endif /* ORIGINAL */

    for (nodePtr = (list_node_t*)TM_SHARED_READ_TAG_P(prevPtr->nextPtr, (uintptr_t)prevPtr);
         nodePtr != NULL;
         nodePtr = (list_node_t*)TM_SHARED_READ_TAG_P(nodePtr->nextPtr, (uintptr_t)nodePtr))
    {
        if (listPtr->compare(TM_SHARED_READ_TAG_P(nodePtr->dataPtr, (uintptr_t)nodePtr), dataPtr) >= 0)
            goto out;
        prevPtr = nodePtr;
    }

out:
#ifndef ORIGINAL
    TM_LOG_END(LIST_PREVIOUS, &prevPtr);
replay:
#endif /* ORIGINAL */
    return prevPtr;
}


/* =============================================================================
 * list_find
 * -- Returns NULL if not found, else returns pointer to data
 * =============================================================================
 */
void*
list_find (list_t* listPtr, void* dataPtr)
{
    list_node_t* nodePtr;
    list_node_t* prevPtr = findPrevious(listPtr, dataPtr);

    nodePtr = prevPtr->nextPtr;

    if ((nodePtr == NULL) ||
        (listPtr->compare(nodePtr->dataPtr, dataPtr) != 0)) {
        return NULL;
    }

    return (nodePtr->dataPtr);
}


/* =============================================================================
 * HTMlist_find
 * -- Returns NULL if not found, else returns pointer to data
 * =============================================================================
 */
void*
HTMlist_find (list_t* listPtr, void* dataPtr)
{
    void *rv;
    list_node_t* prevPtr = TMfindPrevious(TM_ARG  listPtr, dataPtr);
    list_node_t* nodePtr = (list_node_t*)TM_SHARED_READ_TAG_P(prevPtr->nextPtr, (uintptr_t)prevPtr);

    if ((nodePtr == NULL) ||
        (listPtr->compare((rv = TM_SHARED_READ_TAG_P(nodePtr->dataPtr, (uintptr_t)nodePtr)), dataPtr) != 0)) {
        rv = NULL;
        goto out;
    }

out:
    return rv;
}


/* =============================================================================
 * TMlist_find
 * -- Returns NULL if not found, else returns pointer to data
 * =============================================================================
 */
TM_CALLABLE
void*
TMlist_find (TM_ARGDECL  list_t* listPtr, void* dataPtr)
{
#ifdef MERGE_LIST
    __label__ find;
#endif /* MERGE_LIST */

    list_node_t* nodePtr = NULL;
    list_node_t* prevPtr = NULL;
    void *rv;

#if !defined(ORIGINAL) && defined(MERGE_LIST)
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Just-in-time merge */
        ASSERT(STM_SAME_OP(stm_get_current_op(), params->current) || STM_SAME_OPID(stm_get_op_opcode(stm_get_current_op()), LIST_PREVIOUS));

        /* Conflict occurred directly inside LIST_FIND, or inside LIST_PREVIOUS */
        if (STM_SAME_OPID(stm_get_op_opcode(params->current), LIST_FIND) || (STM_SAME_OPID(stm_get_op_opcode(params->previous), LIST_PREVIOUS) && prevPtr)) {
            if (params->leaf) {
                ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->current), LIST_FIND));
# ifdef TM_DEBUG
                printf("\nLIST_FIND_JIT listPtr:%p dataPtr:%p rv:%p\n", listPtr, dataPtr, params->rv.ptr);
# endif
            } else {
# ifdef TM_DEBUG
                printf("\nLIST_FIND_JIT listPtr:%p dataPtr:%p <- LIST_PREVIOUS %p\n", listPtr, dataPtr, params->conflict.result.ptr);
# endif
            }

            stm_read_t r = TM_SHARED_DID_READ(prevPtr->nextPtr);
            if (STM_VALID_READ(r)) {
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                r = TM_SHARED_DID_READ(nodePtr->dataPtr);
                if (STM_VALID_READ(r))
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
            }

            if (!params->leaf) {
                prevPtr = params->conflict.result.ptr;
                ASSERT(prevPtr);
            }

            TM_FINISH_MERGE();
            goto find;
        }

# ifdef TM_DEBUG
        printf("LIST_FIND_JIT listPtr:%p dataPtr:%p UNSUPPORTED:%p\n", listPtr, dataPtr, params->addr);
# endif
        return STM_MERGE_UNSUPPORTED;
    }
#endif /* !ORIGINAL && MERGE_LIST */

#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_FIND,
# ifdef MERGE_LIST
        merge
# else
        NULL
# endif /* MERGE_LIST */
        , listPtr, dataPtr);
#endif /* ORIGINAL */
    prevPtr = TMfindPrevious(TM_ARG  listPtr, dataPtr);
#ifdef MERGE_LIST
find:
#endif /* MERGE_LIST */
    nodePtr = (list_node_t*)TM_SHARED_READ_TAG_P(prevPtr->nextPtr, (uintptr_t)prevPtr);

    if ((nodePtr == NULL) ||
        (listPtr->compare((rv = TM_SHARED_READ_TAG_P(nodePtr->dataPtr, (uintptr_t)nodePtr)), dataPtr) != 0)) {
        rv = NULL;
        goto out;
    }

out:
#ifndef ORIGINAL
    TM_LOG_END(LIST_FIND, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * list_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
list_insert (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;
    list_node_t* currPtr;

    prevPtr = findPrevious(listPtr, dataPtr);
    currPtr = prevPtr->nextPtr;

#ifdef LIST_NO_DUPLICATES
    if ((currPtr != NULL) &&
        listPtr->compare(currPtr->dataPtr, dataPtr) == 0) {
        return FALSE;
    }
#endif

    nodePtr = allocNode(dataPtr);
    if (nodePtr == NULL) {
        return FALSE;
    }

    nodePtr->nextPtr = currPtr;
    prevPtr->nextPtr = nodePtr;
    listPtr->size++;

    return TRUE;
}


/* =============================================================================
 * Plist_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
Plist_insert (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;
    list_node_t* currPtr;

    prevPtr = findPrevious(listPtr, dataPtr);
    currPtr = prevPtr->nextPtr;

#ifdef LIST_NO_DUPLICATES
    if ((currPtr != NULL) &&
        listPtr->compare(currPtr->dataPtr, dataPtr) == 0) {
        return FALSE;
    }
#endif

    nodePtr = PallocNode(dataPtr);
    if (nodePtr == NULL) {
        return FALSE;
    }

    nodePtr->nextPtr = currPtr;
    prevPtr->nextPtr = nodePtr;
    listPtr->size++;

    return TRUE;
}


/* =============================================================================
 * HTMlist_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
HTMlist_insert (list_t* listPtr, void* dataPtr)
{
    bool_t rv;
    list_node_t* prevPtr = HTMfindPrevious(TM_ARG  listPtr, dataPtr);
    list_node_t* currPtr = (list_node_t*)HTM_SHARED_READ_P(prevPtr->nextPtr);

#ifdef LIST_NO_DUPLICATES
    if ((currPtr != NULL) &&
        listPtr->compare(HTM_SHARED_READ_P(currPtr->dataPtr), dataPtr) == 0) {
        rv = FALSE;
        goto out;
    }
#endif

    list_node_t* nodePtr = HTMallocNode(TM_ARG  dataPtr);
    if (nodePtr == NULL) {
        rv = FALSE;
        goto out;
    }

#ifdef MERGE_LIST
    /* Must perform a transactional write, otherwise a concurrent reader may miss a conflict */
    HTM_SHARED_WRITE_P(nodePtr->nextPtr, currPtr);
#else
    nodePtr->nextPtr = currPtr;
#endif /* MERGE_LIST */
    HTM_SHARED_WRITE_P(prevPtr->nextPtr, nodePtr);
    /* Tag the list with a pointer to the newly allocated node */
    HTM_SHARED_WRITE(listPtr->size, HTM_SHARED_READ(listPtr->size) + 1);

    rv = TRUE;
out:
    return rv;
}


/* =============================================================================
 * TMlist_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMlist_insert (TM_ARGDECL  list_t* listPtr, void* dataPtr)
{
#ifdef MERGE_LIST
    __label__ insert;
#endif /* MERGE_LIST */

    list_node_t* prevPtr = NULL;
    list_node_t* nodePtr = NULL;
    list_node_t* currPtr = NULL;
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_LIST)
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Just-in-time merge */
        ASSERT(STM_SAME_OP(stm_get_current_op(), params->current) || STM_SAME_OPID(stm_get_op_opcode(stm_get_current_op()), LIST_PREVIOUS));

        /* Conflict occurred directly inside LIST_INSERT or inside LIST_PREVIOUS */
        if (STM_SAME_OPID(stm_get_op_opcode(params->current), LIST_INSERT) || (STM_SAME_OPID(stm_get_op_opcode(params->previous), LIST_PREVIOUS) && prevPtr)) {
            if (params->leaf == 1) {
# ifdef TM_DEBUG
                printf("\nLIST_INSERT_JIT listPtr:%p dataPtr:%p rv:%ld\n", listPtr, dataPtr, params->rv.sint);
# endif
            } else {
# ifdef TM_DEBUG
                printf("\nLIST_INSERT_JIT listPtr:%p dataPtr:%p <- LIST_PREVIOUS %p\n", listPtr, dataPtr, params->conflict.result.ptr);
# endif
            }

            stm_read_t r = TM_SHARED_DID_READ(prevPtr->nextPtr);
            if (STM_VALID_READ(r)) {
                stm_read_t rnext = stm_get_load_next(r, 1, 0);
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                r = rnext;
#ifdef LIST_NO_DUPLICATES
                if (currPtr && STM_VALID_READ(r)) {
                    ASSERT_FAIL(stm_get_load_addr(r) == (const volatile stm_word_t *)&currPtr->dataPtr);
                    rnext = stm_get_load_next(r, 1, 0);
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    r = rnext;
                }
#endif
            }

            if (nodePtr) {
                stm_malloc_t m = TM_DID_MALLOC(nodePtr);
                if (STM_VALID_MALLOC(m)) {
                    ASSERT_FAIL(TM_UNDO_MALLOC(m));

                    stm_write_t w = TM_SHARED_DID_WRITE(nodePtr->nextPtr);
                    nodePtr = NULL;
                    if (STM_VALID_WRITE(w)) {
                        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));

                        w = TM_SHARED_DID_WRITE(prevPtr->nextPtr);
                        if (STM_VALID_WRITE(w)) {
                            ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));

                            r = TM_SHARED_DID_READ(listPtr->size);
                            if (STM_VALID_READ(r)) {
                                ASSERT_FAIL(stm_get_load_addr(r) == (const volatile stm_word_t *)&listPtr->size);
                                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));

                                w = TM_SHARED_DID_WRITE(listPtr->size);
                                if (STM_VALID_WRITE(w)) {
                                    ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
                                }
                            }
                        }
                    }
                }
            }

            if (!params->leaf) {
                prevPtr = params->conflict.result.ptr;
                ASSERT(prevPtr);
            }

            TM_FINISH_MERGE();
            goto insert;
        }

#  ifdef TM_DEBUG
        printf("LIST_INSERT_JIT listPtr:%p dataPtr:%p UNSUPPORTED:%p\n", listPtr, dataPtr, params->addr);
#  endif
        return STM_MERGE_UNSUPPORTED;
    }
#endif /* !ORIGINAL && MERGE_LIST */
#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_INSERT,
# ifdef MERGE_LIST
        merge
# else
        NULL
# endif /* MERGE_LIST */
        , listPtr, dataPtr);
#endif /* ORIGINAL */
    prevPtr = TMfindPrevious(TM_ARG  listPtr, dataPtr);
#ifdef MERGE_LIST
insert:
#endif /* MERGE_LIST */
    currPtr = (list_node_t*)TM_SHARED_READ_TAG_P(prevPtr->nextPtr, (uintptr_t)prevPtr);

#ifdef LIST_NO_DUPLICATES
    if ((currPtr != NULL) &&
        listPtr->compare(TM_SHARED_READ_TAG_P(currPtr->dataPtr, (uintptr_t)currPtr), dataPtr) == 0) {
        rv = FALSE;
        goto out;
    }
#endif

    nodePtr = TMallocNode(TM_ARG  dataPtr);
    if (nodePtr == NULL) {
        rv = FALSE;
        goto out;
    }

#ifdef MERGE_LIST
    /* Must perform a transactional write, otherwise a concurrent reader may miss a conflict */
    TM_SHARED_WRITE_P(nodePtr->nextPtr, currPtr);
#else
    nodePtr->nextPtr = currPtr;
#endif /* MERGE_LIST */
    TM_SHARED_WRITE_P(prevPtr->nextPtr, nodePtr);
    /* Tag the list with a pointer to the newly allocated node */
    TM_SHARED_WRITE(listPtr->size, TM_SHARED_READ_TAG(listPtr->size, (uintptr_t)nodePtr) + 1);

    rv = TRUE;
out:
#ifndef ORIGINAL
    TM_LOG_END(LIST_INSERT, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * list_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
bool_t
list_remove (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;

    prevPtr = findPrevious(listPtr, dataPtr);

    nodePtr = prevPtr->nextPtr;
    if ((nodePtr != NULL) &&
        (listPtr->compare(nodePtr->dataPtr, dataPtr) == 0))
    {
        prevPtr->nextPtr = nodePtr->nextPtr;
        nodePtr->nextPtr = NULL;
        freeNode(nodePtr);
        listPtr->size--;
        assert(listPtr->size >= 0);
        return TRUE;
    }

    return FALSE;
}


/* =============================================================================
 * Plist_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
bool_t
Plist_remove (list_t* listPtr, void* dataPtr)
{
    list_node_t* prevPtr;
    list_node_t* nodePtr;

    prevPtr = findPrevious(listPtr, dataPtr);

    nodePtr = prevPtr->nextPtr;
    if ((nodePtr != NULL) &&
        (listPtr->compare(nodePtr->dataPtr, dataPtr) == 0))
    {
        prevPtr->nextPtr = nodePtr->nextPtr;
        nodePtr->nextPtr = NULL;
        PfreeNode(nodePtr);
        listPtr->size--;
        assert(listPtr->size >= 0);
        return TRUE;
    }

    return FALSE;
}


/* =============================================================================
 * HTMlist_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
bool_t
HTMlist_remove (list_t* listPtr, void* dataPtr)
{
    bool_t rv;

    list_node_t* prevPtr = HTMfindPrevious(listPtr, dataPtr);
    list_node_t* nodePtr = (list_node_t*)HTM_SHARED_READ_P(prevPtr->nextPtr);

    if ((nodePtr != NULL) &&
        (listPtr->compare(HTM_SHARED_READ_P(nodePtr->dataPtr), dataPtr) == 0))
    {
        HTM_SHARED_WRITE_P(prevPtr->nextPtr, HTM_SHARED_READ_P(nodePtr->nextPtr));
        HTM_SHARED_WRITE_P(nodePtr->nextPtr, (struct list_node*)NULL);
        HTMfreeNode(nodePtr);
        HTM_SHARED_WRITE(listPtr->size, HTM_SHARED_READ(listPtr->size) - 1);
        assert(listPtr->size >= 0);
        rv = TRUE;
        goto out;
    }

    rv = FALSE;
out:
    return rv;
}


/* =============================================================================
 * TMlist_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMlist_remove (TM_ARGDECL  list_t* listPtr, void* dataPtr)
{
#ifdef MERGE_LIST
    __label__ remove;
#endif /* MERGE_LIST */

    list_node_t* prevPtr = NULL;
    list_node_t* nodePtr = NULL;
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_LIST)
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Just-in-time merge */
        ASSERT(STM_SAME_OP(stm_get_current_op(), params->current) || STM_SAME_OPID(stm_get_op_opcode(stm_get_current_op()), LIST_PREVIOUS));

        /* Conflict occurred directly inside LIST_REMOVE or inside LIST_PREVIOUS */
        if (STM_SAME_OPID(stm_get_op_opcode(params->current), LIST_REMOVE) || (STM_SAME_OPID(stm_get_op_opcode(params->previous), LIST_PREVIOUS) && prevPtr)) {
            if (params->leaf) {
# ifdef TM_DEBUG
                printf("\nLIST_REMOVE_JIT listPtr:%p dataPtr:%p rv:%ld\n", listPtr, dataPtr, params->rv.sint);
# endif
            } else {
# ifdef TM_DEBUG
                printf("\nLIST_REMOVE_JIT listPtr:%p dataPtr:%p <- LIST_PREVIOUS %p\n", listPtr, dataPtr, params->conflict.result.ptr);
# endif
            }

            stm_read_t r = TM_SHARED_DID_READ(prevPtr->nextPtr);
            if (STM_VALID_READ(r)) {
                stm_read_t rnext = stm_get_load_next(r, 1, 0);
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                r = rnext;

                if (STM_VALID_READ(r)) {
                    ASSERT_FAIL(stm_get_load_addr(r) == (const volatile stm_word_t *)&nodePtr->dataPtr);
                    rnext = stm_get_load_next(r, 1, 0);
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    r = rnext;

                    if (STM_VALID_READ(r)) {
                        ASSERT_FAIL(stm_get_load_addr(r) == (const volatile stm_word_t *)&nodePtr->nextPtr);
                        rnext = stm_get_load_next(r, 1, 0);
                        ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                        r = rnext;

                        stm_write_t w = TM_SHARED_DID_WRITE(prevPtr->nextPtr);
                        if (STM_VALID_WRITE(w)) {
                            ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));

                            w = TM_SHARED_DID_WRITE(nodePtr->nextPtr);
                            if (STM_VALID_WRITE(w)) {
                                ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));

                                ASSERT_FAIL(TMfreeNodeUndo(nodePtr) != STM_MERGE_ABORT);

                                if (STM_VALID_READ(r)) {
                                    ASSERT_FAIL(stm_get_load_addr(r) == (const volatile stm_word_t *)&listPtr->size);
                                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));

                                    w = TM_SHARED_DID_WRITE(listPtr->size);
                                    if (STM_VALID_WRITE(w))
                                        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
                                }
                            }
                        }
                    }
                }
            }

            if (!params->leaf) {
                prevPtr = params->conflict.result.ptr;
                ASSERT(prevPtr);
            }

            TM_FINISH_MERGE();
            goto remove;
        }

#  ifdef TM_DEBUG
        printf("LIST_REMOVE_JIT listPtr:%p dataPtr:%p UNSUPPORTED:%p\n", listPtr, dataPtr, params->addr);
#  endif
        return STM_MERGE_UNSUPPORTED;
    }
#endif /* !ORIGINAL && MERGE_LIST */

#ifndef ORIGINAL
    TM_LOG_BEGIN(LIST_REMOVE,
# ifdef MERGE_LIST
        merge
# else
        NULL
# endif /* MERGE_LIST */
        , listPtr, dataPtr);
#endif /* ORIGINAL */
    prevPtr = TMfindPrevious(TM_ARG  listPtr, dataPtr);
#ifdef MERGE_LIST
remove:
#endif /* MERGE_LIST */
    nodePtr = (list_node_t*)TM_SHARED_READ_TAG_P(prevPtr->nextPtr, (uintptr_t)prevPtr);
    if ((nodePtr != NULL) &&
        (listPtr->compare(TM_SHARED_READ_TAG_P(nodePtr->dataPtr, (uintptr_t)nodePtr), dataPtr) == 0))
    {
        TM_SHARED_WRITE_P(prevPtr->nextPtr, TM_SHARED_READ_TAG_P(nodePtr->nextPtr, (uintptr_t)nodePtr));
        TM_SHARED_WRITE_P(nodePtr->nextPtr, (struct list_node*)NULL);
        TMfreeNode(TM_ARG  nodePtr);
        TM_SHARED_WRITE(listPtr->size, (TM_SHARED_READ_TAG(listPtr->size, (uintptr_t)nodePtr) - 1));
        assert(listPtr->size >= 0);
        rv = TRUE;
        goto out;
    }

    rv = FALSE;
out:
#ifndef ORIGINAL
    TM_LOG_END(LIST_REMOVE, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * list_clear
 * -- Removes all elements
 * =============================================================================
 */
void
list_clear (list_t* listPtr, void (*freeData)(void *))
{
    freeList(listPtr->head.nextPtr, freeData);
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;
}


/* =============================================================================
 * Plist_clear
 * -- Removes all elements
 * =============================================================================
 */
void
Plist_clear (list_t* listPtr, void (*PfreeData)(void *))
{
    PfreeList(listPtr->head.nextPtr, PfreeData);
    listPtr->head.nextPtr = NULL;
    listPtr->size = 0;
}


/* =============================================================================
 * list_setCompare
 * =============================================================================
 */
void
list_setCompare (list_t* listPtr, long (*compare)(const void*, const void*))
{
    listPtr->compare = compare;
}


/* =============================================================================
 * TMlist_setCompare
 * =============================================================================
 */
TM_CALLABLE
void
TMlist_setCompare (TM_ARGDECL  list_t* listPtr, long (*compare)(const void*, const void*))
{
    TM_SHARED_WRITE_P(listPtr->compare, compare);
}


/* =============================================================================
 * TEST_LIST
 * =============================================================================
 */
#ifdef TEST_LIST


#include <assert.h>
#include <stdio.h>


static long
compare (const void* a, const void* b)
{
    return (*((const long*)a) - *((const long*)b));
}


static void
printList (list_t* listPtr)
{
    list_iter_t it;
    printf("[");
    list_iter_reset(&it, listPtr);
    while (list_iter_hasNext(&it, listPtr)) {
        printf("%li ", *((long*)(list_iter_next(&it, listPtr))));
    }
    puts("]");
}


static void
insertInt (list_t* listPtr, long* data)
{
    printf("Inserting: %li\n", *data);
    list_insert(listPtr, (void*)data);
    printList(listPtr);
}


static void
removeInt (list_t* listPtr, long* data)
{
    printf("Removing: %li\n", *data);
    list_remove(listPtr, (void*)data);
    printList(listPtr);
}


int
main ()
{
    list_t* listPtr;
#ifdef LIST_NO_DUPLICATES
    long data1[] = {3, 1, 4, 1, 5, -1};
#else
    long data1[] = {3, 1, 4, 5, -1};
#endif
    long data2[] = {3, 1, 4, 1, 5, -1};
    long i;

    puts("Starting...");

    puts("List sorted by values:");

    listPtr = list_alloc(&compare);

    for (i = 0; data1[i] >= 0; i++) {
        insertInt(listPtr, &data1[i]);
        assert(*((long*)list_find(listPtr, &data1[i])) == data1[i]);
    }

    for (i = 0; data1[i] >= 0; i++) {
        removeInt(listPtr, &data1[i]);
        assert(list_find(listPtr, &data1[i]) == NULL);
    }

    list_free(listPtr);

    puts("List sorted by addresses:");

    listPtr = list_alloc(NULL);

    for (i = 0; data2[i] >= 0; i++) {
        insertInt(listPtr, &data2[i]);
        assert(*((long*)list_find(listPtr, &data2[i])) == data2[i]);
    }

    for (i = 0; data2[i] >= 0; i++) {
        removeInt(listPtr, &data2[i]);
        assert(list_find(listPtr, &data2[i]) == NULL);
    }

    list_free(listPtr);

    puts("Done.");

    return 0;
}


#endif /* TEST_LIST */


/* =============================================================================
 *
 * End of list.c
 *
 * =============================================================================
 */

#ifndef ORIGINAL
# ifdef MERGE_LIST
stm_merge_t TMlist_merge(stm_merge_context_t *params) {
    const stm_op_id_t op = stm_get_op_opcode(params->current);

    /* Delayed merge only */
    ASSERT(!STM_SAME_OP(stm_get_current_op(), params->current));

    const stm_union_t *args;
    const ssize_t nargs = stm_get_op_args(params->current, &args);

    if (STM_SAME_OPID(op, LIST_GETSZ) || STM_SAME_OPID(op, LIST_ISEMPTY)) {
        ASSERT(nargs == 1);
        const list_t *listPtr = args[0].ptr;
        ASSERT(listPtr);
        ASSERT(!STM_VALID_OP(params->previous));
        ASSERT(params->leaf == 1);

        ASSERT(ENTRY_VALID(params->conflict.entries->e1));
        const stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
        ASSERT(STM_VALID_READ(r));
        if (STM_SAME_OPID(op, LIST_GETSZ)) {
            ASSERT(params->rv.sint >= 0);

            ASSERT(params->addr == &listPtr->size);
            /* Read the old and new values */
# ifdef TM_DEBUG
            long old;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, listPtr->size, old));
# endif
            long new;
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, listPtr->size, new));
# ifdef TM_DEBUG
            printf("LIST_GETSZ addr:%p listPtr:%p listPtr->size:%p (old):%li (new):%li\n", params->addr, listPtr, &listPtr->size, old, new);
            printf("LIST_GETSZ addr:%p listPtr:%p rv(old):%li rv(new):%li\n", params->addr, listPtr, params->rv.sint, new);
# endif
            params->rv.sint = new;
            return STM_MERGE_OK;
        } else {
            ASSERT(!STM_VALID_WRITE(TM_SHARED_DID_WRITE(listPtr->head.nextPtr)));
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);

            ASSERT(params->addr == &listPtr->head.nextPtr);
            /* Read the old and new values */
# ifdef TM_DEBUG
            list_iter_t old = TM_SHARED_READ_TAG_P(listPtr->head.nextPtr, (uintptr_t)listPtr);
# endif
            list_iter_t new;
            ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, listPtr->head.nextPtr, new));
# ifdef TM_DEBUG
            printf("\nLIST_ISEMPTY addr:%p listPtr:%p listPtr->head.nextPtr:%p (old):%p (new):%p\n", params->addr, listPtr, &listPtr->head.nextPtr, old, new);
            printf("LIST_ISEMPTY addr:%p listPtr:%p rv(old):%li rv(new):%i\n", params->addr, listPtr, params->rv.sint, new ? FALSE : TRUE);
# endif
            params->rv.sint = new ? FALSE : TRUE;
            return STM_MERGE_OK;
        }
    } else if (STM_SAME_OPID(op, LIST_IT_HASNEXT) || STM_SAME_OPID(op, LIST_IT_NEXT)) {
        ASSERT(nargs == 2);
        const list_iter_t it = args[0].ptr;
        ASSERT(it);
        const list_t *listPtr = args[1].ptr;
        ASSERT(listPtr);
        ASSERT(!STM_VALID_OP(params->previous));
        ASSERT(params->leaf == 1);

        ASSERT(ENTRY_VALID(params->conflict.entries->e1));
        stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
        ASSERT(STM_VALID_READ(r));
        if (STM_SAME_OPID(op, LIST_IT_HASNEXT)) {
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);

            ASSERT(params->addr == &it->nextPtr);
            /* Read the old and new values */
# ifdef TM_DEBUG
            list_iter_t old;
            ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, it->nextPtr, old));
# endif
            list_iter_t new;
            ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, it->nextPtr, new));
# ifdef TM_DEBUG
            printf("\nLIST_IT_HASNEXT addr:%p listPtr:%p, it:%p .nextPtr(old):%p .nextPtr(new):%p rv(old):%li rv(new):%i\n", params->addr, listPtr, it, old, new, params->rv.sint, new ? TRUE : FALSE);
# endif
            /* Check if the current node has been freed */
            if (new == TMLIST_BAD_ADDR) {
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                params->rv.sint = FALSE;
            } else
                params->rv.sint = new ? TRUE : FALSE;
            /* Set the conflict address to the iterator */
            params->addr = it;
            return STM_MERGE_OK;
        } else {
            list_iter_t oldNext, newNext;
            /* Recover conflicting pointer from address */
            const list_node_t *base = (list_node_t *)TM_SHARED_GET_TAG(r);
            ASSERT(base);
            /* Read the old and new next pointers */
            if (params->addr == &base->nextPtr) {
                /* Conflict is at the iterator (previous node) */
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, base->nextPtr, oldNext));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, base->nextPtr, newNext));
                if (newNext == TMLIST_BAD_ADDR) {
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    return STM_MERGE_OK;
                }

                /* The next node must exist, because it would have otherwise been handled by the merge function for LIST_IT_HASNEXT. Assume the list has been modified since LIST_IT_HASNEXT was merged */
                if (__builtin_expect(!newNext, 0))
                    return STM_MERGE_RETRY;
                ASSERT(oldNext);
                r = TM_SHARED_DID_READ(oldNext->dataPtr);

# ifdef TM_DEBUG
                printf("LIST_IT_NEXT addr:%p it:%p listPtr:%p .next(old):%p .next(new):%p\n", params->addr, base, listPtr, oldNext, newNext);
# endif
                if (oldNext == newNext)
                    return STM_MERGE_OK;
            } else {
                ASSERT(params->addr == &base->dataPtr);
                /* Conflict is at the data pointer */
                oldNext = newNext = (list_iter_t)base;
            }

            const void *oldData, *newData;
            /* Read the old and new data pointer */
            if (oldNext == newNext) {
                ASSERT(STM_VALID_READ(r));
                ASSERT(newNext);
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, oldNext->dataPtr, oldData));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, newNext->dataPtr, newData));
            } else {
                if (STM_VALID_READ(r)) {
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, oldNext->dataPtr, oldData));
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                }
                ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(newNext->dataPtr)));
                newData = TM_SHARED_READ_TAG_P(newNext->dataPtr, (uintptr_t)newNext);
                r = TM_SHARED_DID_READ(newNext->dataPtr);
                ASSERT(STM_VALID_READ(r));
            }
# ifdef TM_DEBUG
            printf("LIST_IT_NEXT addr:%p it:%p listPtr:%p .dataPtr(old):%p .dataPtr(new):%p\n", params->addr, base, listPtr, oldData, newData);
# endif
            if (newData == TMLIST_BAD_ADDR) {
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                return params->addr == &base->nextPtr ? STM_MERGE_RETRY : STM_MERGE_OK;
            }

# ifdef TM_DEBUG
            printf("LIST_IT_NEXT addr:%p it:%p listPtr:%p rv(old):%p rv(new):%p\n", params->addr, base, listPtr, params->rv.ptr, newData);
# endif
            if (oldNext == newNext && oldData == newData)
                return STM_MERGE_OK;
            /* Set the conflict address to the iterator */
            params->addr = (void *)base;
            /* Pass the new return value */
            params->rv.ptr = (void *)newData;
            /* Parent must handle updating the iterator and potentially performing another loop iteration, because this variable may have gone out-of-scope */
            return STM_MERGE_OK_PARENT;
        }
    } else if (STM_SAME_OPID(op, LIST_PREVIOUS) || STM_SAME_OPID(op, LIST_FIND) || STM_SAME_OPID(op, LIST_INSERT) || STM_SAME_OPID(op, LIST_REMOVE)) {
        ASSERT(nargs == 2);
        const list_t *listPtr = args[0].ptr;
        ASSERT(listPtr);
        const void *dataPtr = args[1].ptr;

        if (STM_SAME_OPID(op, LIST_PREVIOUS)) {
            ASSERT(params->leaf == 1);
            ASSERT(!STM_VALID_OP(params->previous));
            ASSERT(ENTRY_VALID(params->conflict.entries->e1));
            stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
            ASSERT(STM_VALID_READ(r));
            ASSERT(params->rv.ptr);

            const list_node_t *oldNext, *prevPtr = NULL, *nodePtr = NULL;
            const void *oldData, *newData;
            /* Recover conflicting pointer from address */
            const list_node_t *base = (list_node_t *)TM_SHARED_GET_TAG(r);
            ASSERT(base);
            if (params->addr == &base->dataPtr) {
                /* Conflict is at the data pointer */
                nodePtr = base;
                ASSERT(!STM_VALID_WRITE(TM_SHARED_DID_WRITE(nodePtr->dataPtr)));

                /* Read the old and new data pointer */
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, nodePtr->dataPtr, oldData));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, nodePtr->dataPtr, newData));
                if (oldData == newData)
                    return STM_MERGE_OK;
data:
# ifdef TM_DEBUG
                printf("LIST_PREVIOUS addr:%p listPtr:%p dataPtr:%p nodePtr:%p nodePtr.data(old):%p nodePtr.data(new):%p\n", params->addr, listPtr, dataPtr, nodePtr, oldData, newData);
# endif

                /* Check if the current node has been freed */
                if (newData == TMLIST_BAD_ADDR) {
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    prevPtr = params->rv.ptr;
                    goto out;
                }

                /* Check if this is the new predecessor */
                if (listPtr->compare(newData, dataPtr) >= 0) {
                    /* Must recover the previous pointer by recursing backwards */
                    if (!prevPtr && (!params->rv.ptr || TM_SHARED_READ_TAG_P(((list_node_t *)params->rv.ptr)->nextPtr, (uintptr_t)params->rv.ptr) != nodePtr)) {
                        prevPtr = TMfindPrevious((list_t *)listPtr, (void *)newData);
# ifdef TM_DEBUG
                        printf("LIST_PREVIOUS addr:%p listPtr:%p dataPtr:%p recovered prevPtr:%p", params->addr, listPtr, dataPtr, prevPtr);
# endif
                    }
                /* Ensure that the iteration remains unchanged */
                } else {
                    ASSERT((listPtr->compare(oldData, dataPtr) >= 0) == (listPtr->compare(newData, dataPtr) >= 0));
                    prevPtr = params->rv.ptr;
                }

                goto out;
            } else {
                ASSERT(params->addr == &base->nextPtr);
                /* Conflict is at the next pointer */
                prevPtr = base;
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, prevPtr->nextPtr, oldNext));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, prevPtr->nextPtr, nodePtr));
                if (oldNext == nodePtr)
                    return STM_MERGE_OK;
next:
# ifdef TM_DEBUG
                printf("LIST_PREVIOUS addr:%p listPtr:%p dataPtr:%p prevPtr:%p prevPtr.next(old):%p prevPtr.next(new):%p\n", params->addr, listPtr, dataPtr, prevPtr, oldNext, nodePtr);
# endif

                /* Check if the new next node is valid */
                if (!nodePtr) {
                    /* This node is the new prevPtr */
                    goto out;
                /* Check if the current node has been freed */
                } else if (nodePtr == TMLIST_BAD_ADDR) {
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));

                    /* Conflict must be earlier in LIST_PREVIOUS, since this node is the predecessor but has been freed */
                    if (nodePtr == params->rv.ptr)
                        return STM_MERGE_RETRY;
                    prevPtr = params->rv.ptr;
                    goto out;
                /* Check if the new next node has been visited before */
                }
                /* Fall through and iterate on the next node */
            }

            /* Iterate on up to one previously visited node, because the return value is only set upon encountering the next node */
            unsigned counter = 0;
            while (nodePtr) {
                /* Read the data pointer */
                r = TM_SHARED_DID_READ(nodePtr->dataPtr);
                if (!STM_VALID_READ(r)) {
                    newData = TM_SHARED_READ_TAG_P(nodePtr->dataPtr, (uintptr_t)nodePtr);
                    /* We should never reach a node that has been freed, because it should have been handled earlier in LIST_REMOVE. Assume the list has been modified since conflict detection on LIST_REMOVE. */
                    if (__builtin_expect(newData == TMLIST_BAD_ADDR, 0))
                        return STM_MERGE_RETRY;
                } else {
                    /* Early termination when previously visited nodes are encountered */
                    if (nodePtr != params->rv.ptr && counter++ >= 1) {
                        prevPtr = params->rv.ptr;
                        break;
                    }

                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, nodePtr->dataPtr, oldData));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, nodePtr->dataPtr, newData));
                    if (oldData != newData)
                        goto data;
                }
# ifdef TM_DEBUG
                printf("LIST_PREVIOUS addr:%p listPtr:%p dataPtr:%p nodePtr:%p .dataPtr:%p\n", params->addr, listPtr, dataPtr, nodePtr, newData);
# endif
                /* Check the data pointer */
                if (listPtr->compare(newData, dataPtr) >= 0) {
                    ASSERT(prevPtr);
                    break;
                }

                /* Continue iterating */
                prevPtr = nodePtr;

                r = TM_SHARED_DID_READ(nodePtr->nextPtr);
                if (!STM_VALID_READ(r)) {
                    nodePtr = TM_SHARED_READ_TAG_P(nodePtr->nextPtr, (uintptr_t)nodePtr);
                    /* We should never reach a node that has been freed, because it should have been handled earlier in LIST_REMOVE. Assume the list has been modified since conflict detection on LIST_REMOVE. */
                    if (__builtin_expect(nodePtr == TMLIST_BAD_ADDR, 0))
                        return STM_MERGE_RETRY;
                } else {
                    /* Must restart upon encountering another conflict, otherwise the correct node may be missed */
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, nodePtr->nextPtr, oldNext));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, nodePtr->nextPtr, nodePtr));
                    if (oldNext != nodePtr)
                        goto next;
                }
# ifdef TM_DEBUG
                printf("LIST_PREVIOUS addr:%p listPtr:%p dataPtr:%p nodePtr:%p .nextPtr:%p\n", params->addr, listPtr, dataPtr, prevPtr, nodePtr);
# endif
            }

out:
# ifdef TM_DEBUG
            printf("LIST_PREVIOUS addr:%p listPtr:%p dataPtr:%p rv(old):%p rv(new):%p\n", params->addr, listPtr, dataPtr, params->rv.ptr, prevPtr);
# endif
            /* Pass the new return value */
            params->rv.ptr = (list_node_t *)prevPtr;
            return STM_MERGE_OK;
        } else if (STM_SAME_OPID(op, LIST_FIND)) {
            stm_read_t rdata;
            const list_node_t *oldNode, *nodePtr;

            /* Conflict occurred directly inside LIST_FIND */
            if (params->leaf == 1) {
# ifdef TM_DEBUG
                printf("\nLIST_FIND addr:%p listPtr:%p dataPtr:%p\n", params->addr, listPtr, dataPtr);
# endif

                ASSERT(!STM_VALID_OP(params->previous));
                ASSERT(ENTRY_VALID(params->conflict.entries->e1));
                stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
                ASSERT(STM_VALID_READ(r));

                /* Recover conflicting pointer from address */
                const list_node_t *base = (list_node_t *)TM_SHARED_GET_TAG(r);
                ASSERT(base);
                if (params->addr == &base->nextPtr) {
                    /* Conflict is at the previous node */
                    const list_node_t *prevPtr = base;
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, prevPtr->nextPtr, oldNode));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, prevPtr->nextPtr, nodePtr));
                    if (oldNode == nodePtr)
                        return STM_MERGE_OK;
# ifdef TM_DEBUG
                    printf("LIST_FIND addr:%p listPtr:%p dataPtr:%p prevPtr:%p prevPtr.next(old):%p prevPtr.next(new):%p\n", params->addr, listPtr, dataPtr, prevPtr, oldNode, nodePtr);
# endif

                    rdata = oldNode ? TM_SHARED_DID_READ(oldNode->dataPtr) : STM_INVALID_READ;
                } else {
                    /* Conflict is at the data pointer */
                    oldNode = nodePtr = base;
                    ASSERT(params->addr == &nodePtr->dataPtr);
                    rdata = r;
                    ASSERT(!STM_VALID_WRITE(TM_SHARED_DID_WRITE(nodePtr->dataPtr)));
                }
            /* Update occurred from LIST_PREVIOUS */
            } else {
                ASSERT(params->leaf == 0);
                ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->previous), LIST_PREVIOUS));
# ifdef TM_DEBUG
                printf("\nLIST_FIND addr:%p listPtr:%p dataPtr:%p <- LIST_PREVIOUS %p\n", params->addr, listPtr, dataPtr, params->conflict.result.ptr);
# endif
                /* Get the previous predecessor */
                const list_node_t *prev_pred = (list_node_t *)params->conflict.previous_result.ptr;
                ASSERT(prev_pred);
                const stm_read_t rprev = TM_SHARED_DID_READ(prev_pred->nextPtr);
                ASSERT(STM_VALID_READ(rprev));
                /* Explicitly set the read position */
                params->read = rprev;
                /* Get the previous node */
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(rprev, prev_pred->nextPtr, oldNode));
                ASSERT_FAIL(TM_SHARED_UNDO_READ(rprev));
                /* Get the read set entry of the node's data pointer */
                rdata = oldNode ? TM_SHARED_DID_READ(oldNode->dataPtr) : STM_INVALID_READ;
                ASSERT(!oldNode || STM_VALID_READ(rdata));
                /* Get the new node */
                ASSERT(params->conflict.result.ptr);
                ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(params->conflict.result.ptr)));
                nodePtr = TM_SHARED_READ_TAG_P(((list_node_t *)params->conflict.result.ptr)->nextPtr, (uintptr_t)params->conflict.result.ptr);
                ASSERT(oldNode == nodePtr || (oldNode != nodePtr && !STM_VALID_READ(TM_SHARED_DID_READ(nodePtr->dataPtr))));
            }

            const void *oldData, *newData;
            /* Read the old and new data pointer */
            if (oldNode && oldNode == nodePtr) {
                ASSERT(STM_VALID_READ(rdata));
                ASSERT(nodePtr);
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(rdata, oldNode->dataPtr, oldData));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(rdata, nodePtr->dataPtr, newData));
                if (oldData == newData)
                    return STM_MERGE_OK;

                ASSERT(params->addr != &nodePtr->dataPtr);
            } else {
                if (oldNode) {
                    ASSERT(STM_VALID_READ(rdata));
# ifdef TM_DEBUG
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(rdata, oldNode->dataPtr, oldData));
# endif
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(rdata));
                } else {
                    ASSERT(!STM_VALID_READ(rdata));
# ifdef TM_DEBUG
                    oldData = NULL;
# endif
                }
                if (nodePtr) {
                    ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(nodePtr->dataPtr)));
                    newData = TM_SHARED_READ_TAG_P(nodePtr->dataPtr, (uintptr_t)nodePtr);
                } else {
                    newData = NULL;
                }
            }

# ifdef TM_DEBUG
            printf("LIST_FIND addr:%p listPtr:%p dataPtr:%p nodePtr:%p nodePtr.data(old):%p nodePtr.data(new):%p\n", params->addr, listPtr, dataPtr, nodePtr, oldData, newData);
# endif
            const void *ret = nodePtr && listPtr->compare(newData, dataPtr) == 0 ? newData : NULL;
# ifdef TM_DEBUG
            printf("LIST_FIND addr:%p listPtr:%p dataPtr:%p rv(old):%p rv(new):%p\n", params->addr, listPtr, dataPtr, params->rv.ptr, ret);
# endif
            params->rv.ptr = (void *)ret;
            return STM_MERGE_OK;
        } else if (STM_SAME_OPID(op, LIST_INSERT)) {
# ifdef LIST_NO_DUPLICATES
            stm_read_t rdata;
# endif
            stm_write_t w;
            bool_t rv = params->rv.sint;
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);
            const list_node_t *oldPrev, *prevPtr, *oldCurr;
            list_node_t *currPtr;
            /* Conflict occurred directly inside LIST_INSERT */
            if (params->leaf == 1) {
                ASSERT(!STM_VALID_OP(params->previous));
                ASSERT(ENTRY_VALID(params->conflict.entries->e1));
                stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
                ASSERT(STM_VALID_READ(r));

                /* Conflict is at the list size */
                if (params->addr == &listPtr->size) {
# ifdef TM_DEBUG
                    long old;
                    ASSERT_FAIL(TM_SHARED_READ_VALUE(r, listPtr->size, old));
# endif
                    long new;
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, listPtr->size, new));
                    w = TM_SHARED_DID_WRITE(listPtr->size);
                    ASSERT(STM_VALID_WRITE(w));
                    ASSERT(STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
# ifdef TM_DEBUG
                    printf("LIST_INSERT addr:%p listPtr:%p dataPtr:%p listPtr->size read (old):%ld (new):%ld, write (new):%ld\n", params->addr, listPtr, dataPtr, old, new, new + 1);
# endif
                    ASSERT(params->rv.sint == TRUE);
                    ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, listPtr->size, new + 1));
                    return STM_MERGE_OK;
                }

                /* Recover conflicting pointer from address */
                list_node_t *base = (list_node_t *)TM_SHARED_GET_TAG(r);
                ASSERT(base);
                if (params->addr == &base->nextPtr) {
                    /* Conflict is at the next pointer */
                    prevPtr = base;
                    ASSERT(params->addr == &prevPtr->nextPtr);
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, prevPtr->nextPtr, oldCurr));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, prevPtr->nextPtr, currPtr));
                    if (oldCurr == currPtr)
                        return STM_MERGE_OK;
# ifdef TM_DEBUG
                    printf("LIST_INSERT addr:%p listPtr:%p dataPtr:%p prevPtr:%p prevPtr.next(old):%p prevPtr.next(new):%p\n", params->addr, listPtr, dataPtr, prevPtr, oldCurr, currPtr);
# endif

                    w = TM_SHARED_DID_WRITE(base->nextPtr);
# ifdef LIST_NO_DUPLICATES
                    rdata = oldCurr ? TM_SHARED_DID_READ(oldCurr->dataPtr) : STM_INVALID_READ;
                    ASSERT(!oldCurr || STM_VALID_READ(rdata));
# endif
                } else {
# ifdef LIST_NO_DUPLICATES
                    /* Conflict is at the data pointer */
                    prevPtr = NULL;
                    w = STM_INVALID_WRITE;
                    oldCurr = currPtr = base;
                    ASSERT(params->addr == &currPtr->dataPtr);
                    rdata = r;
                    ASSERT(!STM_VALID_WRITE(TM_SHARED_DID_WRITE(currPtr->dataPtr)));
# else
#  ifdef TM_DEBUG
                    printf("LIST_INSERT addr:%p listPtr:%p dataPtr:%p UNSUPPORTED:%p\n", params->addr, listPtr, dataPtr, params->addr);
#  endif
                    return STM_MERGE_ABORT;
# endif
                }
            /* Update occurred from LIST_PREVIOUS */
            } else {
                ASSERT(params->leaf == 0);
                ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->previous), LIST_PREVIOUS));
# ifdef TM_DEBUG
                printf("\nLIST_INSERT listPtr:%p dataPtr:%p <- LIST_PREVIOUS %p\n", listPtr, dataPtr, params->conflict.result.ptr);
# endif
                /* Get the previous insertion */
                oldPrev = (list_node_t *)params->conflict.previous_result.ptr;
                ASSERT(oldPrev);

                /* Get the previous next pointer and undo the read */
                const stm_read_t r = TM_SHARED_DID_READ(oldPrev->nextPtr);
                ASSERT(STM_VALID_READ(r));
                /* Explicitly set the read position */
                params->read = r;
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, oldPrev->nextPtr, oldCurr));
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                w = TM_SHARED_DID_WRITE(oldPrev->nextPtr);
# ifdef LIST_NO_DUPLICATES
                rdata = oldCurr && oldCurr != TMLIST_BAD_ADDR ? TM_SHARED_DID_READ(oldCurr->dataPtr) : STM_INVALID_READ;
                ASSERT(!oldCurr || oldCurr == TMLIST_BAD_ADDR || STM_VALID_READ(rdata));
# endif

                /* Prepare for the new insertion */
                ASSERT(params->conflict.result.ptr);
                prevPtr = (list_node_t *)params->conflict.result.ptr;
                ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(prevPtr->nextPtr)));
                currPtr = TM_SHARED_READ_TAG_P(prevPtr->nextPtr, (uintptr_t)prevPtr);
            }

            /* Check if currPtr->dataPtr has changed and update the return value */
# if defined(LIST_NO_DUPLICATES) || defined(TM_DEBUG)
            const void *oldData = NULL;
# endif /* LIST_NO_DUPLICATES || TM_DEBUG */
            const void *newData = NULL;
# ifdef LIST_NO_DUPLICATES
            if (oldCurr == currPtr) {
                ASSERT(oldCurr != TMLIST_BAD_ADDR);
                if (oldCurr) {
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(rdata, oldCurr->dataPtr, oldData));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(rdata, currPtr->dataPtr, newData));

                    if (params->addr == &oldCurr->dataPtr && oldData == newData)
                        return STM_MERGE_OK;
                }
            } else {
                if (__builtin_expect(oldCurr && oldCurr != TMLIST_BAD_ADDR, 1)) {
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(rdata, oldCurr->dataPtr, oldData));
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(rdata));
                }
                if (__builtin_expect(currPtr && currPtr != TMLIST_BAD_ADDR, 1)) {
                    ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(currPtr->dataPtr)));
                    newData = TM_SHARED_READ_TAG_P(currPtr->dataPtr, (uintptr_t)currPtr);
                }
            }
# else
            newData = currPtr ? currPtr->dataPtr : NULL;
# endif
# ifdef TM_DEBUG
            printf("LIST_INSERT addr:%p listPtr:%p dataPtr:%p currPtr(old):%p currPtr(new):%p currPtr.data(old):%p currPtr.data(new):%p\n", params->addr, listPtr, dataPtr, oldCurr, currPtr, oldData, newData);
# endif

# ifdef LIST_NO_DUPLICATES
            /* Data must be valid, otherwise there would be an update from LIST_PREVIOUS */
            if (__builtin_expect(newData == TMLIST_BAD_ADDR, 0))
                return STM_MERGE_RETRY;

            if (oldData != newData)
                rv = __builtin_expect(currPtr && currPtr != TMLIST_BAD_ADDR && newData != TMLIST_BAD_ADDR, 1) && !listPtr->compare(newData, dataPtr) ? FALSE : TRUE;
# else
            // FIXME: Race between LIST_PREVIOUS and LIST_INSERT
            if (currPtr && (__builtin_expect(newData == TMLIST_BAD_ADDR, 0) || listPtr->compare(newData, dataPtr) < 0))
                return STM_MERGE_ABORT;
# endif

            list_node_t *nodePtr;
            /* Handle the update based on the previous and current return values */
            if (params->rv.sint == TRUE) {
                /* Recover pointer to the previously allocated pointer */
                const stm_read_t rsize = TM_SHARED_DID_READ(listPtr->size);
                ASSERT(STM_VALID_READ(rsize));
                nodePtr = (list_node_t *)TM_SHARED_GET_TAG(rsize);
                ASSERT((void *)nodePtr != listPtr);

                /* Restore the next pointer of the previous node. This is necessary if the previous insertion occurred at the wrong point, or if should no longer be inserted. */
                if ((rv == TRUE && !params->leaf) || rv == FALSE) {
                    /* Conflict was on data pointer, previous pointer is not available */
                    if (params->leaf == 1 && prevPtr && params->addr != &prevPtr->nextPtr && !STM_VALID_WRITE(w)) {
#  ifdef TM_DEBUG
                        printf("LIST_INSERT addr:%p listPtr:%p dataPtr:%p UNSUPPORTED:%p\n", params->addr, listPtr, dataPtr, params->addr);
#  endif
                        return STM_MERGE_ABORT;
                    }

                    ASSERT(STM_VALID_WRITE(w));
                    /* Must revert the write because it is at the wrong point and may be stale. */
                    ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
                }

                /* A node was previously inserted, and should remain inserted */
                if (rv == TRUE) {
                    ASSERT(prevPtr);
                    /* Update the node insertion. Note that it is possible for the write to be missing; e.g. if multiple inserts occur at the same node, and a previous one was reverted after merging. */
                    if (params->leaf == 1 && STM_VALID_WRITE(w)) {
                        ASSERT_FAIL(TM_SHARED_WRITE_UPDATE_P(w, prevPtr->nextPtr, nodePtr));
                    } else {
                        /* Change the read position to avoid modifying any reads in this operation */
                        params->read = stm_get_load_next(stm_get_load_last(params->read), 0, 0);
                        TM_SHARED_WRITE_P(prevPtr->nextPtr, nodePtr);
                    }

                    /* Update the next node */
                    ASSERT(nodePtr && STM_VALID_MALLOC(TM_DID_MALLOC(nodePtr)));
                    ASSERT(nodePtr != currPtr);
                    if (__builtin_expect(currPtr != TMLIST_BAD_ADDR, 1)) {
                        w = TM_SHARED_DID_WRITE(nodePtr->nextPtr);
                        ASSERT(STM_VALID_WRITE(w));
                        ASSERT_FAIL(TM_SHARED_WRITE_UPDATE_P(w, nodePtr->nextPtr, currPtr));
                    }
                } else {
                    /* A node was previously inserted, but now should not */

                    /* Deallocate the new list node */
                    ASSERT(nodePtr);
                    stm_malloc_t m = TM_DID_MALLOC(nodePtr);
                    ASSERT(STM_VALID_MALLOC(m));
                    w = TM_SHARED_DID_WRITE(nodePtr->nextPtr);
                    ASSERT(STM_VALID_WRITE(w));
                    ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
                    ASSERT_FAIL(TM_UNDO_MALLOC(m));

                    /* Undo modification to the list size */
                    const stm_read_t rsize = TM_SHARED_DID_READ(listPtr->size);
                    ASSERT(STM_VALID_READ(rsize));
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(rsize));
                    w = TM_SHARED_DID_WRITE(listPtr->size);
                    ASSERT(STM_VALID_WRITE(w));
                    ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
                }
            } else {
                if (rv == TRUE) {
                    /* No node was previously inserted, but now one should be */

                    /* Allocate the new node */
                    ASSERT(!nodePtr);
                    nodePtr = TMallocNode((list_node_t *)dataPtr);
                    if (__builtin_expect(!nodePtr, 0))
                        return STM_MERGE_ABORT;

                    /* Insert it into the list */
                    if (__builtin_expect(currPtr != TMLIST_BAD_ADDR, 1))
                        TM_SHARED_WRITE_P(nodePtr->nextPtr, currPtr);
                    ASSERT(!STM_VALID_WRITE(TM_SHARED_DID_WRITE(prevPtr->nextPtr)));
                    TM_SHARED_WRITE_P(prevPtr->nextPtr, nodePtr);

                    /* Increase the list size */
                    ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(listPtr->size)));
                    ASSERT(!STM_VALID_WRITE(TM_SHARED_DID_WRITE(listPtr->size)));
                    TM_SHARED_WRITE(listPtr->size, TM_SHARED_READ_TAG(listPtr->size, (uintptr_t)nodePtr) + 1);
                } else {
# ifdef TM_DEBUG
                    printf("LIST_INSERT addr:%p listPtr:%p dataPtr:%p UNSUPPORTED rv:%ld\n", params->addr, listPtr, dataPtr, rv);
# endif
                    /* FIXME: Conflict unsupported */
                    ASSERT(0);
                    return STM_MERGE_ABORT;
                }
            }

            if (__builtin_expect(currPtr == TMLIST_BAD_ADDR || newData == TMLIST_BAD_ADDR, 0))
                return STM_MERGE_RETRY;
# ifdef TM_DEBUG
            printf("LIST_INSERT addr:%p listPtr:%p dataPtr:%p rv(old):%ld rv(new):%ld\n", params->addr, listPtr, dataPtr, params->rv.sint, rv);
# endif
            params->rv.sint = rv;
            return STM_MERGE_OK;
        } else {
            stm_write_t w;
            bool_t rv = params->rv.sint;
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);
            const list_node_t *oldNode, *nodePtr, *oldPrev, *prevPtr, *nextPtr;

            /* Get the node being removed */
            const stm_read_t rnode = TM_SHARED_DID_READ(listPtr->size);
            ASSERT(STM_VALID_READ(rnode));
            oldNode = (list_node_t *)TM_SHARED_GET_TAG(rnode);
            ASSERT(oldNode);

            if (rv != TRUE) {
                /* FIXME: Conflict unsupported */
                ASSERT(0);
#  ifdef TM_DEBUG
                printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p UNSUPPORTED rv:%ld\n", params->addr, listPtr, dataPtr, rv);
#  endif
                return STM_MERGE_ABORT;
            }

            /* Conflict occurred directly inside LIST_REMOVE */
            if (params->leaf == 1) {
                ASSERT(!STM_VALID_OP(params->previous));
                ASSERT(ENTRY_VALID(params->conflict.entries->e1));
                stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
                ASSERT(STM_VALID_READ(r));

                /* Conflict is at the list size */
                if (params->addr == &listPtr->size) {
# ifdef TM_DEBUG
                    long old;
                    ASSERT_FAIL(TM_SHARED_READ_VALUE(r, listPtr->size, old));
# endif
                    long new;
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, listPtr->size, new));
                    w = TM_SHARED_DID_WRITE(listPtr->size);
                    ASSERT(STM_VALID_WRITE(w));
                    ASSERT(STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
# ifdef TM_DEBUG
                    printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p listPtr->size read (old):%ld (new):%ld, write (new):%ld\n", params->addr, listPtr, dataPtr, old, new, new - 1);
# endif
                    ASSERT(params->rv.sint == TRUE);
                    ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, listPtr->size, new - 1));
                    return STM_MERGE_OK;
                }

                /* Get the predecessor of the node being removed */
                const stm_op_t prev_op = stm_find_op_descendant(params->current, LIST_PREVIOUS);
                ASSERT(STM_VALID_OP(prev_op));
                stm_union_t prev_ret;
                const int prev_type = stm_get_op_ret(prev_op, &prev_ret);
                ASSERT(prev_type == FFI_TYPE_POINTER);
                oldPrev = prev_ret.ptr;
                ASSERT(oldPrev);
                if (params->addr == &oldNode->dataPtr) {
                    /* Conflict is at the data pointer */
                    const void *old, *new;
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, oldNode->dataPtr, old));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, oldNode->dataPtr, new));

                    if (old == new)
                        return STM_MERGE_OK;
# ifdef TM_DEBUG
                    printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p nodePtr:%p nodePtr.dataPtr(old):%p nodePtr.dataPtr(new):%p\n", params->addr, listPtr, dataPtr,oldNode, old, new);
# endif
                    /* Data must be the same, otherwise there would be an update from LIST_PREVIOUS */
                    if (__builtin_expect(new == TMLIST_BAD_ADDR, 0))
                        return STM_MERGE_RETRY;
# ifdef TM_DEBUG
                    printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p UNSUPPORTED\n", params->addr, listPtr, dataPtr);
# endif
                    /* FIXME: Conflict unsupported */
                    ASSERT(0);
                    return STM_MERGE_ABORT;
                } else if (params->addr == &oldNode->nextPtr) {
                    /* Conflict is at the next pointer */
                    const list_node_t *oldNext;
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, oldNode->nextPtr, oldNext));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, oldNode->nextPtr, nextPtr));
                    if (oldNext == nextPtr)
                        return STM_MERGE_OK;
# ifdef TM_DEBUG
                    printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p nodePtr:%p nodePtr.next(old):%p nodePtr.next(new):%p\n", params->addr, listPtr, dataPtr, oldNode, oldNext, nextPtr);
# endif

                    /* Update the predecessor */
                    const stm_write_t wprev = TM_SHARED_DID_WRITE(oldPrev->nextPtr);
                    ASSERT(STM_VALID_WRITE(wprev));
                    ASSERT_FAIL(TM_SHARED_WRITE_UPDATE_P(wprev, oldPrev->nextPtr, nextPtr));
                } else if (params->addr == &oldPrev->nextPtr) {
                    /* Conflict is at the previous node's next pointer */
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, oldPrev->nextPtr, oldNode));
                    ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, oldPrev->nextPtr, nodePtr));
                    if (oldNode == nodePtr)
                        return STM_MERGE_OK;
# ifdef TM_DEBUG
                    printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p prevPtr:%p prevPtr.next(old):%p prevPtr.next(new):%p\n", params->addr, listPtr, dataPtr, oldPrev, oldNode, nodePtr);
# endif
                    /* Node must be the same, otherwise it would be an update from LIST_PREVIOUS */
                    if (__builtin_expect(oldNode != nodePtr, 0))
                        return STM_MERGE_RETRY;

                    /* Restore the write to the next node */
                    w = TM_SHARED_DID_WRITE(oldPrev->nextPtr);
                    ASSERT(STM_VALID_WRITE(w));
                    ASSERT(STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
                    const stm_read_t rnext = TM_SHARED_DID_READ(oldNode->nextPtr);
                    ASSERT(STM_VALID_READ(rnext));
                    const list_node_t *oldNext;
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(rnext, oldNode->nextPtr, oldNext));
                    ASSERT_FAIL(TM_SHARED_WRITE_UPDATE_P(w, oldPrev->nextPtr, oldNext));
                } else {
# ifdef TM_DEBUG
                    printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p UNSUPPORTED:%p\n", params->addr, listPtr, dataPtr, params->addr);
# endif
                    /* FIXME: Conflict unsupported */
                    ASSERT(0);
                    return STM_MERGE_ABORT;
                }
            /* Update occurred from LIST_PREVIOUS */
            } else {
                ASSERT(params->leaf == 0);
                ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->previous), LIST_PREVIOUS));
                prevPtr = params->conflict.result.ptr;
# ifdef TM_DEBUG
                printf("\nLIST_REMOVE listPtr:%p dataPtr:%p <- LIST_PREVIOUS %p\n", listPtr, dataPtr, prevPtr);
# endif
                /* FIXME */
                ASSERT(params->rv.sint == TRUE);

                /* Get the previous node and undo its changes */
                oldPrev = (list_node_t *)params->conflict.previous_result.ptr;
                ASSERT(oldPrev);
                stm_read_t r = TM_SHARED_DID_READ(oldPrev->nextPtr);
                ASSERT(STM_VALID_READ(r));
                /* Explicitly set the read position */
                params->read = r;
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                stm_write_t w = TM_SHARED_DID_WRITE(oldPrev->nextPtr);
                ASSERT(STM_VALID_WRITE(w));
                ASSERT_FAIL(TM_SHARED_WRITE_VALUE_P(w, oldPrev->nextPtr, nextPtr));
                ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));

                nodePtr = TM_SHARED_READ_TAG_P(prevPtr->nextPtr, (uintptr_t)prevPtr);

                /* Different node is to be deleted */
                if (oldNode == nodePtr || nodePtr == TMLIST_BAD_ADDR) {
                    /* Change the read position to avoid modifying any reads in this operation */
                    params->read = stm_get_load_next(stm_get_load_last(params->read), 0, 0);
                    /* Write the same next pointer */
                    TM_SHARED_WRITE_P(prevPtr->nextPtr, nextPtr);
                    rv = TRUE;
                } else {
                    /* Undo operations on the deleted node */
                    r = TM_SHARED_DID_READ(oldNode->dataPtr);
                    ASSERT(STM_VALID_READ(r));
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    r = TM_SHARED_DID_READ(oldNode->nextPtr);
                    ASSERT(STM_VALID_READ(r));
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    w = TM_SHARED_DID_WRITE(oldNode->nextPtr);
                    ASSERT(STM_VALID_WRITE(w));
                    ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
                    ASSERT_FAIL(TMfreeNodeUndo(nodePtr) != STM_MERGE_ABORT);

                    /* Delete the new node */
                    if (nodePtr && listPtr->compare(TM_SHARED_READ_TAG_P(nodePtr->dataPtr, (uintptr_t)nodePtr), dataPtr) == 0) {
                        TM_SHARED_WRITE_P(prevPtr->nextPtr, TM_SHARED_READ_TAG_P(nodePtr->nextPtr, (uintptr_t)nodePtr));
                        TM_SHARED_WRITE_P(nodePtr->nextPtr, (struct list_node*)NULL);
                        TMfreeNode((void *)nodePtr);
                        rv = TRUE;
                    } else
                        rv = FALSE;
                }

                if (params->rv.sint == TRUE && rv == TRUE) {
                    if (__builtin_expect(nodePtr != TMLIST_BAD_ADDR, 1)) {
                        /* A node was previously removed, and should remain removed */
                        /* Set the tag to the new deleted node */
                        r = TM_SHARED_DID_READ(listPtr->size);
                        ASSERT(STM_VALID_READ(r));
                        ASSERT_FAIL(TM_SHARED_SET_TAG(r, (uintptr_t)nodePtr));
                    }
                } else if (params->rv.sint == TRUE && rv == FALSE) {
                    /* Undo modification to the list size */
                    r = TM_SHARED_DID_READ(listPtr->size);
                    ASSERT(STM_VALID_READ(r));
                    w = TM_SHARED_DID_WRITE(listPtr->size);
                    ASSERT(STM_VALID_WRITE(w));
                    ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                } else {
# ifdef TM_DEBUG
                    printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p UNSUPPORTED rv(old):%ld rv(new):%ld\n", params->addr, listPtr, dataPtr, params->rv.sint, rv);
# endif
                    /* FIXME: Conflict unsupported */
                    ASSERT(0);
                    return STM_MERGE_ABORT;
                }
            }

# ifdef TM_DEBUG
            printf("LIST_REMOVE addr:%p listPtr:%p dataPtr:%p rv(old):%ld rv(new):%ld\n", params->addr, listPtr, dataPtr, params->rv.sint, rv);
# endif
            params->rv.sint = rv;
            return STM_MERGE_OK;
        }
    }

# ifdef TM_DEBUG
    printf("\nLIST_MERGE UNSUPPORTED addr:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}

#  define TMLIST_MERGE TMlist_merge
# else
#  define TMLIST_MERGE NULL
# endif /* MERGE_LIST */

__attribute__((constructor)) void list_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*p[], {&ffi_type_pointer});
    TM_LOG_TYPE_DECLARE_INIT(*pp[], {&ffi_type_pointer, &ffi_type_pointer});
    # define TM_LOG_OP TM_LOG_OP_INIT
    # include "list.inc"
    # undef TM_LOG_OP
}
#endif /* ORIGINAL */
