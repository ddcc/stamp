/* =============================================================================
 *
 * list.h
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


#ifndef LIST_H
#define LIST_H 1

#include "tm.h"
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct list_node {
    void* dataPtr;
    struct list_node* nextPtr;
} list_node_t;

typedef list_node_t* list_iter_t;

typedef struct list {
    list_node_t head;
    TM_PURE long (*compare)(const void*, const void*);   /* returns {-1,0,1}, 0 -> equal */
    long size;
} list_t;

/* =============================================================================
 * list_compare
 * =============================================================================
 */
long
list_compare (const void* a, const void* b);


/* =============================================================================
 * HTMlist_iter_reset
 * =============================================================================
 */
void
HTMlist_iter_reset (list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * list_iter_reset
 * =============================================================================
 */
TM_PURE
void
list_iter_reset (list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * HTMlist_iter_reset
 * =============================================================================
 */
void
HTMlist_iter_reset (list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * TMlist_iter_reset
 * =============================================================================
 */
TM_CALLABLE
void
TMlist_iter_reset (TM_ARGDECL  list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * list_iter_hasNext
 * =============================================================================
 */
TM_PURE
bool_t
list_iter_hasNext (list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * HTMlist_iter_hasNext
 * =============================================================================
 */
bool_t
HTMlist_iter_hasNext (list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * TMlist_iter_hasNext
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMlist_iter_hasNext (TM_ARGDECL  list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * list_iter_next
 * =============================================================================
 */
TM_PURE
void*
list_iter_next (list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * HTMlist_iter_next
 * =============================================================================
 */
void*
HTMlist_iter_next (list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * TMlist_iter_next
 * =============================================================================
 */
TM_CALLABLE
void*
TMlist_iter_next (TM_ARGDECL  list_iter_t* itPtr, list_t* listPtr);


/* =============================================================================
 * list_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
list_t*
list_alloc (long (*compare)(const void*, const void*));


/* =============================================================================
 * Plist_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
list_t*
Plist_alloc (long (*compare)(const void*, const void*));


/* =============================================================================
 * HTMlist_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
list_t*
HTMlist_alloc (long (*compare)(const void*, const void*));


/* =============================================================================
 * TMlist_alloc
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
TM_CALLABLE
list_t*
TMlist_alloc (TM_ARGDECL  long (*compare)(const void*, const void*));


/* =============================================================================
 * list_free
 * =============================================================================
 */
void
list_free (list_t* listPtr, void (*freeData)(void *));


/* =============================================================================
 * Plist_free
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
void
Plist_free (list_t* listPtr, void (*PfreeData)(void *));


/* =============================================================================
 * HTMlist_free
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
void
HTMlist_free (list_t* listPtr, void (*HTMfreeData)(void *));


/* =============================================================================
 * TMlist_free
 * -- If NULL passed for 'compare' function, will compare data pointer addresses
 * -- Returns NULL on failure
 * =============================================================================
 */
TM_CALLABLE
void
TMlist_free (TM_ARGDECL  list_t* listPtr, TM_CALLABLE void (*TMfreeData)(void *));


/* =============================================================================
 * list_isEmpty
 * -- Return TRUE if list is empty, else FALSE
 * =============================================================================
 */
bool_t
list_isEmpty (list_t* listPtr);


/* =============================================================================
 * TMlist_isEmpty
 * -- Return TRUE if list is empty, else FALSE
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMlist_isEmpty (TM_ARGDECL  list_t* listPtr);


/* =============================================================================
 * list_getSize
 * -- Returns size of list
 * =============================================================================
 */
TM_PURE
long
list_getSize (list_t* listPtr);


/* =============================================================================
 * HTMlist_getSize
 * -- Returns size of list
 * =============================================================================
 */
long
HTMlist_getSize (list_t* listPtr);


/* =============================================================================
 * TMlist_getSize
 * -- Returns size of list
 * =============================================================================
 */
TM_CALLABLE
long
TMlist_getSize (TM_ARGDECL  list_t* listPtr);


/* =============================================================================
 * list_find
 * -- Returns NULL if not found, else returns pointer to data
 * =============================================================================
 */
TM_PURE
void*
list_find (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * HTMlist_find
 * -- Returns NULL if not found, else returns pointer to data
 * =============================================================================
 */
void*
HTMlist_find (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * TMlist_find
 * -- Returns NULL if not found, else returns pointer to data
 * =============================================================================
 */
TM_CALLABLE
void*
TMlist_find (TM_ARGDECL  list_t* listPtr, void* dataPtr);


/* =============================================================================
 * list_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
list_insert (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * Plist_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
TM_PURE
bool_t
Plist_insert (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * Plist_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
HTMlist_insert (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * TMlist_insert
 * -- Return TRUE on success, else FALSE
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMlist_insert (TM_ARGDECL  list_t* listPtr, void* dataPtr);


/* =============================================================================
 * list_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
bool_t
list_remove (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * Plist_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
bool_t
Plist_remove (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * HTMlist_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
bool_t
HTMlist_remove (list_t* listPtr, void* dataPtr);


/* =============================================================================
 * TMlist_remove
 * -- Returns TRUE if successful, else FALSE
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMlist_remove (TM_ARGDECL  list_t* listPtr, void* dataPtr);


/* =============================================================================
 * list_clear
 * -- Removes all elements
 * =============================================================================
 */
void
list_clear (list_t* listPtr, void (*freeData)(void *));


/* =============================================================================
 * Plist_clear
 * -- Removes all elements
 * =============================================================================
 */
TM_PURE
void
Plist_clear (list_t* listPtr, void (*PfreeData)(void *));


/* =============================================================================
 * list_setCompare
 * =============================================================================
 */
void
list_setCompare (list_t* listPtr, long (*compare)(const void*, const void*));


/* =============================================================================
 * TMlist_setCompare
 * =============================================================================
 */
TM_CALLABLE
void
TMlist_setCompare (TM_ARGDECL  list_t* listPtr, long (*compare)(const void*, const void*));


#define LIST_ITER_RESET(it, list)       list_iter_reset(TM_ARG  it, list)
#define LIST_ITER_HASNEXT(it, list)     list_iter_hasNext(TM_ARG  it, list)
#define LIST_ITER_NEXT(it, list)        list_iter_next(TM_ARG  it, list)
#define LIST_ALLOC(cmp)                 list_alloc(cmp)
#define LIST_FREE(list, free)           list_free(list, free)
#define LIST_GETSIZE(list)              list_getSize(list)
#define LIST_INSERT(list, data)         list_insert(list, data)
#define LIST_REMOVE(list, data)         list_remove(list, data)
#define LIST_CLEAR(list, free)          list_clear(list, free)

#define PLIST_ALLOC(cmp)                Plist_alloc(cmp)
#define PLIST_FREE(list, free)          Plist_free(list, free)
#define PLIST_GETSIZE(list)             list_getSize(list)
#define PLIST_INSERT(list, data)        Plist_insert(list, data)
#define PLIST_REMOVE(list, data)        Plist_remove(list, data)
#define PLIST_CLEAR(list, free)         Plist_clear(list, free)

#define HTMLIST_ITER_RESET(it, list)    HTMlist_iter_reset(it, list)
#define HTMLIST_ITER_HASNEXT(it, list)  HTMlist_iter_hasNext(it, list)
#define HTMLIST_ITER_NEXT(it, list)     HTMlist_iter_next(it, list)
#define HTMLIST_ALLOC(cmp)              HTMlist_alloc(cmp)
#define HTMLIST_FREE(list, free)        HTMlist_free(list, free)
#define HTMLIST_GETSIZE(list)           HTMlist_getSize(list)
#define HTMLIST_ISEMPTY(list)           HTMlist_isEmpty(list)
#define HTMLIST_FIND(list, data)        HTMlist_find(list, data)
#define HTMLIST_INSERT(list, data)      HTMlist_insert(list, data)
#define HTMLIST_REMOVE(list, data)      HTMlist_remove(list, data)

#define TMLIST_ITER_RESET(it, list)     TMlist_iter_reset(TM_ARG  it, list)
#define TMLIST_ITER_HASNEXT(it, list)   TMlist_iter_hasNext(TM_ARG  it, list)
#define TMLIST_ITER_NEXT(it, list)      TMlist_iter_next(TM_ARG  it, list)
#define TMLIST_ALLOC(cmp)               TMlist_alloc(TM_ARG  cmp)
#define TMLIST_FREE(list, free)         TMlist_free(TM_ARG  list, free)
#define TMLIST_GETSIZE(list)            TMlist_getSize(TM_ARG  list)
#define TMLIST_ISEMPTY(list)            TMlist_isEmpty(TM_ARG  list)
#define TMLIST_FIND(list, data)         TMlist_find(TM_ARG  list, data)
#define TMLIST_INSERT(list, data)       TMlist_insert(TM_ARG  list, data)
#define TMLIST_REMOVE(list, data)       TMlist_remove(TM_ARG  list, data)

#ifdef __cplusplus
}
#endif


#endif /* LIST_H */


/* =============================================================================
 *
 * End of list.h
 *
 * =============================================================================
 */
