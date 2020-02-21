/* =============================================================================
 *
 * reservation.c
 * -- Representation of car, flight, and hotel relations
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
#include "memory.h"
#include "reservation.h"
#include "tm.h"
#include "types.h"

#if !defined(ORIGINAL) && defined(MERGE_RESERVATION)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "reservation.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_RESERVATION */

/* =============================================================================
 * DECLARATION OF TM_CALLABLE FUNCTIONS
 * =============================================================================
 */

TM_CANCELLABLE
static void
checkReservation (TM_ARGDECL  reservation_t* reservationPtr);

/* =============================================================================
 * reservation_info_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
TM_CALLABLE
reservation_info_t*
reservation_info_alloc (TM_ARGDECL  reservation_type_t type, long id, long price)
{
    reservation_info_t* reservationInfoPtr;

    reservationInfoPtr = (reservation_info_t*)TM_MALLOC(sizeof(reservation_info_t));
    if (reservationInfoPtr != NULL) {
        reservationInfoPtr->type = type;
        reservationInfoPtr->id = id;
        reservationInfoPtr->price = price;
    }

    return reservationInfoPtr;
}

reservation_info_t*
HTMreservation_info_alloc (reservation_type_t type, long id, long price)
{
    reservation_info_t* reservationInfoPtr;

    reservationInfoPtr = (reservation_info_t*)HTM_MALLOC(sizeof(reservation_info_t));
    if (reservationInfoPtr != NULL) {
        reservationInfoPtr->type = type;
        reservationInfoPtr->id = id;
        reservationInfoPtr->price = price;
    }

    return reservationInfoPtr;
}

reservation_info_t*
reservation_info_alloc_seq (reservation_type_t type, long id, long price)
{
    reservation_info_t* reservationInfoPtr;

    reservationInfoPtr = (reservation_info_t*)malloc(sizeof(reservation_info_t));
    if (reservationInfoPtr != NULL) {
        reservationInfoPtr->type = type;
        reservationInfoPtr->id = id;
        reservationInfoPtr->price = price;
    }

    return reservationInfoPtr;
}

/* =============================================================================
 * reservation_info_free
 * =============================================================================
 */
TM_CALLABLE
void
reservation_info_free (TM_ARGDECL  reservation_info_t* reservationInfoPtr)
{
    TM_FREE(reservationInfoPtr);
}

void
HTMreservation_info_free (reservation_info_t* reservationInfoPtr)
{
    HTM_FREE(reservationInfoPtr);
}

void
reservation_info_free_seq (reservation_info_t* reservationInfoPtr)
{
    free(reservationInfoPtr);
}


/* =============================================================================
 * reservation_info_compare
 * -- Returns -1 if A < B, 0 if A = B, 1 if A > B
 * =============================================================================
 */
long
reservation_info_compare (const reservation_info_t* aPtr, const reservation_info_t* bPtr)
{
    long typeDiff;

    typeDiff = aPtr->type - bPtr->type;

    return ((typeDiff != 0) ? (typeDiff) : (aPtr->id - bPtr->id));
}


/* =============================================================================
 * checkReservation
 * -- Check if consistent
 * =============================================================================
 */
TM_CANCELLABLE
static void
checkReservation (TM_ARGDECL  reservation_t* reservationPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_RESERVATION)
    TM_LOG_BEGIN(RSV_CHECK, NULL, reservationPtr);
#endif /* !ORIGINAL && MERGE_RESERVATION */

    long numUsed = (long)TM_SHARED_READ_TAG(reservationPtr->numUsed, reservationPtr);
    if (numUsed < 0) {
        TM_RESTART();
    }

    long numFree = (long)TM_SHARED_READ_TAG(reservationPtr->numFree, reservationPtr);
    if (numFree < 0) {
        TM_RESTART();
    }

    long numTotal = (long)TM_SHARED_READ_TAG(reservationPtr->numTotal, reservationPtr);
    if (numTotal < 0) {
        TM_RESTART();
    }

    if ((numUsed + numFree) != numTotal) {
        TM_RESTART();
    }

    long price = (long)TM_SHARED_READ_TAG(reservationPtr->price, reservationPtr);
    if (price < 0) {
        TM_RESTART();
    }

#if !defined(ORIGINAL) && defined(MERGE_RESERVATION)
    TM_LOG_END(RSV_CHECK, NULL);
#endif /* !ORIGINAL && MERGE_RESERVATION */
}

#define CHECK_RESERVATION(reservation) \
    checkReservation(TM_ARG  reservation)

static void
HTMcheckReservation (reservation_t* reservationPtr)
{
    long numUsed = (long)HTM_SHARED_READ(reservationPtr->numUsed);
    if (numUsed < 0) {
        HTM_RESTART();
    }

    long numFree = (long)HTM_SHARED_READ(reservationPtr->numFree);
    if (numFree < 0) {
        HTM_RESTART();
    }

    long numTotal = (long)HTM_SHARED_READ(reservationPtr->numTotal);
    if (numTotal < 0) {
        HTM_RESTART();
    }

    if ((numUsed + numFree) != numTotal) {
        HTM_RESTART();
    }

    long price = (long)HTM_SHARED_READ(reservationPtr->price);
    if (price < 0) {
        HTM_RESTART();
    }
}

#define HTMCHECK_RESERVATION(reservation) \
    HTMcheckReservation(reservation)

static void
checkReservation_seq (reservation_t* reservationPtr)
{
    assert(reservationPtr->numUsed >= 0);
    assert(reservationPtr->numFree >= 0);
    assert(reservationPtr->numTotal >= 0);
    assert((reservationPtr->numUsed + reservationPtr->numFree) ==
           (reservationPtr->numTotal));
    assert(reservationPtr->price >= 0);
}


/* =============================================================================
 * reservation_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
TM_CANCELLABLE
reservation_t*
reservation_alloc (TM_ARGDECL  long id, long numTotal, long price)
{
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)TM_MALLOC(sizeof(reservation_t));
    if (reservationPtr != NULL) {
        reservationPtr->id = id;
        reservationPtr->numUsed = 0;
        reservationPtr->numFree = numTotal;
        reservationPtr->numTotal = numTotal;
        reservationPtr->price = price;
        CHECK_RESERVATION(reservationPtr);
    }

    return reservationPtr;
}


reservation_t*
HTMreservation_alloc (long id, long numTotal, long price)
{
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)HTM_MALLOC(sizeof(reservation_t));
    if (reservationPtr != NULL) {
        reservationPtr->id = id;
        reservationPtr->numUsed = 0;
        reservationPtr->numFree = numTotal;
        reservationPtr->numTotal = numTotal;
        reservationPtr->price = price;
        HTMCHECK_RESERVATION(reservationPtr);
    }

    return reservationPtr;
}


reservation_t*
reservation_alloc_seq (long id, long numTotal, long price)
{
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)malloc(sizeof(reservation_t));
    if (reservationPtr != NULL) {
        reservationPtr->id = id;
        reservationPtr->numUsed = 0;
        reservationPtr->numFree = numTotal;
        reservationPtr->numTotal = numTotal;
        reservationPtr->price = price;
        checkReservation_seq(reservationPtr);
    }

    return reservationPtr;
}


/* =============================================================================
 * reservation_addToTotal
 * -- Adds if 'num' > 0, removes if 'num' < 0;
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
reservation_addToTotal (TM_ARGDECL  reservation_t* reservationPtr, long num)
{
    long numFree = (long)TM_SHARED_READ_TAG(reservationPtr->numFree, reservationPtr);

    if (numFree + num < 0) {
        return FALSE;
    }

    TM_SHARED_WRITE(reservationPtr->numFree, (numFree + num));
    TM_SHARED_WRITE(reservationPtr->numTotal,
                    ((long)TM_SHARED_READ_TAG(reservationPtr->numTotal, reservationPtr) + num));

    CHECK_RESERVATION(reservationPtr);

    return TRUE;
}


bool_t
HTMreservation_addToTotal (reservation_t* reservationPtr, long num)
{
    long numFree = (long)HTM_SHARED_READ(reservationPtr->numFree);

    if (numFree + num < 0) {
        return FALSE;
    }

    HTM_SHARED_WRITE(reservationPtr->numFree, (numFree + num));
    HTM_SHARED_WRITE(reservationPtr->numTotal,
                    ((long)HTM_SHARED_READ(reservationPtr->numTotal) + num));

    HTMCHECK_RESERVATION(reservationPtr);

    return TRUE;
}


bool_t
reservation_addToTotal_seq (reservation_t* reservationPtr, long num)
{
    if (reservationPtr->numFree + num < 0) {
        return FALSE;
    }

    reservationPtr->numFree += num;
    reservationPtr->numTotal += num;

    checkReservation_seq(reservationPtr);

    return TRUE;
}


/* =============================================================================
 * reservation_make
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
reservation_make (TM_ARGDECL  reservation_t* reservationPtr)
{
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_RESERVATION)
    TM_LOG_BEGIN(RSV_MAKE, NULL, reservationPtr);
#endif /* !ORIGINAL && MERGE_RESERVATION */
    long numFree = (long)TM_SHARED_READ_TAG(reservationPtr->numFree, reservationPtr);

    if (numFree < 1) {
        rv = FALSE;
        goto out;
    }
    TM_SHARED_WRITE(reservationPtr->numUsed,
                    ((long)TM_SHARED_READ_TAG(reservationPtr->numUsed, reservationPtr) + 1));
    TM_SHARED_WRITE(reservationPtr->numFree, (numFree - 1));

    CHECK_RESERVATION(reservationPtr);

    rv = TRUE;
out:
#if !defined(ORIGINAL) && defined(MERGE_RESERVATION)
    TM_LOG_END(RSV_MAKE, &rv);
#endif /* !ORIGINAL && MERGE_RESERVATION */
    return rv;
}


bool_t
HTMreservation_make (reservation_t* reservationPtr)
{
    bool_t rv;

    long numFree = (long)HTM_SHARED_READ(reservationPtr->numFree);

    if (numFree < 1) {
        rv = FALSE;
        goto out;
    }
    HTM_SHARED_WRITE(reservationPtr->numUsed,
                    ((long)HTM_SHARED_READ(reservationPtr->numUsed) + 1));
    HTM_SHARED_WRITE(reservationPtr->numFree, (numFree - 1));

    HTMCHECK_RESERVATION(reservationPtr);

    rv = TRUE;
out:
    return rv;
}


bool_t
reservation_make_seq (reservation_t* reservationPtr)
{
    if (reservationPtr->numFree < 1) {
        return FALSE;
    }

    reservationPtr->numUsed++;
    reservationPtr->numFree--;

    checkReservation_seq(reservationPtr);

    return TRUE;
}


/* =============================================================================
 * reservation_cancel
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
reservation_cancel (TM_ARGDECL  reservation_t* reservationPtr)
{
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_RESERVATION)
    TM_LOG_BEGIN(RSV_CANCEL, NULL, reservationPtr);
#endif /* !ORIGINAL && MERGE_RESERVATION */
    long numUsed = (long)TM_SHARED_READ_TAG(reservationPtr->numUsed, reservationPtr);

    if (numUsed < 1) {
        rv = FALSE;
        goto out;
    }

    TM_SHARED_WRITE(reservationPtr->numUsed, (numUsed - 1));
    TM_SHARED_WRITE(reservationPtr->numFree,
                    ((long)TM_SHARED_READ_TAG(reservationPtr->numFree, reservationPtr) + 1));

    CHECK_RESERVATION(reservationPtr);

    rv = TRUE;
out:
#if !defined(ORIGINAL) && defined(MERGE_RESERVATION)
    TM_LOG_END(RSV_CANCEL, &rv);
#endif /* !ORIGINAL && MERGE_RESERVATION */
    return rv;
}


bool_t
HTMreservation_cancel (reservation_t* reservationPtr)
{
    bool_t rv;

    long numUsed = (long)TM_SHARED_READ(reservationPtr->numUsed);

    if (numUsed < 1) {
        rv = FALSE;
        goto out;
    }

    HTM_SHARED_WRITE(reservationPtr->numUsed, (numUsed - 1));
    HTM_SHARED_WRITE(reservationPtr->numFree,
                    ((long)TM_SHARED_READ(reservationPtr->numFree) + 1));

    HTMCHECK_RESERVATION(reservationPtr);

    rv = TRUE;
out:
    return rv;
}


bool_t
reservation_cancel_seq (reservation_t* reservationPtr)
{
    if (reservationPtr->numUsed < 1) {
        return FALSE;
    }

    reservationPtr->numUsed--;
    reservationPtr->numFree++;

    checkReservation_seq(reservationPtr);

    return TRUE;
}


/* =============================================================================
 * reservation_updatePrice
 * -- Failure if 'price' < 0
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
reservation_updatePrice (TM_ARGDECL  reservation_t* reservationPtr, long newPrice)
{
    if (newPrice < 0) {
        return FALSE;
    }

    TM_SHARED_WRITE(reservationPtr->price, newPrice);

    CHECK_RESERVATION(reservationPtr);

    return TRUE;
}


bool_t
HTMreservation_updatePrice (reservation_t* reservationPtr, long newPrice)
{
    if (newPrice < 0) {
        return FALSE;
    }

    HTM_SHARED_WRITE(reservationPtr->price, newPrice);

    HTMCHECK_RESERVATION(reservationPtr);

    return TRUE;
}


bool_t
reservation_updatePrice_seq (reservation_t* reservationPtr, long newPrice)
{
    if (newPrice < 0) {
        return FALSE;
    }

    reservationPtr->price = newPrice;

    checkReservation_seq(reservationPtr);

    return TRUE;
}


/* =============================================================================
 * reservation_compare
 * -- Returns -1 if A < B, 0 if A = B, 1 if A > B
 * =============================================================================
 */
long
reservation_compare (reservation_t* aPtr, reservation_t* bPtr)
{
    return (aPtr->id - bPtr->id);
}


/* =============================================================================
 * reservation_hash
 * =============================================================================
 */
ulong_t
reservation_hash (reservation_t* reservationPtr)
{
    /* Separate tables for cars, flights, etc, so no need to use 'type' */
    return (ulong_t)reservationPtr->id;
}


/* =============================================================================
 * reservation_free
 * =============================================================================
 */
TM_CALLABLE
void
reservation_free (TM_ARGDECL  reservation_t* reservationPtr)
{
    TM_FREE(reservationPtr);
}


void
HTMreservation_free (reservation_t* reservationPtr)
{
    HTM_FREE(reservationPtr);
}


void
reservation_free_seq (reservation_t* reservationPtr)
{
    free(reservationPtr);
}


/* =============================================================================
 * TEST_RESERVATION
 * =============================================================================
 */
#ifdef TEST_RESERVATION


#include <assert.h>
#include <stdio.h>


int
main ()
{
    reservation_info_t* reservationInfo1Ptr;
    reservation_info_t* reservationInfo2Ptr;
    reservation_info_t* reservationInfo3Ptr;

    reservation_t* reservation1Ptr;
    reservation_t* reservation2Ptr;
    reservation_t* reservation3Ptr;

    assert(memory_init(1, 4, 2));

    puts("Starting...");

    reservationInfo1Ptr = reservation_info_alloc(0, 0, 0);
    reservationInfo2Ptr = reservation_info_alloc(0, 0, 1);
    reservationInfo3Ptr = reservation_info_alloc(2, 0, 1);

    /* Test compare */
    assert(reservation_info_compare(reservationInfo1Ptr, reservationInfo2Ptr) == 0);
    assert(reservation_info_compare(reservationInfo1Ptr, reservationInfo3Ptr) > 0);
    assert(reservation_info_compare(reservationInfo2Ptr, reservationInfo3Ptr) > 0);

    reservation1Ptr = reservation_alloc(0, 0, 0);
    reservation2Ptr = reservation_alloc(0, 0, 1);
    reservation3Ptr = reservation_alloc(2, 0, 1);

    /* Test compare */
    assert(reservation_compare(reservation1Ptr, reservation2Ptr) == 0);
    assert(reservation_compare(reservation1Ptr, reservation3Ptr) != 0);
    assert(reservation_compare(reservation2Ptr, reservation3Ptr) != 0);

    /* Cannot reserve if total is 0 */
    assert(!reservation_make(reservation1Ptr));

    /* Cannot cancel if used is 0 */
    assert(!reservation_cancel(reservation1Ptr));

    /* Cannot update with negative price */
    assert(!reservation_updatePrice(reservation1Ptr, -1));

    /* Cannot make negative total */
    assert(!reservation_addToTotal(reservation1Ptr, -1));

    /* Update total and price */
    assert(reservation_addToTotal(reservation1Ptr, 1));
    assert(reservation_updatePrice(reservation1Ptr, 1));
    assert(reservation1Ptr->numUsed == 0);
    assert(reservation1Ptr->numFree == 1);
    assert(reservation1Ptr->numTotal == 1);
    assert(reservation1Ptr->price == 1);
    checkReservation(reservation1Ptr);

    /* Make and cancel reservation */
    assert(reservation_make(reservation1Ptr));
    assert(reservation_cancel(reservation1Ptr));
    assert(!reservation_cancel(reservation1Ptr));

    reservation_info_free(reservationInfo1Ptr);
    reservation_info_free(reservationInfo2Ptr);
    reservation_info_free(reservationInfo3Ptr);

    reservation_free(reservation1Ptr);
    reservation_free(reservation2Ptr);
    reservation_free(reservation3Ptr);

    puts("All tests passed.");

    return 0;
}


#endif /* TEST_RESERVATION */


/* =============================================================================
 *
 * End of reservation.c
 *
 * =============================================================================
 */

#if !defined(ORIGINAL) && defined(MERGE_RESERVATION)
stm_merge_t TMreservation_merge(stm_merge_context_t *params) {
    stm_op_id_t op = stm_get_op_opcode(params->current);

    if (STM_SAME_OPID(op, RSV_MAKE) || STM_SAME_OPID(op, RSV_CANCEL) || STM_SAME_OPID(op, RSV_CHECK)) {
        ASSERT(params->leaf == 1);
        ASSERT(ENTRY_VALID(params->conflict.entries->e1));
        stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
        stm_write_t w;

        const reservation_t *reservationPtr = (const reservation_t *)TM_SHARED_GET_TAG(r);
        ASSERT(reservationPtr);
# ifdef TM_DEBUG
        printf("\n%s addr:%p reservationPtr:%p\n", STM_SAME_OPID(op, RSV_MAKE) ? "RSV_MAKE" : "RSV_CANCEL", params->addr, reservationPtr);
# endif

        ASSERT(STM_VALID_READ(r));
        ASSERT(!((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) || STM_SAME_OPID(op, RSV_CHECK) || params->rv.sint == TRUE);
        /* Check the reservationPtr->price */
        if (params->addr == &reservationPtr->price) {
            long oldPrice, newPrice;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, reservationPtr->price, oldPrice));
            ASSERT(oldPrice >= 0);
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, reservationPtr->price, newPrice));
            ASSERT(newPrice >= 0);
            if (oldPrice == newPrice)
                return STM_MERGE_OK;
# ifdef TM_DEBUG
            printf("%s reservationPtr->price (old):%ld (new):%ld\n", STM_SAME_OPID(op, RSV_MAKE) ? "RSV_MAKE" : "RSV_CANCEL", oldPrice, newPrice);
# endif
            return STM_MERGE_OK;
        /* Update the read on reservationPtr->total */
        } else if (params->addr == &reservationPtr->numTotal) {
            long oldTotal, newTotal;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, reservationPtr->numTotal, oldTotal));
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, reservationPtr->numTotal, newTotal));

# ifdef TM_DEBUG
            printf("%s reservationPtr->numTotal (old):%ld (new):%ld\n", STM_SAME_OPID(op, RSV_MAKE) ? "RSV_MAKE" : "RSV_CANCEL", oldTotal, newTotal);
# endif
            return STM_MERGE_OK;

        } else if (params->addr == &reservationPtr->numFree) {
            long oldFree, newFree;

            /* Update the read on reservationPtr->numFree */
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, reservationPtr->numFree, oldFree));
            if (STM_SAME_OPID(op, RSV_CHECK))
                ASSERT(oldFree >= 0);
            else
                ASSERT(STM_SAME_OPID(op, RSV_MAKE) ? oldFree > 0 : oldFree >= 0);
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, reservationPtr->numFree, newFree));
            if (STM_SAME_OPID(op, RSV_MAKE) && __builtin_expect(newFree < 1, 0))
                return STM_MERGE_ABORT;
            ASSERT(newFree >= 0);
            if (oldFree == newFree)
                return STM_MERGE_OK;

            /* Update the write on reservationPtr->numFree */
            if (!STM_SAME_OPID(op, RSV_CHECK)) {
                /* Get the write on reservationPtr->numFree */
                w = TM_SHARED_DID_WRITE(reservationPtr->numFree);
                ASSERT_FAIL(STM_VALID_WRITE(w));
                ASSERT((stm_get_features() & STM_FEATURE_OPLOG_FULL) != STM_FEATURE_OPLOG_FULL || STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));

                const long newFreeW = STM_SAME_OPID(op, RSV_MAKE) ? newFree - 1 : newFree + 1;
                ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, reservationPtr->numFree, newFreeW));
# ifdef TM_DEBUG
                printf("%s reservationPtr->numFree:%p (old):%ld (new):%ld, write (new):%ld\n", STM_SAME_OPID(op, RSV_MAKE) ? "RSV_MAKE" : "RSV_CANCEL", &reservationPtr->numFree, oldFree, newFree, newFreeW);
# endif
            } else {
# ifdef TM_DEBUG
                printf("%s reservationPtr->numFree:%p (old):%ld (new):%ld\n", STM_SAME_OPID(op, RSV_MAKE) ? "RSV_MAKE" : "RSV_CANCEL", &reservationPtr->numFree, oldFree, newFree);
# endif
            }

            return STM_MERGE_OK;
        } else if (params->addr == &reservationPtr->numUsed) {
            long oldUsed, newUsed;

            /* Update the read on reservationPtr->numUsed */
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, reservationPtr->numUsed, oldUsed));
            if (STM_SAME_OPID(op, RSV_CHECK))
                ASSERT(oldUsed >= 0);
            else
                ASSERT(STM_SAME_OPID(op, RSV_MAKE) ? oldUsed >= 0 : oldUsed > 0);
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, reservationPtr->numUsed, newUsed));
            if (STM_SAME_OPID(op, RSV_CANCEL) && __builtin_expect(newUsed < 1, 0))
                    return STM_MERGE_ABORT;
            ASSERT(newUsed >= 0);
            if (oldUsed == newUsed)
                return STM_MERGE_OK;

            /* Update the write on reservationPtr->numUsed */
            if (!STM_SAME_OPID(op, RSV_CHECK)) {
                 /* Get the write on reservationPtr->numUsed */
                w = TM_SHARED_DID_WRITE(reservationPtr->numUsed);
                ASSERT(STM_VALID_WRITE(w));
                ASSERT((stm_get_features() & STM_FEATURE_OPLOG_FULL) != STM_FEATURE_OPLOG_FULL || STM_SAME_OP(stm_get_load_op(r), stm_get_store_op(w)));

                const long newUsedW = STM_SAME_OPID(op, RSV_MAKE) ? newUsed + 1 : newUsed - 1;
                ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, reservationPtr->numUsed, newUsedW));
# ifdef TM_DEBUG
                printf("%s reservationPtr->numUsed:%p (old):%ld (new):%ld, write (new):%ld\n", STM_SAME_OPID(op, RSV_MAKE) ? "RSV_MAKE" : "RSV_CANCEL", &reservationPtr->numUsed, oldUsed, newUsed, newUsedW);
# endif
            } else {
# ifdef TM_DEBUG
                printf("%s reservationPtr->numUsed:%p (old):%ld (new):%ld\n", STM_SAME_OPID(op, RSV_MAKE) ? "RSV_MAKE" : "RSV_CANCEL", &reservationPtr->numUsed, oldUsed, newUsed);
# endif
            }

            return STM_MERGE_OK;
        }
    }

# ifdef TM_DEBUG
    printf("\nRSV_MERGE UNSUPPORTED addr:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}

__attribute__((constructor)) void reservation_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*p[], {&ffi_type_pointer});
    # define TM_LOG_OP TM_LOG_OP_INIT
    # include "reservation.inc"
    # undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_RESERVATION */
