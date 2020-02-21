/* =============================================================================
 *
 * queue.c
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
#include <string.h>
#include "random.h"
#include "tm.h"
#include "types.h"
#include "queue.h"


struct queue {
    long pop; /* points before element to pop */
    long push;
    long capacity;
    void** elements;
};

enum config {
    QUEUE_GROWTH_FACTOR = 2,
};

#ifndef ORIGINAL
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "queue.inc"
# undef TM_LOG_OP
#endif /* ORIGINAL */

/* =============================================================================
 * queue_alloc
 * =============================================================================
 */
queue_t*
queue_alloc (long initCapacity)
{
    queue_t* queuePtr = (queue_t*)malloc(sizeof(queue_t));

    if (queuePtr) {
        long capacity = ((initCapacity < 2) ? 2 : initCapacity);
        queuePtr->elements = (void**)malloc(capacity * sizeof(void*));
        if (queuePtr->elements == NULL) {
            free(queuePtr);
            return NULL;
        }
        queuePtr->pop      = capacity - 1;
        queuePtr->push     = 0;
        queuePtr->capacity = capacity;
    }

    return queuePtr;
}


/* =============================================================================
 * Pqueue_alloc
 * =============================================================================
 */
queue_t*
Pqueue_alloc (long initCapacity)
{
    queue_t* queuePtr = (queue_t*)P_MALLOC(sizeof(queue_t));

    if (queuePtr) {
        long capacity = ((initCapacity < 2) ? 2 : initCapacity);
        queuePtr->elements = (void**)P_MALLOC(capacity * sizeof(void*));
        if (queuePtr->elements == NULL) {
            free(queuePtr);
            return NULL;
        }
        queuePtr->pop      = capacity - 1;
        queuePtr->push     = 0;
        queuePtr->capacity = capacity;
    }

    return queuePtr;
}


/* =============================================================================
 * TMqueue_alloc
 * =============================================================================
 */
queue_t*
TMqueue_alloc (TM_ARGDECL  long initCapacity)
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(QUEUE_ALLOC, NULL, initCapacity);
#endif /* ORIGINAL */

    queue_t* queuePtr = (queue_t*)TM_MALLOC(sizeof(queue_t));

    if (queuePtr) {
        long capacity = ((initCapacity < 2) ? 2 : initCapacity);
        queuePtr->elements = (void**)TM_MALLOC(capacity * sizeof(void*));
        if (queuePtr->elements == NULL) {
            free(queuePtr);
            queuePtr = NULL;
            goto out;
        }
        queuePtr->pop      = capacity - 1;
        queuePtr->push     = 0;
        queuePtr->capacity = capacity;
    }

out:
#ifndef ORIGINAL
    TM_LOG_END(QUEUE_ALLOC, &queuePtr);
#endif /* ORIGINAL */
    return queuePtr;
}


/* =============================================================================
 * queue_free
 * =============================================================================
 */
void
queue_free (queue_t* queuePtr)
{
    free(queuePtr->elements);
    free(queuePtr);
}


/* =============================================================================
 * Pqueue_free
 * =============================================================================
 */
void
Pqueue_free (queue_t* queuePtr)
{
    P_FREE(queuePtr->elements);
    P_FREE(queuePtr);
}


/* =============================================================================
 * TMqueue_free
 * =============================================================================
 */
void
TMqueue_free (TM_ARGDECL  queue_t* queuePtr)
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(QUEUE_FREE, NULL, queuePtr);
#endif /* ORIGINAL */
    TM_FREE((void**)TM_SHARED_READ_P(queuePtr->elements));
    TM_FREE(queuePtr);
#ifndef ORIGINAL
    TM_LOG_END(QUEUE_FREE, NULL);
#endif /* ORIGINAL */
}


/* =============================================================================
 * queue_isEmpty
 * =============================================================================
 */
bool_t
queue_isEmpty (queue_t* queuePtr)
{
    long pop      = queuePtr->pop;
    long push     = queuePtr->push;
    long capacity = queuePtr->capacity;

    return (((pop + 1) % capacity == push) ? TRUE : FALSE);
}


/* =============================================================================
 * HTMqueue_isEmpty
 * =============================================================================
 */
bool_t
HTMqueue_isEmpty (queue_t* queuePtr)
{
    long pop      = (long)HTM_SHARED_READ(queuePtr->pop);
    long push     = (long)HTM_SHARED_READ(queuePtr->push);
    long capacity = (long)HTM_SHARED_READ(queuePtr->capacity);

    return ((pop + 1) % capacity == push) ? TRUE : FALSE;
}


/* =============================================================================
 * TMqueue_isEmpty
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMqueue_isEmpty (TM_ARGDECL  queue_t* queuePtr)
{
    bool_t rv;

#ifndef ORIGINAL
    TM_LOG_BEGIN(QUEUE_ISEMPTY, NULL, queuePtr);
#endif /* ORIGINAL */
    long pop      = (long)TM_SHARED_READ(queuePtr->pop);
    long push     = (long)TM_SHARED_READ(queuePtr->push);
    long capacity = (long)TM_SHARED_READ(queuePtr->capacity);

    rv = ((pop + 1) % capacity == push) ? TRUE : FALSE;
#ifndef ORIGINAL
    TM_LOG_END(QUEUE_ISEMPTY, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * queue_clear
 * =============================================================================
 */
void
queue_clear (queue_t* queuePtr)
{
    queuePtr->pop  = queuePtr->capacity - 1;
    queuePtr->push = 0;
}


/* =============================================================================
 * queue_shuffle
 * =============================================================================
 */
void
queue_shuffle (queue_t* queuePtr, random_t* randomPtr)
{
    long pop      = queuePtr->pop;
    long push     = queuePtr->push;
    long capacity = queuePtr->capacity;

    long numElement;
    if (pop < push) {
        numElement = push - (pop + 1);
    } else {
        numElement = capacity - (pop - push + 1);
    }

    void** elements = queuePtr->elements;
    long i;
    long base = pop + 1;
    for (i = 0; i < numElement; i++) {
        long r1 = random_generate(randomPtr) % numElement;
        long r2 = random_generate(randomPtr) % numElement;
        long i1 = (base + r1) % capacity;
        long i2 = (base + r2) % capacity;
        void* tmp = elements[i1];
        elements[i1] = elements[i2];
        elements[i2] = tmp;
    }
}


/* =============================================================================
 * queue_push
 * =============================================================================
 */
bool_t
queue_push (queue_t* queuePtr, void* dataPtr)
{
    long pop      = queuePtr->pop;
    long push     = queuePtr->push;
    long capacity = queuePtr->capacity;

    assert(pop != push);

    /* Need to resize */
    long newPush = (push + 1) % capacity;
    if (newPush == pop) {

        long newCapacity = capacity * QUEUE_GROWTH_FACTOR;
        void** newElements = (void**)malloc(newCapacity * sizeof(void*));
        if (newElements == NULL) {
            return FALSE;
        }

        long dst = 0;
        void** elements = queuePtr->elements;
        if (pop < push) {
            long src;
            for (src = (pop + 1); src < push; src++, dst++) {
                newElements[dst] = elements[src];
            }
        } else {
            long src;
            for (src = (pop + 1); src < capacity; src++, dst++) {
                newElements[dst] = elements[src];
            }
            for (src = 0; src < push; src++, dst++) {
                newElements[dst] = elements[src];
            }
        }

        free(elements);
        queuePtr->elements = newElements;
        queuePtr->pop      = newCapacity - 1;
        queuePtr->capacity = newCapacity;
        push = dst;
        newPush = push + 1; /* no need modulo */
    }

    queuePtr->elements[push] = dataPtr;
    queuePtr->push = newPush;

    return TRUE;
}


/* =============================================================================
 * Pqueue_push
 * =============================================================================
 */
bool_t
Pqueue_push (queue_t* queuePtr, void* dataPtr)
{
    long pop      = queuePtr->pop;
    long push     = queuePtr->push;
    long capacity = queuePtr->capacity;

    assert(pop != push);

    /* Need to resize */
    long newPush = (push + 1) % capacity;
    if (newPush == pop) {

        long newCapacity = capacity * QUEUE_GROWTH_FACTOR;
        void** newElements = (void**)P_MALLOC(newCapacity * sizeof(void*));
        if (newElements == NULL) {
            return FALSE;
        }

        long dst = 0;
        void** elements = queuePtr->elements;
        if (pop < push) {
            long src;
            for (src = (pop + 1); src < push; src++, dst++) {
                newElements[dst] = elements[src];
            }
        } else {
            long src;
            for (src = (pop + 1); src < capacity; src++, dst++) {
                newElements[dst] = elements[src];
            }
            for (src = 0; src < push; src++, dst++) {
                newElements[dst] = elements[src];
            }
        }

        P_FREE(elements);
        queuePtr->elements = newElements;
        queuePtr->pop      = newCapacity - 1;
        queuePtr->capacity = newCapacity;
        push = dst;
        newPush = push + 1; /* no need modulo */

    }

    queuePtr->elements[push] = dataPtr;
    queuePtr->push = newPush;

    return TRUE;
}


/* =============================================================================
 * HTMqueue_push
 * =============================================================================
 */
bool_t
HTMqueue_push (queue_t* queuePtr, void* dataPtr)
{
    bool_t rv;

    long pop;
    long push = -1;
    long capacity = -1;
    long newPush;
    void** elements = NULL;

    pop      = (long)HTM_SHARED_READ(queuePtr->pop);
    push     = (long)HTM_SHARED_READ(queuePtr->push);
    capacity = (long)HTM_SHARED_READ(queuePtr->capacity);

    assert(pop != push);

    /* Need to resize */
    newPush = (push + 1) % capacity;
    if (newPush == pop) {
        long newCapacity = capacity * QUEUE_GROWTH_FACTOR;
        void** newElements = (void**)HTM_MALLOC(newCapacity * sizeof(void*));
        if (newElements == NULL) {
            rv = FALSE;
            goto out;
        }

        long dst = 0;
        void** elements = (void**)HTM_SHARED_READ_P(queuePtr->elements);
        if (pop < push) {
            long src;
            for (src = (pop + 1); src < push; src++, dst++) {
                newElements[dst] = (void*)HTM_SHARED_READ_P(elements[src]);
            }
        } else {
            long src;
            for (src = (pop + 1); src < capacity; src++, dst++) {
                newElements[dst] = (void*)HTM_SHARED_READ_P(elements[src]);
            }
            for (src = 0; src < push; src++, dst++) {
                newElements[dst] = (void*)HTM_SHARED_READ_P(elements[src]);
            }
        }

        HTM_FREE(elements);
        HTM_SHARED_WRITE_P(queuePtr->elements, newElements);
        HTM_SHARED_WRITE(queuePtr->pop,      newCapacity - 1);
        HTM_SHARED_WRITE(queuePtr->capacity, newCapacity);
        push = dst;
        newPush = push + 1; /* no need modulo */

    }

    elements = (void**)HTM_SHARED_READ_P(queuePtr->elements);
    HTM_SHARED_WRITE_P(elements[push], dataPtr);
    HTM_SHARED_WRITE(queuePtr->push, newPush);

    rv = TRUE;
out:
    return rv;
}


/* =============================================================================
 * TMqueue_push
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMqueue_push (TM_ARGDECL  queue_t* queuePtr, void* dataPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_QUEUE)
    __label__ replay;
#endif /* !ORIGINAL && MERGE_QUEUE */

    bool_t rv;

    long pop;
    long push = -1;
    long capacity = -1;
    long newPush;
    void** elements = NULL;

#ifndef ORIGINAL
# ifdef MERGE_QUEUE
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Conflict occurred directly inside QUEUE_PUSH */
        if (STM_SAME_OPID(stm_get_op_opcode(params->current), QUEUE_PUSH)) {
            ASSERT(params->leaf == 1);

# ifdef TM_DEBUG
            printf("\nQUEUE_PUSH_JIT addr:%p queuePtr:%p\n", params->addr, queuePtr);
# endif

            rv = params->rv.sint;
            TM_FINISH_MERGE();
            goto replay;
        }

# ifdef TM_DEBUG
        printf("\nQUEUE_PUSH_JIT UNSUPPORTED addr:%p\n", params->addr);
# endif
        return STM_MERGE_UNSUPPORTED;
    }
# endif /* MERGE_QUEUE */
    TM_LOG_BEGIN(QUEUE_PUSH,
# ifdef MERGE_QUEUE
        merge
# else
        NULL
# endif /* MERGE_QUEUE */
        , queuePtr, dataPtr);
#endif /* ORIGINAL */

    pop      = (long)TM_SHARED_READ(queuePtr->pop);
    push     = (long)TM_SHARED_READ(queuePtr->push);
    capacity = (long)TM_SHARED_READ(queuePtr->capacity);

    assert(pop != push);

    /* Need to resize */
    newPush = (push + 1) % capacity;
    if (newPush == pop) {
        long newCapacity = capacity * QUEUE_GROWTH_FACTOR;
        void** newElements = (void**)TM_MALLOC(newCapacity * sizeof(void*));
        if (newElements == NULL) {
            rv = FALSE;
            goto out;
        }

        long dst = 0;
        void** elements = (void**)TM_SHARED_READ_P(queuePtr->elements);
        if (pop < push) {
            long src;
            for (src = (pop + 1); src < push; src++, dst++) {
                newElements[dst] = (void*)TM_SHARED_READ_P(elements[src]);
            }
        } else {
            long src;
            for (src = (pop + 1); src < capacity; src++, dst++) {
                newElements[dst] = (void*)TM_SHARED_READ_P(elements[src]);
            }
            for (src = 0; src < push; src++, dst++) {
                newElements[dst] = (void*)TM_SHARED_READ_P(elements[src]);
            }
        }

        TM_FREE(elements);
        TM_SHARED_WRITE_P(queuePtr->elements, newElements);
        TM_SHARED_WRITE(queuePtr->pop,      newCapacity - 1);
        TM_SHARED_WRITE(queuePtr->capacity, newCapacity);
        push = dst;
        newPush = push + 1; /* no need modulo */

    }

    elements = (void**)TM_SHARED_READ_P(queuePtr->elements);
    TM_SHARED_WRITE_P(elements[push], dataPtr);
    TM_SHARED_WRITE(queuePtr->push, newPush);

    rv = TRUE;
out:
#ifndef ORIGINAL
    TM_LOG_END(QUEUE_PUSH, &rv);
replay:
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * queue_pop
 * =============================================================================
 */
void*
queue_pop (queue_t* queuePtr)
{
    long pop      = queuePtr->pop;
    long push     = queuePtr->push;
    long capacity = queuePtr->capacity;

    long newPop = (pop + 1) % capacity;
    if (newPop == push) {
        return NULL;
    }

    void* dataPtr = queuePtr->elements[newPop];
    queuePtr->pop = newPop;

    return dataPtr;
}


/* =============================================================================
 * HTMqueue_pop
 * =============================================================================
 */
void*
HTMqueue_pop (queue_t* queuePtr)
{
    void *dataPtr = NULL;
    long pop;
    long push = -1;
    long capacity = -1;
    long newPop;
    void **elements = NULL;

    pop      = (long)HTM_SHARED_READ(queuePtr->pop);
    push     = (long)HTM_SHARED_READ(queuePtr->push);
    capacity = (long)HTM_SHARED_READ(queuePtr->capacity);

    newPop = (pop + 1) % capacity;
    if (newPop == push)
        goto out;

    elements = (void**)HTM_SHARED_READ_P(queuePtr->elements);
    dataPtr = (void*)HTM_SHARED_READ_P(elements[newPop]);
    HTM_SHARED_WRITE(queuePtr->pop, newPop);

out:
    return dataPtr;
}


/* =============================================================================
 * TMqueue_pop
 * =============================================================================
 */
TM_CALLABLE
void*
TMqueue_pop (TM_ARGDECL  queue_t* queuePtr)
{
#if !defined(ORIGINAL) && defined(MERGE_QUEUE)
    __label__ replay;
#endif /* !ORIGINAL && MERGE_QUEUE */

    void *dataPtr = NULL;
    long pop;
    long push = -1;
    long capacity = -1;
    long newPop;
    void **elements = NULL;

#ifndef ORIGINAL
# ifdef MERGE_QUEUE
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Conflict occurred directly inside QUEUE_POP */
        if (STM_SAME_OPID(stm_get_op_opcode(params->current), QUEUE_POP)) {
            ASSERT(params->leaf == 1);

# ifdef TM_DEBUG
            printf("\nQUEUE_POP_JIT addr:%p queuePtr:%p\n", params->addr, queuePtr);
# endif

            dataPtr = params->rv.ptr;
            TM_FINISH_MERGE();
            goto replay;
        }

# ifdef TM_DEBUG
        printf("\nQUEUE_POP_JIT UNSUPPORTED addr:%p\n", params->addr);
# endif
        return STM_MERGE_UNSUPPORTED;

    }
# endif /* MERGE_QUEUE */
    TM_LOG_BEGIN(QUEUE_POP,
# ifdef MERGE_QUEUE
        merge
# else
        NULL
# endif /* MERGE_QUEUE */
        , queuePtr);
#endif /* ORIGINAL */
    pop      = (long)TM_SHARED_READ(queuePtr->pop);
    push     = (long)TM_SHARED_READ(queuePtr->push);
    capacity = (long)TM_SHARED_READ(queuePtr->capacity);

    newPop = (pop + 1) % capacity;
    if (newPop == push)
        goto out;

    elements = (void**)TM_SHARED_READ_P(queuePtr->elements);
    dataPtr = (void*)TM_SHARED_READ_P(elements[newPop]);
    TM_SHARED_WRITE(queuePtr->pop, newPop);

out:
#ifndef ORIGINAL
    TM_LOG_END(QUEUE_POP, &dataPtr);
replay:
#endif /* ORIGINAL */
    return dataPtr;
}


/* =============================================================================
 * TEST_QUEUE
 * =============================================================================
 */
#ifdef TEST_QUEUE

#include <assert.h>
#include <stdio.h>


static void
printQueue (queue_t* queuePtr)
{
    long   push     = queuePtr->push;
    long   pop      = queuePtr->pop;
    long   capacity = queuePtr->capacity;
    void** elements = queuePtr->elements;

    printf("[");

    long i;
    for (i = ((pop + 1) % capacity); i != push; i = ((i + 1) % capacity)) {
        printf("%li ", *(long*)elements[i]);
    }
    puts("]");
}


static void
insertData (queue_t* queuePtr, long* dataPtr)
{
    printf("Inserting %li: ", *dataPtr);
    assert(queue_push(queuePtr, dataPtr));
    printQueue(queuePtr);
}


int
main ()
{
    queue_t* queuePtr;
    random_t* randomPtr;
    long data[] = {3, 1, 4, 1, 5};
    long numData = sizeof(data) / sizeof(data[0]);
    long i;

    randomPtr = random_alloc();
    assert(randomPtr);
    random_seed(randomPtr, 0);

    puts("Starting tests...");

    queuePtr = queue_alloc(-1);

    assert(queue_isEmpty(queuePtr));
    for (i = 0; i < numData; i++) {
        insertData(queuePtr, &data[i]);
    }
    assert(!queue_isEmpty(queuePtr));

    for (i = 0; i < numData; i++) {
        long* dataPtr = (long*)queue_pop(queuePtr);
        printf("Removing %li: ", *dataPtr);
        printQueue(queuePtr);
    }
    assert(!queue_pop(queuePtr));
    assert(queue_isEmpty(queuePtr));

    puts("All tests passed.");

    for (i = 0; i < numData; i++) {
        insertData(queuePtr, &data[i]);
    }
    for (i = 0; i < numData; i++) {
        printf("Shuffle %li: ", i);
        queue_shuffle(queuePtr, randomPtr);
        printQueue(queuePtr);
    }
    assert(!queue_isEmpty(queuePtr));

    queue_free(queuePtr);

    return 0;
}


#endif /* TEST_QUEUE */


/* =============================================================================
 *
 * End of queue.c
 *
 * =============================================================================
 */

#ifndef ORIGINAL
# ifdef MERGE_QUEUE
stm_merge_t TMqueue_merge(stm_merge_context_t *params) {
    const stm_op_id_t op = stm_get_op_opcode(params->current);

    /* Delayed merge only */
    ASSERT(!STM_SAME_OP(stm_get_current_op(), params->current));

    stm_read_t r;
    const stm_union_t *args;
    const ssize_t nargs = stm_get_op_args(params->current, &args);

    if (STM_SAME_OPID(op, QUEUE_POP)) {
        ASSERT(nargs == 1);
        const queue_t *queuePtr = args[0].ptr;
        ASSERT(queuePtr);
        ASSERT(!STM_VALID_OP(params->previous));
        ASSERT(params->leaf == 1);

        r = ENTRY_GET_READ(params->conflict.entries->e1);
        ASSERT(STM_VALID_READ(r));

# ifdef TM_DEBUG
        printf("\nQUEUE_POP addr:%p queuePtr:%p\n", params->addr, queuePtr);
# endif
        /* Conflict occurred directly inside QUEUE_POP */
        ASSERT(params->leaf == 1);
        ASSERT(params->conflict.entries->type == STM_RD_VALIDATE);
        ASSERT(ENTRY_VALID(params->conflict.entries->e1));

        /* Update the queue head */
        if (params->addr == &queuePtr->pop)
            r = ENTRY_GET_READ(params->conflict.entries->e1);
        else
            r = TM_SHARED_DID_READ(queuePtr->pop);
        ASSERT(STM_VALID_READ(r));
        long oldPop, newPop;
        ASSERT_FAIL(TM_SHARED_READ_VALUE(r, queuePtr->pop, oldPop));
        ASSERT(oldPop >= 0);
        ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, queuePtr->pop, newPop));
        ASSERT(newPop >= 0);
# ifdef TM_DEBUG
        printf("QUEUE_POP queuePtr->pop:%p (old):%ld (new):%ld\n", &queuePtr->pop, oldPop, newPop);
# endif
        const stm_write_t w = TM_SHARED_DID_WRITE(queuePtr->pop);

        /* Update the queue tail */
        if (params->addr == &queuePtr->push)
            r = ENTRY_GET_READ(params->conflict.entries->e1);
        else
            r = TM_SHARED_DID_READ(queuePtr->push);
        ASSERT(STM_VALID_READ(r));

        long oldPush, newPush;
        ASSERT_FAIL(TM_SHARED_READ_VALUE(r, queuePtr->push, oldPush));
        ASSERT(oldPush >= 0);
        ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, queuePtr->push, newPush));
        ASSERT(newPush >= 0);
# ifdef TM_DEBUG
        printf("QUEUE_POP queuePtr->push:%p (old):%ld (new):%ld\n", &queuePtr->push, oldPush, newPush);
# endif

        if (oldPop == newPop && oldPush == newPush && (params->addr == &queuePtr->pop || params->addr == &queuePtr->push))
            return STM_MERGE_OK;

        /* Update the queue capacity */
        if (params->addr == &queuePtr->capacity)
            r = ENTRY_GET_READ(params->conflict.entries->e1);
        else
            r = TM_SHARED_DID_READ(queuePtr->capacity);

        long oldCapacity, newCapacity;
        /* Update the queue capacity */
        ASSERT_FAIL(TM_SHARED_READ_VALUE(r, queuePtr->capacity, oldCapacity));
        ASSERT(oldCapacity > 0);
        ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, queuePtr->capacity, newCapacity));
        ASSERT(newCapacity > 0);
# ifdef TM_DEBUG
        printf("QUEUE_POP queuePtr->capacity:%p (old):%ld (new):%ld\n", &queuePtr->capacity, oldCapacity, newCapacity);
# endif

        /* FIXME: Capacity change unsupported */
        if (oldCapacity != newCapacity) {
            ASSERT(0);
# ifdef TM_DEBUG
            printf("\nQUEUE_POP UNSUPPORTED:%p\n", params->addr);
# endif
            return STM_MERGE_ABORT;
        }

        /* Update the queue pointer */
        if (params->addr == &queuePtr->elements)
            r = ENTRY_GET_READ(params->conflict.entries->e1);
        else
            r = TM_SHARED_DID_READ(queuePtr->elements);
        void **oldElements = NULL, **newElements = NULL;
        if (STM_VALID_READ(r)) {
            ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, queuePtr->elements, oldElements));
            ASSERT(oldElements);
            ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, queuePtr->elements, newElements));
            ASSERT(newElements);
# ifdef TM_DEBUG
            printf("QUEUE_POP queuePtr->elements:%p (old):%p (new):%p\n", &queuePtr->elements, oldElements, newElements);
# endif
            if (params->addr == &queuePtr->elements && oldElements == newElements)
                return STM_MERGE_OK;
        }

        /* Get the return value */
        void *old_rv = ((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) ? params->rv.ptr : (void *)(uintptr_t)(((oldPop + 1) % oldCapacity) != oldPush), *rv = old_rv;
        params->rv.ptr = old_rv;

        /* Check the queue head */
        const long capacity = (long)TM_SHARED_READ(queuePtr->capacity);
        const long oldPopW = (oldPop + 1) % capacity;
        ASSERT(oldPopW >= 0);
        const long newPopW = (newPop + 1) % capacity;
        ASSERT(newPopW >= 0);
        if (newPopW == newPush)
            rv = NULL;
# ifdef TM_DEBUG
        printf("QUEUE_POP queuePtr->pop write (old):%ld (new):%ld\n", oldPopW, newPopW);
# endif

        /* Check the return value */
        if (rv) {
            if (old_rv) {
# ifdef TM_DEBUG
                ASSERT(oldPopW != oldPush);
# endif
                /* Update the queue head */
                ASSERT(STM_VALID_WRITE(w));
                ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, queuePtr->pop, newPopW));

                ASSERT(oldElements);
                /* Get the element */
                if (params->addr == &oldElements[oldPopW])
                    r = ENTRY_GET_READ(params->conflict.entries->e1);
                else
                    r = TM_SHARED_DID_READ(oldElements[oldPopW]);
                ASSERT(STM_VALID_READ(r));
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, oldElements[oldPopW], old_rv));
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
            } else {
# ifdef TM_DEBUG
                ASSERT(oldPopW == oldPush);
# endif
                ASSERT(!STM_VALID_READ(r));
                ASSERT(!STM_VALID_WRITE(w));

                newElements = TM_SHARED_READ_P(queuePtr->elements);

                /* Update the queue head */
                TM_SHARED_WRITE(queuePtr->pop, newPopW);
            }

            ASSERT(newElements);
            /* Update the element */
            ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(newElements[newPopW])));
            rv = TM_SHARED_READ_P(newElements[newPopW]);
        } else {
            if (old_rv) {
                /* Update the queue head */
                ASSERT(STM_VALID_WRITE(w));
                ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));

                ASSERT(oldElements);
                /* Update the element */
                if (params->addr == &oldElements[oldPopW])
                    r = ENTRY_GET_READ(params->conflict.entries->e1);
                else
                    r = TM_SHARED_DID_READ(oldElements[oldPopW]);
                ASSERT(STM_VALID_READ(r));
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, oldElements[oldPopW], old_rv));
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));

                /* Update the element pointer */
                r = TM_SHARED_DID_READ(queuePtr->elements);
                ASSERT(STM_VALID_READ(r));
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
            } else {
                ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(queuePtr->elements)));
                ASSERT(!STM_VALID_WRITE(TM_SHARED_DID_WRITE(queuePtr->pop)));
            }
        }

# ifdef TM_DEBUG
        printf("QUEUE_POP rv(old):%p rv(new):%p\n", old_rv, rv);
# endif
        params->conflict.previous_result.ptr = old_rv;
        params->rv.ptr = rv;
        return STM_MERGE_OK;
    } else if (STM_SAME_OPID(op, QUEUE_PUSH)) {
        ASSERT(nargs == 2);
        const queue_t *queuePtr = args[0].ptr;
        ASSERT(queuePtr);
        const void *dataPtr = args[1].ptr;
        ASSERT(!STM_VALID_OP(params->previous));
        ASSERT(params->leaf == 1);

# ifdef TM_DEBUG
        printf("\nQUEUE_PUSH addr:%p queuePtr:%p dataPtr:%p\n", params->addr, queuePtr, dataPtr);
# endif

        /* Conflict occurred directly inside QUEUE_PUSH */
        ASSERT(params->leaf == 1);
        ASSERT(params->conflict.entries->type == STM_RD_VALIDATE);
        ASSERT(ENTRY_VALID(params->conflict.entries->e1));
        ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);

        /* FIXME: Return value change unsupported */
        if (params->rv.sint != TRUE) {
            ASSERT(0);
# ifdef TM_DEBUG
            printf("\nQUEUE_PUSH UNSUPPORTED:%p\n", params->addr);
# endif
            return STM_MERGE_ABORT;
        }

        /* Update the queue head */
        if (params->addr == &queuePtr->pop)
            r = ENTRY_GET_READ(params->conflict.entries->e1);
        else
            r = TM_SHARED_DID_READ(queuePtr->pop);
        ASSERT(STM_VALID_READ(r));
        long oldPop, newPop;
        ASSERT_FAIL(TM_SHARED_READ_VALUE(r, queuePtr->pop, oldPop));
        ASSERT(oldPop >= 0);
        ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, queuePtr->pop, newPop));
        ASSERT(newPop >= 0);
# ifdef TM_DEBUG
        printf("QUEUE_PUSH queuePtr->pop:%p (old):%ld (new):%ld\n", &queuePtr->pop, oldPop, newPop);
# endif

        /* Update the read of the queue tail */
        if (params->addr == &queuePtr->push)
            r = ENTRY_GET_READ(params->conflict.entries->e1);
        else
            r = TM_SHARED_DID_READ(queuePtr->push);
        ASSERT(STM_VALID_READ(r));

        long oldPush, newPush;
        ASSERT_FAIL(TM_SHARED_READ_VALUE(r, queuePtr->push, oldPush));
        ASSERT(oldPush >= 0);
        ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, queuePtr->push, newPush));
        ASSERT(newPush >= 0);

        if (oldPop == newPop && oldPush == newPush && (params->addr == &queuePtr->pop || params->addr == &queuePtr->push))
            return STM_MERGE_OK;

        /* Update the write of the queue tail */
        if (params->addr == &queuePtr->capacity)
            r = ENTRY_GET_READ(params->conflict.entries->e1);
        else
            r = TM_SHARED_DID_READ(queuePtr->capacity);
        ASSERT(STM_VALID_READ(r));
        long oldCapacity, newCapacity;
        ASSERT_FAIL(TM_SHARED_READ_VALUE(r, queuePtr->capacity, oldCapacity));
        ASSERT(oldCapacity >= 0);
        ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, queuePtr->capacity, newCapacity));
        ASSERT(newCapacity >= 0);
# ifdef TM_DEBUG
        printf("QUEUE_PUSH queuePtr->capacity:%p (old):%ld (new):%ld\n", &queuePtr->pop, oldCapacity, newCapacity);
# endif

        const stm_write_t w = TM_SHARED_DID_WRITE(queuePtr->push);
        ASSERT(STM_VALID_WRITE(w));
        ASSERT(STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));
        const long newPushW = (newPush + 1) % newCapacity;
        ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, queuePtr->push, newPushW));
# ifdef TM_DEBUG
        printf("QUEUE_PUSH queuePtr->push:%p (old):%ld (new):%ld, write (new):%ld\n", &queuePtr->push, oldPush, newPush, newPushW);
# endif

        /* FIXME: Queue will resize */
        if (newPushW == newPop) {
            ASSERT(0);
# ifdef TM_DEBUG
            printf("\nQUEUE_PUSH UNSUPPORTED:%p\n", params->addr);
# endif
            return STM_MERGE_ABORT;
        }

        /* Update the new element */
        if (params->addr == &queuePtr->elements)
            r = ENTRY_GET_READ(params->conflict.entries->e1);
        else
            r = TM_SHARED_DID_READ(queuePtr->elements);
        ASSERT(STM_VALID_READ(r));
        void **oldElements, **newElements;
        ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, queuePtr->elements, oldElements));
        ASSERT(oldElements);
        ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, queuePtr->elements, newElements));
        ASSERT(newElements);
# ifdef TM_DEBUG
        printf("QUEUE_PUSH queuePtr->elements:%p (old):%ld (new):%ld\n", &queuePtr->elements, oldElements, newElements);
# endif

        /* Update the new element */
        const stm_write_t welem = TM_SHARED_DID_WRITE(oldElements[oldPush]);
        ASSERT(STM_VALID_WRITE(welem));
        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(welem));
        ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(newElements[newPush])));
        TM_SHARED_WRITE_P(newElements[newPush], dataPtr);

# ifdef TM_DEBUG
        printf("QUEUE_PUSH rv:%ld\n", params->rv.sint);
# endif
        return STM_MERGE_OK;
    }

# ifdef TM_DEBUG
    printf("\nQUEUE_MERGE UNSUPPORTED:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}
# define TMQUEUE_MERGE TMqueue_merge
#else
# define TMQUEUE_MERGE NULL
#endif /* MERGE_QUEUE */

__attribute__((constructor)) void queue_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*pp[], {&ffi_type_pointer, &ffi_type_pointer});
    TM_LOG_TYPE_DECLARE_INIT(*p[], {&ffi_type_pointer});
    # define TM_LOG_OP TM_LOG_OP_INIT
    # include "queue.inc"
    # undef TM_LOG_OP
}
#endif /* ORIGINAL */
