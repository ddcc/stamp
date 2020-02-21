/* =============================================================================
 *
 * heap.c
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
#include "heap.h"
#include "tm.h"
#include "types.h"

struct heap {
    void** elements;
    long size;
    long capacity;
    TM_PURE long (*compare)(const void*, const void*);
};


#define PARENT(i)       ((i) / 2)
#define LEFT_CHILD(i)   (2*i)
#define RIGHT_CHILD(i)  (2*(i) + 1)


#ifndef ORIGINAL
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "heap.inc"
# undef TM_LOG_OP
#endif /* ORIGINAL */


/* =============================================================================
 * DECLARATION OF TM_CALLABLE FUNCTIONS
 * =============================================================================
 */

TM_CALLABLE
static void
TMsiftUp (TM_ARGDECL  heap_t* heapPtr, long startIndex);

TM_CALLABLE
static void
TMheapify (TM_ARGDECL  heap_t* heapPtr, long startIndex);


/* =============================================================================
 * heap_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
heap_t*
heap_alloc (long initCapacity, long (*compare)(const void*, const void*))
{
    heap_t* heapPtr;

    heapPtr = (heap_t*)malloc(sizeof(heap_t));
    if (heapPtr) {
        long capacity = ((initCapacity > 0) ? (initCapacity) : (1));
        heapPtr->elements = (void**)malloc(capacity * sizeof(void*));
        assert(heapPtr->elements);
        heapPtr->size = 0;
        heapPtr->capacity = capacity;
        heapPtr->compare = compare;
    }

    return heapPtr;
}


/* =============================================================================
 * heap_free
 * =============================================================================
 */
void
heap_free (heap_t* heapPtr)
{
    free(heapPtr->elements);
    free(heapPtr);
}


/* =============================================================================
 * siftUp
 * =============================================================================
 */
static void
siftUp (heap_t* heapPtr, long startIndex)
{
    void** elements = heapPtr->elements;
    long (*compare)(const void*, const void*) = heapPtr->compare;

    long index = startIndex;
    while ((index > 1)) {
        long parentIndex = PARENT(index);
        void* parentPtr = elements[parentIndex];
        void* thisPtr   = elements[index];
        if (compare(parentPtr, thisPtr) >= 0) {
            break;
        }
        void* tmpPtr = parentPtr;
        elements[parentIndex] = thisPtr;
        elements[index] = tmpPtr;
        index = parentIndex;
    }
}


/* =============================================================================
 * HTMsiftUp
 * =============================================================================
 */
static void
HTMsiftUp (heap_t* heapPtr, long startIndex)
{
    void** elements = (void**)HTM_SHARED_READ_P(heapPtr->elements);
    long (*compare)(const void*, const void*) = heapPtr->compare;

    long index = startIndex;
    while ((index > 1)) {
        long parentIndex = PARENT(index);
        void* parentPtr = (void*)HTM_SHARED_READ_P(elements[parentIndex]);
        void* thisPtr   = (void*)HTM_SHARED_READ_P(elements[index]);
        if (compare(parentPtr, thisPtr) >= 0) {
            break;
        }
        void* tmpPtr = parentPtr;
        HTM_SHARED_WRITE_P(elements[parentIndex], thisPtr);
        HTM_SHARED_WRITE_P(elements[index], tmpPtr);
        index = parentIndex;
    }

    return;
}


/* =============================================================================
 * TMsiftUp
 * =============================================================================
 */
TM_CALLABLE
static void
TMsiftUp (TM_ARGDECL  heap_t* heapPtr, long startIndex)
{
#ifndef ORIGINAL
# ifdef MERGE_HEAP
    __label__ replay;

    stm_merge_t merge(stm_merge_context_t *params) {
        /* Conflict occurred directly inside HEAP_SIFTUP */
        ASSERT(params->leaf == 1);
        ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->current), HEAP_SIFTUP));

# ifdef TM_DEBUG
        printf("HEAP_SIFTUP_JIT heapPtr:%p startIndex:%li\n", heapPtr, startIndex);
# endif

        TM_FINISH_MERGE();
        goto replay;
    }
# endif /* MERGE_HEAP */

    TM_LOG_BEGIN(HEAP_SIFTUP,
# ifdef MERGE_HEAP
        merge
# else
        NULL
# endif /* MERGE_HEAP */
        , heapPtr, startIndex);
#endif /* ORIGINAL */
    void** elements = (void**)TM_SHARED_READ_P(heapPtr->elements);
    TM_PURE long (*compare)(const void*, const void*) = heapPtr->compare;

    long index = startIndex;
    while ((index > 1)) {
        long parentIndex = PARENT(index);
        void* parentPtr = (void*)TM_SHARED_READ_P(elements[parentIndex]);
        void* thisPtr   = (void*)TM_SHARED_READ_P(elements[index]);
        if (compare(parentPtr, thisPtr) >= 0) {
            break;
        }
        void* tmpPtr = parentPtr;
        TM_SHARED_WRITE_P(elements[parentIndex], thisPtr);
        TM_SHARED_WRITE_P(elements[index], tmpPtr);
        index = parentIndex;
    }

#ifndef ORIGINAL
    TM_LOG_END(HEAP_SIFTUP, NULL);
replay:
#endif /* ORIGINAL */
    return;
}


/* =============================================================================
 * heap_insert
 * -- Returns FALSE on failure
 * =============================================================================
 */
bool_t
heap_insert (heap_t* heapPtr, void* dataPtr)
{
    long size = heapPtr->size;
    long capacity = heapPtr->capacity;

    if ((size + 1) >= capacity) {
        long newCapacity = capacity * 2;
        void** newElements = (void**)malloc(newCapacity * sizeof(void*));
        if (newElements == NULL) {
            return FALSE;
        }
        heapPtr->capacity = newCapacity;
        long i;
        void** elements = heapPtr->elements;
        for (i = 0; i <= size; i++) {
            newElements[i] = elements[i];
        }
        free(heapPtr->elements);
        heapPtr->elements = newElements;
    }

    size = ++(heapPtr->size);
    heapPtr->elements[size] = dataPtr;
    siftUp(heapPtr, size);

    return TRUE;
}


/* =============================================================================
 * HTMheap_insert
 * -- Returns FALSE on failure
 * =============================================================================
 */
bool_t
HTMheap_insert (heap_t* heapPtr, void* dataPtr)
{
    bool_t rv;

    long size = (long)HTM_SHARED_READ(heapPtr->size);
    long capacity = (long)HTM_SHARED_READ(heapPtr->capacity);

    if ((size + 1) >= capacity) {
        long newCapacity = capacity * 2;
        void** newElements = (void**)HTM_MALLOC(newCapacity * sizeof(void*));
        if (newElements == NULL) {
            rv = FALSE;
            goto out;
        }
        HTM_SHARED_WRITE(heapPtr->capacity, newCapacity);
        long i;
        void** elements = HTM_SHARED_READ_P(heapPtr->elements);
        for (i = 0; i <= size; i++) {
            newElements[i] = (void*)HTM_SHARED_READ_P(elements[i]);
        }
        TM_FREE(heapPtr->elements);
        TM_SHARED_WRITE(heapPtr->elements, newElements);
    }

    size++;
    HTM_SHARED_WRITE(heapPtr->size, size);
    void** elements = (void**)HTM_SHARED_READ_P(heapPtr->elements);
    HTM_SHARED_WRITE_P(elements[size], dataPtr);
    HTMsiftUp(TM_ARG  heapPtr, size);

    rv = TRUE;
out:
    return rv;
}


/* =============================================================================
 * TMheap_insert
 * -- Returns FALSE on failure
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMheap_insert (TM_ARGDECL  heap_t* heapPtr, void* dataPtr)
{
# if !defined(ORIGINAL) && defined(MERGE_HEAP)
    __label__ replay;
# endif /* !ORIGINAL && MERGE_HEAP */

    bool_t rv;
#ifndef ORIGINAL

# ifdef MERGE_HEAP
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Conflict occurred directly inside HEAP_INSERT */
        ASSERT(params->leaf == 1);
        ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->current), HEAP_INSERT));

# ifdef TM_DEBUG
        printf("HEAP_INSERT_JIT heapPtr:%p dataPtr:%p rv:%p\n", heapPtr, dataPtr, params->rv.ptr);
# endif

        TM_FINISH_MERGE();
        goto replay;
    }
# endif /* MERGE_HEAP */

    TM_LOG_BEGIN(HEAP_INSERT,
# ifdef MERGE_HEAP
        merge
# else
        NULL
# endif /* MERGE_HEAP */
        , heapPtr, dataPtr);
#endif /* ORIGINAL */
    long size = (long)TM_SHARED_READ(heapPtr->size);
    long capacity = (long)TM_SHARED_READ(heapPtr->capacity);

    if ((size + 1) >= capacity) {
        long newCapacity = capacity * 2;
        void** newElements = (void**)TM_MALLOC(newCapacity * sizeof(void*));
        if (newElements == NULL) {
            rv = FALSE;
            goto out;
        }
        TM_SHARED_WRITE(heapPtr->capacity, newCapacity);
        long i;
        void** elements = TM_SHARED_READ_P(heapPtr->elements);
        for (i = 0; i <= size; i++) {
            newElements[i] = (void*)TM_SHARED_READ_P(elements[i]);
        }
        TM_FREE(heapPtr->elements);
        TM_SHARED_WRITE(heapPtr->elements, newElements);
    }

    size++;
    TM_SHARED_WRITE(heapPtr->size, size);
    void** elements = (void**)TM_SHARED_READ_P(heapPtr->elements);
    TM_SHARED_WRITE_P(elements[size], dataPtr);
    TMsiftUp(TM_ARG  heapPtr, size);

    rv = TRUE;
out:
#ifndef ORIGINAL
    TM_LOG_END(HEAP_INSERT, &rv);
replay:
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * heapify
 * =============================================================================
 */
static void
heapify (heap_t* heapPtr, long startIndex)
{
    void** elements = heapPtr->elements;
    long (*compare)(const void*, const void*) = heapPtr->compare;

    long size = heapPtr->size;
    long index = startIndex;

    while (1) {

        long leftIndex = LEFT_CHILD(index);
        long rightIndex = RIGHT_CHILD(index);
        long maxIndex = -1;

        if ((leftIndex <= size) &&
            (compare(elements[leftIndex], elements[index]) > 0))
        {
            maxIndex = leftIndex;
        } else {
            maxIndex = index;
        }

        if ((rightIndex <= size) &&
            (compare(elements[rightIndex], elements[maxIndex]) > 0))
        {
            maxIndex = rightIndex;
        }

        if (maxIndex == index) {
            break;
        } else {
            void* tmpPtr = elements[index];
            elements[index] = elements[maxIndex];
            elements[maxIndex] = tmpPtr;
            index = maxIndex;
        }
    }
}


/* =============================================================================
 * HTMheapify
 * =============================================================================
 */
static void
HTMheapify (heap_t* heapPtr, long startIndex)
{
    void** elements = (void**)HTM_SHARED_READ_P(heapPtr->elements);
    long (*compare)(const void*, const void*) = heapPtr->compare;

    long size = (long)HTM_SHARED_READ(heapPtr->size);
    long index = startIndex;

    while (1) {

        long leftIndex = LEFT_CHILD(index);
        long rightIndex = RIGHT_CHILD(index);
        long maxIndex = -1;

        if ((leftIndex <= size) &&
            (compare((void*)HTM_SHARED_READ_P(elements[leftIndex]),
                     (void*)HTM_SHARED_READ_P(elements[index])) > 0))
        {
            maxIndex = leftIndex;
        } else {
            maxIndex = index;
        }

        if ((rightIndex <= size) &&
            (compare((void*)HTM_SHARED_READ_P(elements[rightIndex]),
                     (void*)HTM_SHARED_READ_P(elements[maxIndex])) > 0))
        {
            maxIndex = rightIndex;
        }

        if (maxIndex == index) {
            break;
        } else {
            void* tmpPtr = (void*)HTM_SHARED_READ_P(elements[index]);
            HTM_SHARED_WRITE_P(elements[index],
                              (void*)HTM_SHARED_READ(elements[maxIndex]));
            HTM_SHARED_WRITE_P(elements[maxIndex], tmpPtr);
            index = maxIndex;
        }
    }
}


/* =============================================================================
 * TMheapify
 * =============================================================================
 */
TM_CALLABLE
static void
TMheapify (TM_ARGDECL  heap_t* heapPtr, long startIndex)
{
    void** elements = (void**)TM_SHARED_READ_P(heapPtr->elements);
    TM_PURE long (*compare)(const void*, const void*) = heapPtr->compare;

    long size = (long)TM_SHARED_READ(heapPtr->size);
    long index = startIndex;

    while (1) {

        long leftIndex = LEFT_CHILD(index);
        long rightIndex = RIGHT_CHILD(index);
        long maxIndex = -1;

        if ((leftIndex <= size) &&
            (compare((void*)TM_SHARED_READ_P(elements[leftIndex]),
                     (void*)TM_SHARED_READ_P(elements[index])) > 0))
        {
            maxIndex = leftIndex;
        } else {
            maxIndex = index;
        }

        if ((rightIndex <= size) &&
            (compare((void*)TM_SHARED_READ_P(elements[rightIndex]),
                     (void*)TM_SHARED_READ_P(elements[maxIndex])) > 0))
        {
            maxIndex = rightIndex;
        }

        if (maxIndex == index) {
            break;
        } else {
            void* tmpPtr = (void*)TM_SHARED_READ_P(elements[index]);
            TM_SHARED_WRITE_P(elements[index],
                              (void*)TM_SHARED_READ(elements[maxIndex]));
            TM_SHARED_WRITE_P(elements[maxIndex], tmpPtr);
            index = maxIndex;
        }
    }
}



/* =============================================================================
 * heap_remove
 * -- Returns NULL if empty
 * =============================================================================
 */
void*
heap_remove (heap_t* heapPtr)
{
    long size = heapPtr->size;

    if (size < 1) {
        return NULL;
    }

    void** elements = heapPtr->elements;
    void* dataPtr = elements[1];
    elements[1] = elements[size];
    heapPtr->size = size - 1;
    heapify(heapPtr, 1);

    return dataPtr;
}


/* =============================================================================
 * HTMheap_remove
 * -- Returns NULL if empty
 * =============================================================================
 */
void*
HTMheap_remove (heap_t* heapPtr)
{
    long size = (long)HTM_SHARED_READ(heapPtr->size);

    if (size < 1) {
        return NULL;
    }

    void** elements = (void**)HTM_SHARED_READ_P(heapPtr->elements);
    void* dataPtr = (void*)HTM_SHARED_READ_P(elements[1]);
    HTM_SHARED_WRITE_P(elements[1], HTM_SHARED_READ_P(elements[size]));
    HTM_SHARED_WRITE(heapPtr->size, (size - 1));
    HTMheapify(TM_ARG  heapPtr, 1);

    return dataPtr;
}


/* =============================================================================
 * TMheap_remove
 * -- Returns NULL if empty
 * =============================================================================
 */
TM_CALLABLE
void*
TMheap_remove (TM_ARGDECL  heap_t* heapPtr)
{
    long size = (long)TM_SHARED_READ(heapPtr->size);

    if (size < 1) {
        return NULL;
    }

    void** elements = (void**)TM_SHARED_READ_P(heapPtr->elements);
    void* dataPtr = (void*)TM_SHARED_READ_P(elements[1]);
    TM_SHARED_WRITE_P(elements[1], TM_SHARED_READ_P(elements[size]));
    TM_SHARED_WRITE(heapPtr->size, (size - 1));
    TMheapify(TM_ARG  heapPtr, 1);

    return dataPtr;
}


/* =============================================================================
 * heap_isValid
 * =============================================================================
 */
bool_t
heap_isValid (heap_t* heapPtr)
{
    long size = heapPtr->size;
    long (*compare)(const void*, const void*) = heapPtr->compare;
    void** elements = heapPtr->elements;

    long i;
    for (i = 1; i < size; i++) {
        if (compare(elements[i+1], elements[PARENT(i+1)]) > 0) {
            return FALSE;
        }
    }

    return TRUE;
}


/* =============================================================================
 * TEST_HEAP
 * =============================================================================
 */
#ifdef TEST_HEAP


#include <assert.h>
#include <stdio.h>


static long
compare (const void* a, const void* b)
{
    return (*((const long*)a) - *((const long*)b));
}


static void
printHeap (heap_t* heapPtr)
{
    printf("[");

    long i;
    for (i = 0; i < heapPtr->size; i++) {
        printf("%li ", *(long*)heapPtr->elements[i+1]);
    }

    puts("]");
}


static void
insertInt (heap_t* heapPtr, long* data)
{
    printf("Inserting: %li\n", *data);
    assert(heap_insert(heapPtr, (void*)data));
    printHeap(heapPtr);
    assert(heap_isValid(heapPtr));
}


static void
removeInt (heap_t* heapPtr)
{
    long* data = heap_remove(heapPtr);
    printf("Removing: %li\n", *data);
    printHeap(heapPtr);
    assert(heap_isValid(heapPtr));
}



long global_data[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9};
long global_numData = sizeof(global_data) / sizeof(global_data[0]);

int
main ()
{
    puts("Starting...");

    heap_t* heapPtr = heap_alloc(1, compare);

    assert(heapPtr);

    long i;
    for (i = 0; i < global_numData; i++) {
        insertInt(heapPtr, &global_data[i]);
    }

    for (i = 0; i < global_numData; i++) {
        removeInt(heapPtr);
    }

    assert(heap_remove(heapPtr) == NULL); /* empty */

    heap_free(heapPtr);

    puts("Passed all tests.");

    return 0;
}


#endif /* TEST_HEAP */


/* =============================================================================
 *
 * End of heap.c
 *
 * =============================================================================
 */


#ifndef ORIGINAL
# ifdef MERGE_HEAP
stm_merge_t TMheap_merge(stm_merge_context_t *params) {
    const stm_op_id_t op = stm_get_op_opcode(params->current);

    /* Delayed merge only */
    ASSERT(!STM_SAME_OP(stm_get_current_op(), params->current));

    const stm_union_t *args;
    const ssize_t nargs = stm_get_op_args(params->current, &args);

    if (STM_SAME_OPID(op, HEAP_INSERT)) {
        ASSERT(nargs == 2);
        heap_t* heapPtr = args[0].ptr;
        ASSERT(heapPtr);
        void* dataPtr = args[1].ptr;
        ASSERT(dataPtr);

        ASSERT(params->leaf == 1);
        ASSERT(ENTRY_VALID(params->conflict.entries->e1));
        stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
        ASSERT(STM_VALID_READ(r));

        /* Conflict is at the heap size */
        if (params->addr == &heapPtr->size) {
            long old, new;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, heapPtr->size, old));
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, heapPtr->size, new));
# ifdef TM_DEBUG
            printf("HEAP_INSERT addr:%p heapPtr:%p dataPtr:%p heapPtr->size read (old):%ld (new):%ld, write (new):%ld\n", params->addr, heapPtr, dataPtr, old, new, new + 1);
# endif

            long oldCapacity, newCapacity;
            /* Update the heap capacity */
            r = TM_SHARED_DID_READ(heapPtr->capacity);
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, heapPtr->capacity, oldCapacity));
            ASSERT(oldCapacity > 0);
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, heapPtr->capacity, newCapacity));
            ASSERT(newCapacity > 0);

            if (oldCapacity != newCapacity || new + 1 >= newCapacity)
                return STM_MERGE_ABORT;

            /* Revert and reinsert the element in the heap */
            void **elements = TM_SHARED_READ_P(heapPtr->elements);
            stm_write_t w = TM_SHARED_DID_WRITE(elements[old + 1]);
            ASSERT_FAIL(STM_VALID_WRITE(w));
            ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
            TM_SHARED_WRITE(elements[new + 1], dataPtr);

            /* Increment the heap size */
            w = TM_SHARED_DID_WRITE(heapPtr->size);
            ASSERT_FAIL(STM_VALID_WRITE(w));
            ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, heapPtr->size, new + 1));

            ASSERT(params->rv.sint == TRUE);
            /* Revert and redo the heap sift */
            params->read = stm_get_load_next(stm_get_load_last(r), 0, 0);
            ASSERT_FAIL(stm_undo_op_descendants(params->current, HEAP_SIFTUP));
            TMsiftUp(TM_ARG  heapPtr, new + 1);

            return STM_MERGE_OK;
        }
    }

# ifdef TM_DEBUG
    printf("\nHEAP_MERGE UNSUPPORTED addr:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}
# define TMHEAP_MERGE TMheap_merge
#else
# define TMHEAP_MERGE NULL
#endif /* MERGE_HEAP */

__attribute__((constructor)) void heap_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*pl[], {&ffi_type_pointer, &ffi_type_slong});
    TM_LOG_TYPE_DECLARE_INIT(*pp[], {&ffi_type_pointer, &ffi_type_pointer});
    # define TM_LOG_OP TM_LOG_OP_INIT
    # include "heap.inc"
    # undef TM_LOG_OP
}
#endif /* ORIGINAL */
