/* =============================================================================
 *
 * manager.c
 * -- Travel reservation resource manager
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
#include "customer.h"
#include "map.h"
#include "pair.h"
#include "manager.h"
#include "reservation.h"
#include "tm.h"
#include "types.h"

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "manager.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_MANAGER */

/* =============================================================================
 * DECLARATION OF TM_CALLABLE FUNCTIONS
 * =============================================================================
 */

static void (*freeData)(void *);

TM_CALLABLE
static long
queryNumFree (TM_ARGDECL  MAP_T* tablePtr, long id);

TM_CALLABLE
static long
queryPrice (TM_ARGDECL  MAP_T* tablePtr, long id);

TM_CANCELLABLE
static bool_t
reserve (TM_ARGDECL MAP_T* tablePtr, MAP_T* customerTablePtr, long customerId, long id, reservation_type_t type);

TM_CANCELLABLE
bool_t
addReservation (TM_ARGDECL  MAP_T* tablePtr, long id, long num, long price);


/* =============================================================================
 * tableAlloc
 * =============================================================================
 */
static MAP_T*
tableAlloc ()
{
    return MAP_ALLOC(NULL, NULL);
}


/* =============================================================================
 * manager_alloc
 * =============================================================================
 */
manager_t*
manager_alloc ()
{
    manager_t* managerPtr = (manager_t*)malloc(sizeof(manager_t));
    assert(managerPtr != NULL);

    managerPtr->carTablePtr = tableAlloc();
    managerPtr->roomTablePtr = tableAlloc();
    managerPtr->flightTablePtr = tableAlloc();
    managerPtr->customerTablePtr = tableAlloc();
    assert(managerPtr->carTablePtr != NULL);
    assert(managerPtr->roomTablePtr != NULL);
    assert(managerPtr->flightTablePtr != NULL);
    assert(managerPtr->customerTablePtr != NULL);

    return managerPtr;
}


/* =============================================================================
 * tableFree
 * =============================================================================
 */
static void
tableFree (void *k, void *v)
{
    if (freeData && v)
        freeData(v);
}



/* =============================================================================
 * manager_free
 * =============================================================================
 */
void
manager_free (manager_t* managerPtr)
{
    freeData = (void (*)(void *))reservation_free_seq;
    MAP_FREE(managerPtr->carTablePtr, tableFree);
    freeData = (void (*)(void *))reservation_free_seq;
    MAP_FREE(managerPtr->roomTablePtr, tableFree);
    freeData = (void (*)(void *))reservation_free_seq;
    MAP_FREE(managerPtr->flightTablePtr, tableFree);
    freeData = (void (*)(void *))customer_free_seq;
    MAP_FREE(managerPtr->customerTablePtr, tableFree);

    free(managerPtr);
}


/* =============================================================================
 * ADMINISTRATIVE INTERFACE
 * =============================================================================
 */


/* =============================================================================
 * addReservation
 * -- If 'num' > 0 then add, if < 0 remove
 * -- Adding 0 seats is error if does not exist
 * -- If 'price' < 0, do not update price
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
addReservation (TM_ARGDECL  MAP_T* tablePtr, long id, long num, long price)
{
    reservation_t* reservationPtr;
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_BEGIN(MGR_ADDRESERVE, NULL, tablePtr, id, num, price);
#endif /* !ORIGINAL && MERGE_MANAGER */
    reservationPtr = (reservation_t*)TMMAP_FIND(tablePtr, id);
    if (reservationPtr == NULL) {
        /* Create new reservation */
        if (num < 1 || price < 0) {
            rv = FALSE;
            goto out;
        }
        reservationPtr = RESERVATION_ALLOC(id, num, price);
        assert(reservationPtr != NULL);
        TMMAP_INSERT(tablePtr, id, reservationPtr);
    } else {
        /* Update existing reservation */
        if (!RESERVATION_ADD_TO_TOTAL(reservationPtr, num)) {
            rv = FALSE;
            goto out;
        }
        if ((long)TM_SHARED_READ_TAG(reservationPtr->numTotal, reservationPtr) == 0) {
            rv = TMMAP_REMOVE(tablePtr, id);
            if (rv == FALSE) {
                TM_RESTART();
            }
            RESERVATION_FREE(reservationPtr);
        } else {
            RESERVATION_UPDATE_PRICE(reservationPtr, price);
        }
    }

    rv = TRUE;
out:
#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_END(MGR_ADDRESERVE, &price);
#endif /* !ORIGINAL && MERGE_MANAGER */
    return rv;
}

bool_t
HTMaddReservation (MAP_T* tablePtr, long id, long num, long price)
{
    reservation_t* reservationPtr;
    bool_t status;

    reservationPtr = (reservation_t*)HTMMAP_FIND(tablePtr, id);
    if (reservationPtr == NULL) {
        /* Create new reservation */
        if (num < 1 || price < 0) {
            return FALSE;
        }
        reservationPtr = HTMRESERVATION_ALLOC(id, num, price);
        assert(reservationPtr != NULL);
        status = HTMMAP_INSERT(tablePtr, id, reservationPtr);
        assert(status);
    } else {
        /* Update existing reservation */
        if (!HTMRESERVATION_ADD_TO_TOTAL(reservationPtr, num)) {
            return FALSE;
        }
        if (reservationPtr->numTotal == 0) {
            status = HTMMAP_REMOVE(tablePtr, id);
            assert(status);
        } else {
            HTMRESERVATION_ADD_TO_TOTAL(reservationPtr, price);
        }
    }

    return TRUE;
}

bool_t
addReservation_seq (MAP_T* tablePtr, long id, long num, long price)
{
    reservation_t* reservationPtr;
    bool_t status;

    reservationPtr = (reservation_t*)MAP_FIND(tablePtr, id);
    if (reservationPtr == NULL) {
        /* Create new reservation */
        if (num < 1 || price < 0) {
            return FALSE;
        }
        reservationPtr = RESERVATION_ALLOC_SEQ(id, num, price);
        assert(reservationPtr != NULL);
        status = MAP_INSERT(tablePtr, id, reservationPtr);
        assert(status);
    } else {
        /* Update existing reservation */
        if (!RESERVATION_ADD_TO_TOTAL_SEQ(reservationPtr, num)) {
            return FALSE;
        }
        if (reservationPtr->numTotal == 0) {
            status = MAP_REMOVE(tablePtr, id);
            assert(status);
        } else {
            RESERVATION_ADD_TO_TOTAL_SEQ(reservationPtr, price);
        }
    }

    return TRUE;
}


/* =============================================================================
 * manager_addCar
 * -- Add cars to a city
 * -- Adding to an existing car overwrite the price if 'price' >= 0
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_addCar (TM_ARGDECL
                manager_t* managerPtr, long carId, long numCars, long price)
{
    return addReservation(TM_ARG  managerPtr->carTablePtr, carId, numCars, price);
}

bool_t
HTMmanager_addCar (manager_t* managerPtr, long carId, long numCars, long price)
{
    return HTMaddReservation(managerPtr->carTablePtr, carId, numCars, price);
}

bool_t
manager_addCar_seq (manager_t* managerPtr, long carId, long numCars, long price)
{
    return addReservation_seq(managerPtr->carTablePtr, carId, numCars, price);
}


/* =============================================================================
 * manager_deleteCar
 * -- Delete cars from a city
 * -- Decreases available car count (those not allocated to a customer)
 * -- Fails if would make available car count negative
 * -- If decresed to 0, deletes entire entry
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_deleteCar (TM_ARGDECL  manager_t* managerPtr, long carId, long numCar)
{
    /* -1 keeps old price */
    return addReservation(TM_ARG  managerPtr->carTablePtr, carId, -numCar, -1);
}

bool_t
HTMmanager_deleteCar (manager_t* managerPtr, long carId, long numCar)
{
    /* -1 keeps old price */
    return HTMaddReservation(managerPtr->carTablePtr, carId, -numCar, -1);
}

bool_t
manager_deleteCar_seq (manager_t* managerPtr, long carId, long numCar)
{
    /* -1 keeps old price */
    return addReservation_seq(managerPtr->carTablePtr, carId, -numCar, -1);
}


/* =============================================================================
 * manager_addRoom
 * -- Add rooms to a city
 * -- Adding to an existing room overwrite the price if 'price' >= 0
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_addRoom (TM_ARGDECL
                 manager_t* managerPtr, long roomId, long numRoom, long price)
{
    return addReservation(TM_ARG  managerPtr->roomTablePtr, roomId, numRoom, price);
}

bool_t
HTMmanager_addRoom (manager_t* managerPtr, long roomId, long numRoom, long price)
{
    return HTMaddReservation(managerPtr->roomTablePtr, roomId, numRoom, price);
}

bool_t
manager_addRoom_seq (manager_t* managerPtr, long roomId, long numRoom, long price)
{
    return addReservation_seq(managerPtr->roomTablePtr, roomId, numRoom, price);
}


/* =============================================================================
 * manager_deleteRoom
 * -- Delete rooms from a city
 * -- Decreases available room count (those not allocated to a customer)
 * -- Fails if would make available room count negative
 * -- If decresed to 0, deletes entire entry
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_deleteRoom (TM_ARGDECL  manager_t* managerPtr, long roomId, long numRoom)
{
    /* -1 keeps old price */
    return addReservation(TM_ARG  managerPtr->roomTablePtr, roomId, -numRoom, -1);
}

bool_t
HTMmanager_deleteRoom (manager_t* managerPtr, long roomId, long numRoom)
{
    /* -1 keeps old price */
    return HTMaddReservation(managerPtr->roomTablePtr, roomId, -numRoom, -1);
}

bool_t
manager_deleteRoom_seq (manager_t* managerPtr, long roomId, long numRoom)
{
    /* -1 keeps old price */
    return addReservation_seq(TM_ARG  managerPtr->roomTablePtr, roomId, -numRoom, -1);
}


/* =============================================================================
 * manager_addFlight
 * -- Add seats to a flight
 * -- Adding to an existing flight overwrite the price if 'price' >= 0
 * -- Returns TRUE on success, FALSE on failure
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_addFlight (TM_ARGDECL
                   manager_t* managerPtr, long flightId, long numSeat, long price)
{
    return addReservation(TM_ARG
                          managerPtr->flightTablePtr, flightId, numSeat, price);
}

bool_t
HTMmanager_addFlight (manager_t* managerPtr, long flightId, long numSeat, long price)
{
    return HTMaddReservation(managerPtr->flightTablePtr, flightId, numSeat, price);
}

bool_t
manager_addFlight_seq (manager_t* managerPtr, long flightId, long numSeat, long price)
{
    return addReservation_seq(managerPtr->flightTablePtr, flightId, numSeat, price);
}


/* =============================================================================
 * manager_deleteFlight
 * -- Delete an entire flight
 * -- Fails if customer has reservation on this flight
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_deleteFlight (TM_ARGDECL  manager_t* managerPtr, long flightId)
{
    reservation_t* reservationPtr;
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_BEGIN(MGR_DELFLIGHT, NULL, managerPtr, flightId);
#endif /* !ORIGINAL && MERGE_MANAGER */
    reservationPtr = (reservation_t*)TMMAP_FIND(managerPtr->flightTablePtr, flightId);
    if (reservationPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    if ((long)TM_SHARED_READ_TAG(reservationPtr->numUsed, reservationPtr) > 0) {
        rv = FALSE; /* somebody has a reservation */
        goto out;
    }

    rv = addReservation(TM_ARG
                        managerPtr->flightTablePtr,
                        flightId,
                        -1*(long)TM_SHARED_READ_TAG(reservationPtr->numTotal, reservationPtr),
                        -1 /* -1 keeps old price */);
out:
#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_END(MGR_DELFLIGHT, &rv);
#endif /* !ORIGINAL && MERGE_MANAGER */
    return rv;
}

bool_t
HTMmanager_deleteFlight (manager_t* managerPtr, long flightId)
{
    reservation_t* reservationPtr;
    bool_t rv;

    reservationPtr = (reservation_t*)HTMMAP_FIND(managerPtr->flightTablePtr, flightId);
    if (reservationPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    if (reservationPtr->numUsed > 0) {
        rv = FALSE; /* somebody has a reservation */
        goto out;
    }

    rv = HTMaddReservation(TM_ARG
                        managerPtr->flightTablePtr,
                        flightId,
                        -1*reservationPtr->numTotal,
                        -1 /* -1 keeps old price */);
out:
    return rv;
}

bool_t
manager_deleteFlight_seq (manager_t* managerPtr, long flightId)
{
    reservation_t* reservationPtr;
    bool_t rv;

    reservationPtr = (reservation_t*)MAP_FIND(managerPtr->flightTablePtr, flightId);
    if (reservationPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    if (reservationPtr->numUsed > 0) {
        rv = FALSE; /* somebody has a reservation */
        goto out;
    }

    rv = addReservation_seq(TM_ARG
                        managerPtr->flightTablePtr,
                        flightId,
                        -1*reservationPtr->numTotal,
                        -1 /* -1 keeps old price */);
out:
    return rv;
}


/* =============================================================================
 * manager_addCustomer
 * -- If customer already exists, returns failure
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_addCustomer (TM_ARGDECL  manager_t* managerPtr, long customerId)
{
    customer_t* customerPtr;
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_BEGIN(MGR_ADDCUSTOMER, NULL, managerPtr, customerId);
#endif /* !ORIGINAL && MERGE_MANAGER */
    if (TMMAP_CONTAINS(managerPtr->customerTablePtr, customerId)) {
        rv = FALSE;
        goto out;
    }

    customerPtr = CUSTOMER_ALLOC(customerId);
    assert(customerPtr != NULL);
    rv = TMMAP_INSERT(managerPtr->customerTablePtr, customerId, customerPtr);
    if (rv == FALSE) {
        TM_RESTART();
    }

    rv = TRUE;
out:
#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_END(MGR_ADDCUSTOMER, &rv);
#endif /* !ORIGINAL && MERGE_MANAGER */
    return rv;
}

bool_t
HTMmanager_addCustomer (manager_t* managerPtr, long customerId)
{
    customer_t* customerPtr;
    bool_t rv;

    if (HTMMAP_CONTAINS(managerPtr->customerTablePtr, customerId)) {
        rv = FALSE;
        goto out;
    }

    customerPtr = HTMCUSTOMER_ALLOC(customerId);
    assert(customerPtr != NULL);
    rv = HTMMAP_INSERT(managerPtr->customerTablePtr, customerId, customerPtr);
    if (rv == FALSE) {
        HTM_RESTART();
    }

    rv = TRUE;
out:
    return rv;
}

bool_t
manager_addCustomer_seq (manager_t* managerPtr, long customerId)
{
    customer_t* customerPtr;
    bool_t status;

    if (MAP_CONTAINS(managerPtr->customerTablePtr, customerId)) {
        return FALSE;
    }

    customerPtr = CUSTOMER_ALLOC_SEQ(customerId);
    assert(customerPtr != NULL);
    status = MAP_INSERT(managerPtr->customerTablePtr, customerId, customerPtr);
    assert(status);

    return TRUE;
}


/* =============================================================================
 * manager_deleteCustomer
 * -- Delete this customer and associated reservations
 * -- If customer does not exist, returns success
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_deleteCustomer (TM_ARGDECL  manager_t* managerPtr, long customerId)
{
    customer_t* customerPtr;
    MAP_T* reservationTables[NUM_RESERVATION_TYPE];
    list_t* reservationInfoListPtr;
    list_iter_t it;
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_BEGIN(MGR_DELCUSTOMER, NULL, managerPtr, customerId);
#endif /* !ORIGINAL && MERGE_MANAGER */
    customerPtr = (customer_t*)TMMAP_FIND(managerPtr->customerTablePtr, customerId);
    if (customerPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    reservationTables[RESERVATION_CAR] = managerPtr->carTablePtr;
    reservationTables[RESERVATION_ROOM] = managerPtr->roomTablePtr;
    reservationTables[RESERVATION_FLIGHT] = managerPtr->flightTablePtr;

    /* Cancel this customer's reservations */
    reservationInfoListPtr = customerPtr->reservationInfoListPtr;
    TMLIST_ITER_RESET(&it, reservationInfoListPtr);
    while (TMLIST_ITER_HASNEXT(&it, reservationInfoListPtr)) {
        reservation_info_t* reservationInfoPtr;
        reservation_t* reservationPtr;
        reservationInfoPtr =
            (reservation_info_t*)TMLIST_ITER_NEXT(&it, reservationInfoListPtr);
        reservationPtr =
            (reservation_t*)TMMAP_FIND(reservationTables[reservationInfoPtr->type],
                                     reservationInfoPtr->id);
        if (reservationPtr == NULL) {
            TM_RESTART();
        }
        rv = RESERVATION_CANCEL(reservationPtr);
        if (rv == FALSE) {
            TM_RESTART();
        }
        RESERVATION_INFO_FREE(reservationInfoPtr);
    }

    rv = TMMAP_REMOVE(managerPtr->customerTablePtr, customerId);
    if (rv == FALSE) {
        TM_RESTART();
    }
    CUSTOMER_FREE(customerPtr);

    rv = TRUE;
out:
#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_END(MGR_DELCUSTOMER, &rv);
#endif /* !ORIGINAL && MERGE_MANAGER */
    return rv;
}

bool_t
HTMmanager_deleteCustomer (manager_t* managerPtr, long customerId)
{
    customer_t* customerPtr;
    MAP_T* reservationTables[NUM_RESERVATION_TYPE];
    list_t* reservationInfoListPtr;
    list_iter_t it;
    bool_t rv;

    customerPtr = (customer_t*)HTMMAP_FIND(managerPtr->customerTablePtr, customerId);
    if (customerPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    reservationTables[RESERVATION_CAR] = managerPtr->carTablePtr;
    reservationTables[RESERVATION_ROOM] = managerPtr->roomTablePtr;
    reservationTables[RESERVATION_FLIGHT] = managerPtr->flightTablePtr;

    /* Cancel this customer's reservations */
    reservationInfoListPtr = customerPtr->reservationInfoListPtr;
    HTMLIST_ITER_RESET(&it, reservationInfoListPtr);
    while (HTMLIST_ITER_HASNEXT(&it, reservationInfoListPtr)) {
        reservation_info_t* reservationInfoPtr;
        reservation_t* reservationPtr;
        reservationInfoPtr =
            (reservation_info_t*)HTMLIST_ITER_NEXT(&it, reservationInfoListPtr);
        reservationPtr =
            (reservation_t*)HTMMAP_FIND(reservationTables[reservationInfoPtr->type],
                                     reservationInfoPtr->id);
        if (reservationPtr == NULL) {
            HTM_RESTART();
        }
        rv = HTMRESERVATION_CANCEL(reservationPtr);
        if (rv == FALSE) {
            HTM_RESTART();
        }
        HTMRESERVATION_INFO_FREE(reservationInfoPtr);
    }

    rv = HTMMAP_REMOVE(managerPtr->customerTablePtr, customerId);
    if (rv == FALSE) {
        HTM_RESTART();
    }
    HTMCUSTOMER_FREE(customerPtr);

    rv = TRUE;
out:
    return rv;
}

bool_t
manager_deleteCustomer_seq (manager_t* managerPtr, long customerId)
{
    customer_t* customerPtr;
    MAP_T* reservationTables[NUM_RESERVATION_TYPE];
    list_t* reservationInfoListPtr;
    list_iter_t it;
    bool_t rv;

    customerPtr = (customer_t*)MAP_FIND(managerPtr->customerTablePtr, customerId);
    if (customerPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    reservationTables[RESERVATION_CAR] = managerPtr->carTablePtr;
    reservationTables[RESERVATION_ROOM] = managerPtr->roomTablePtr;
    reservationTables[RESERVATION_FLIGHT] = managerPtr->flightTablePtr;

    /* Cancel this customer's reservations */
    reservationInfoListPtr = customerPtr->reservationInfoListPtr;
    LIST_ITER_RESET(&it, reservationInfoListPtr);
    while (LIST_ITER_HASNEXT(&it, reservationInfoListPtr)) {
        reservation_info_t* reservationInfoPtr;
        reservation_t* reservationPtr;
        reservationInfoPtr =
            (reservation_info_t*)LIST_ITER_NEXT(&it, reservationInfoListPtr);
        reservationPtr =
            (reservation_t*)MAP_FIND(reservationTables[reservationInfoPtr->type],
                                     reservationInfoPtr->id);
        assert (reservationPtr != NULL);
        rv = RESERVATION_CANCEL_SEQ(reservationPtr);
        assert (rv != FALSE);
        RESERVATION_INFO_FREE_SEQ(reservationInfoPtr);
    }

    rv = MAP_REMOVE(managerPtr->customerTablePtr, customerId);
    assert (rv != FALSE);
    CUSTOMER_FREE_SEQ(customerPtr);

    rv = TRUE;
out:
    return rv;
}


/* =============================================================================
 * QUERY INTERFACE
 * =============================================================================
 */


/* =============================================================================
 * queryNumFree
 * -- Return numFree of a reservation, -1 if failure
 * =============================================================================
 */
TM_CALLABLE
static long
queryNumFree (TM_ARGDECL  MAP_T* tablePtr, long id)
{
    long numFree = -1;
    reservation_t* reservationPtr;

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_BEGIN(MGR_QUERYFREE, NULL, tablePtr, id);
#endif /* !ORIGINAL && MERGE_MANAGER */
    reservationPtr = (reservation_t*)TMMAP_FIND(tablePtr, id);
    if (reservationPtr != NULL) {
        numFree = (long)TM_SHARED_READ_TAG(reservationPtr->numFree, reservationPtr);
    }
#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_END(MGR_QUERYFREE, &numFree);
#endif /* !ORIGINAL && MERGE_MANAGER */

    return numFree;
}

static long
HTMqueryNumFree (MAP_T* tablePtr, long id)
{
    long numFree = -1;
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)HTMMAP_FIND(tablePtr, id);
    if (reservationPtr != NULL) {
        numFree = (long)HTM_SHARED_READ(reservationPtr->numFree);
    }

    return numFree;
}

static long
queryNumFree_seq (MAP_T* tablePtr, long id)
{
    long numFree = -1;
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)MAP_FIND(tablePtr, id);
    if (reservationPtr != NULL) {
        numFree = reservationPtr->numFree;
    }

    return numFree;
}


/* =============================================================================
 * queryPrice
 * -- Return price of a reservation, -1 if failure
 * =============================================================================
 */
TM_CALLABLE
static long
queryPrice (TM_ARGDECL  MAP_T* tablePtr, long id)
{
    long price = -1;
    reservation_t* reservationPtr;

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_BEGIN(MGR_QUERYPRICE, NULL, tablePtr, id);
#endif /* !ORIGINAL && MERGE_MANAGER */
    reservationPtr = (reservation_t*)TMMAP_FIND(tablePtr, id);
    if (reservationPtr != NULL) {
        price = (long)TM_SHARED_READ_TAG(reservationPtr->price, reservationPtr);
    }
#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_END(MGR_QUERYPRICE, &price);
#endif /* !ORIGINAL && MERGE_MANAGER */

    return price;
}

static long
HTMqueryPrice (TM_ARGDECL  MAP_T* tablePtr, long id)
{
    long price = -1;
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)HTMMAP_FIND(tablePtr, id);
    if (reservationPtr != NULL) {
        price = (long)HTM_SHARED_READ(reservationPtr->price);
    }

    return price;
}

static long
queryPrice_seq (MAP_T* tablePtr, long id)
{
    long price = -1;
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)MAP_FIND(tablePtr, id);
    if (reservationPtr != NULL) {
        price = reservationPtr->price;
    }

    return price;
}


/* =============================================================================
 * manager_queryCar
 * -- Return the number of empty seats on a car
 * -- Returns -1 if the car does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryCar (TM_ARGDECL  manager_t* managerPtr, long carId)
{
    return queryNumFree(TM_ARG  managerPtr->carTablePtr, carId);
}

long
HTMmanager_queryCar (manager_t* managerPtr, long carId)
{
    return HTMqueryNumFree(managerPtr->carTablePtr, carId);
}

long
manager_queryCar_seq (TM_ARGDECL  manager_t* managerPtr, long carId)
{
    return queryNumFree_seq(TM_ARG  managerPtr->carTablePtr, carId);
}


/* =============================================================================
 * manager_queryCarPrice
 * -- Return the price of the car
 * -- Returns -1 if the car does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryCarPrice (TM_ARGDECL  manager_t* managerPtr, long carId)
{
    return queryPrice(TM_ARG  managerPtr->carTablePtr, carId);
}

long
HTMmanager_queryCarPrice (manager_t* managerPtr, long carId)
{
    return HTMqueryPrice(managerPtr->carTablePtr, carId);
}

long
manager_queryCarPrice_seq (TM_ARGDECL  manager_t* managerPtr, long carId)
{
    return queryPrice_seq(TM_ARG  managerPtr->carTablePtr, carId);
}


/* =============================================================================
 * manager_queryRoom
 * -- Return the number of empty seats on a room
 * -- Returns -1 if the room does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryRoom (TM_ARGDECL  manager_t* managerPtr, long roomId)
{
    return queryNumFree(TM_ARG  managerPtr->roomTablePtr, roomId);
}

long
HTMmanager_queryRoom (manager_t* managerPtr, long roomId)
{
    return HTMqueryNumFree(managerPtr->roomTablePtr, roomId);
}

long
manager_queryRoom_seq (manager_t* managerPtr, long roomId)
{
    return queryNumFree_seq(managerPtr->roomTablePtr, roomId);
}


/* =============================================================================
 * manager_queryRoomPrice
 * -- Return the price of the room
 * -- Returns -1 if the room does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryRoomPrice (TM_ARGDECL  manager_t* managerPtr, long roomId)
{
    return queryPrice(TM_ARG  managerPtr->roomTablePtr, roomId);
}

long
HTMmanager_queryRoomPrice (manager_t* managerPtr, long roomId)
{
    return HTMqueryPrice(managerPtr->roomTablePtr, roomId);
}

long
manager_queryRoomPrice_seq (manager_t* managerPtr, long roomId)
{
    return queryPrice_seq(managerPtr->roomTablePtr, roomId);
}


/* =============================================================================
 * manager_queryFlight
 * -- Return the number of empty seats on a flight
 * -- Returns -1 if the flight does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryFlight (TM_ARGDECL  manager_t* managerPtr, long flightId)
{
    return queryNumFree(TM_ARG  managerPtr->flightTablePtr, flightId);
}

long
HTMmanager_queryFlight (manager_t* managerPtr, long flightId)
{
    return HTMqueryNumFree(managerPtr->flightTablePtr, flightId);
}

long
manager_queryFlight_seq (manager_t* managerPtr, long flightId)
{
    return queryNumFree_seq(managerPtr->flightTablePtr, flightId);
}


/* =============================================================================
 * manager_queryFlightPrice
 * -- Return the price of the flight
 * -- Returns -1 if the flight does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryFlightPrice (TM_ARGDECL  manager_t* managerPtr, long flightId)
{
    return queryPrice(TM_ARG  managerPtr->flightTablePtr, flightId);
}

long
HTMmanager_queryFlightPrice (manager_t* managerPtr, long flightId)
{
    return HTMqueryPrice(managerPtr->flightTablePtr, flightId);
}

long
manager_queryFlightPrice_seq (manager_t* managerPtr, long flightId)
{
    return queryPrice_seq(managerPtr->flightTablePtr, flightId);
}


/* =============================================================================
 * manager_queryCustomerBill
 * -- Return the total price of all reservations held for a customer
 * -- Returns -1 if the customer does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryCustomerBill (TM_ARGDECL  manager_t* managerPtr, long customerId)
{
    long bill = -1;
    customer_t* customerPtr;

    customerPtr = (customer_t*)TMMAP_FIND(managerPtr->customerTablePtr, customerId);

    if (customerPtr != NULL) {
        bill = CUSTOMER_GET_BILL(customerPtr);
    }

    return bill;
}

long
HTMmanager_queryCustomerBill (manager_t* managerPtr, long customerId)
{
    long bill = -1;
    customer_t* customerPtr;

    customerPtr = (customer_t*)HTMMAP_FIND(managerPtr->customerTablePtr, customerId);

    if (customerPtr != NULL) {
        bill = HTMCUSTOMER_GET_BILL(customerPtr);
    }

    return bill;
}

long
manager_queryCustomerBill_seq (manager_t* managerPtr, long customerId)
{
    long bill = -1;
    customer_t* customerPtr;

    customerPtr = (customer_t*)MAP_FIND(managerPtr->customerTablePtr, customerId);

    if (customerPtr != NULL) {
        bill = CUSTOMER_GET_BILL_SEQ(customerPtr);
    }

    return bill;
}


/* =============================================================================
 * RESERVATION INTERFACE
 * =============================================================================
 */


/* =============================================================================
 * reserve
 * -- Customer is not allowed to reserve same (type, id) multiple times
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
static bool_t
reserve (TM_ARGDECL
         MAP_T* tablePtr, MAP_T* customerTablePtr,
         long customerId, long id, reservation_type_t type)
{
    customer_t* customerPtr;
    reservation_t* reservationPtr;
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_BEGIN(MGR_RESERVE, NULL, tablePtr, customerTablePtr, customerId, id, type);
#endif /* !ORIGINAL && MERGE_MANAGER */
    customerPtr = (customer_t*)TMMAP_FIND(customerTablePtr, customerId);
    if (customerPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    reservationPtr = (reservation_t*)TMMAP_FIND(tablePtr, id);
    if (reservationPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    if (!RESERVATION_MAKE(reservationPtr)) {
        rv = FALSE;
        goto out;
    }

    if (!CUSTOMER_ADD_RESERVATION_INFO(
            customerPtr,
            type,
            id,
            (long)TM_SHARED_READ_TAG(reservationPtr->price, reservationPtr)))
    {
        /* Undo previous successful reservation */
        rv = RESERVATION_CANCEL(reservationPtr);
        if (rv == FALSE) {
            TM_RESTART();
        }
        rv = FALSE;
        goto out;
    }

    rv = TRUE;
out:
#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_END(MGR_RESERVE, &rv);
#endif /* !ORIGINAL && MERGE_MANAGER */
    return rv;
}

static bool_t
HTMreserve (MAP_T* tablePtr, MAP_T* customerTablePtr,
         long customerId, long id, reservation_type_t type)
{
    customer_t* customerPtr;
    reservation_t* reservationPtr;
    bool_t rv;

    customerPtr = (customer_t*)HTMMAP_FIND(customerTablePtr, customerId);
    if (customerPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    reservationPtr = (reservation_t*)HTMMAP_FIND(tablePtr, id);
    if (reservationPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    if (!HTMRESERVATION_MAKE(reservationPtr)) {
        rv = FALSE;
        goto out;
    }

    if (!HTMCUSTOMER_ADD_RESERVATION_INFO(
            customerPtr,
            type,
            id,
            (long)HTM_SHARED_READ(reservationPtr->price)))
    {
        /* Undo previous successful reservation */
        rv = HTMRESERVATION_CANCEL(reservationPtr);
        if (rv == FALSE) {
            HTM_RESTART();
        }
        rv = FALSE;
        goto out;
    }

    rv = TRUE;
out:
    return rv;
}

static bool_t
reserve_seq (
         MAP_T* tablePtr, MAP_T* customerTablePtr,
         long customerId, long id, reservation_type_t type)
{
    customer_t* customerPtr;
    reservation_t* reservationPtr;
    bool_t rv;

    customerPtr = (customer_t*)MAP_FIND(customerTablePtr, customerId);
    if (customerPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    reservationPtr = (reservation_t*)MAP_FIND(tablePtr, id);
    if (reservationPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    if (!RESERVATION_MAKE_SEQ(reservationPtr)) {
        rv = FALSE;
        goto out;
    }

    if (!CUSTOMER_ADD_RESERVATION_INFO_SEQ(
            customerPtr,
            type,
            id,
            reservationPtr->price))
    {
        /* Undo previous successful reservation */
        rv = RESERVATION_CANCEL_SEQ(reservationPtr);
        assert (rv == FALSE);
        rv = FALSE;
        goto out;
    }

    rv = TRUE;
out:
    return rv;
}


/* =============================================================================
 * manager_reserveCar
 * -- Returns failure if the car or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_reserveCar (TM_ARGDECL  manager_t* managerPtr, long customerId, long carId)
{
    return reserve(TM_ARG
                   managerPtr->carTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   carId,
                   RESERVATION_CAR);
}

bool_t
HTMmanager_reserveCar (manager_t* managerPtr, long customerId, long carId)
{
    return HTMreserve(managerPtr->carTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   carId,
                   RESERVATION_CAR);
}

bool_t
manager_reserveCar_seq (manager_t* managerPtr, long customerId, long carId)
{
    return reserve_seq(
                   managerPtr->carTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   carId,
                   RESERVATION_CAR);
}


/* =============================================================================
 * manager_reserveRoom
 * -- Returns failure if the room or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_reserveRoom (TM_ARGDECL  manager_t* managerPtr, long customerId, long roomId)
{
    return reserve(TM_ARG
                   managerPtr->roomTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   roomId,
                   RESERVATION_ROOM);
}

bool_t
HTMmanager_reserveRoom (manager_t* managerPtr, long customerId, long roomId)
{
    return HTMreserve(managerPtr->roomTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   roomId,
                   RESERVATION_ROOM);
}

bool_t
manager_reserveRoom_seq (manager_t* managerPtr, long customerId, long roomId)
{
    return reserve_seq(
                   managerPtr->roomTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   roomId,
                   RESERVATION_ROOM);
}
/* =============================================================================
 * manager_reserveFlight
 * -- Returns failure if the flight or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_reserveFlight (TM_ARGDECL
                       manager_t* managerPtr, long customerId, long flightId)
{
    return reserve(TM_ARG
                   managerPtr->flightTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   flightId,
                   RESERVATION_FLIGHT);
}

bool_t
HTMmanager_reserveFlight (manager_t* managerPtr, long customerId, long flightId)
{
    return HTMreserve(managerPtr->flightTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   flightId,
                   RESERVATION_FLIGHT);
}

bool_t
manager_reserveFlight_seq (manager_t* managerPtr, long customerId, long flightId)
{
    return reserve_seq(
                   managerPtr->flightTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   flightId,
                   RESERVATION_FLIGHT);
}


/* =============================================================================
 * cancel
 * -- Customer is not allowed to cancel multiple times
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
static bool_t
cancel (TM_ARGDECL
        MAP_T* tablePtr, MAP_T* customerTablePtr,
        long customerId, long id, reservation_type_t type)
{
    customer_t* customerPtr;
    reservation_t* reservationPtr;
    bool_t rv;

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_BEGIN(MGR_CANCEL, NULL, tablePtr, customerTablePtr, customerId, id, type);
#endif /* !ORIGINAL && MERGE_MANAGER */
    customerPtr = (customer_t*)TMMAP_FIND(customerTablePtr, customerId);
    if (customerPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    reservationPtr = (reservation_t*)TMMAP_FIND(tablePtr, id);
    if (reservationPtr == NULL) {
        rv = FALSE;
        goto out;
    }

    if (!RESERVATION_CANCEL(reservationPtr)) {
        rv = FALSE;
        goto out;
    }

    if (!CUSTOMER_REMOVE_RESERVATION_INFO(customerPtr, type, id)) {
        /* Undo previous successful cancellation */
        rv = RESERVATION_MAKE(reservationPtr);
        if (rv == FALSE) {
            TM_RESTART();
        }
        rv = FALSE;
        goto out;
    }

    rv = TRUE;
out:
#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
    TM_LOG_END(MGR_CANCEL, &rv);
#endif /* !ORIGINAL && MERGE_MANAGER */
    return rv;
}


/* =============================================================================
 * manager_cancelCar
 * -- Returns failure if the car, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_cancelCar (TM_ARGDECL  manager_t* managerPtr, long customerId, long carId)
{
    return cancel(TM_ARG
                  managerPtr->carTablePtr,
                  managerPtr->customerTablePtr,
                  customerId,
                  carId,
                  RESERVATION_CAR);
}


/* =============================================================================
 * manager_cancelRoom
 * -- Returns failure if the room, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_cancelRoom (TM_ARGDECL  manager_t* managerPtr, long customerId, long roomId)
{
    return cancel(TM_ARG
                  managerPtr->roomTablePtr,
                  managerPtr->customerTablePtr,
                  customerId,
                  roomId,
                  RESERVATION_ROOM);
}



/* =============================================================================
 * manager_cancelFlight
 * -- Returns failure if the flight, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_cancelFlight (TM_ARGDECL
                      manager_t* managerPtr, long customerId, long flightId)
{
    return cancel(TM_ARG
                  managerPtr->flightTablePtr,
                  managerPtr->customerTablePtr,
                  customerId,
                  flightId,
                  RESERVATION_FLIGHT);
}


/* =============================================================================
 * TEST_MANAGER
 * =============================================================================
 */
#ifdef TEST_MANAGER


#include <assert.h>
#include <stdio.h>


int
main ()
{
    manager_t* managerPtr;

    assert(memory_init(1, 4, 2));

    puts("Starting...");

    managerPtr = manager_alloc();

    /* Test administrative interface for cars */
    assert(!manager_addCar(managerPtr, 0, -1, 0)); /* negative num */
    assert(!manager_addCar(managerPtr, 0, 0, -1)); /* negative price */
    assert(!manager_addCar(managerPtr, 0, 0, 0)); /* zero num */
    assert(manager_addCar(managerPtr, 0, 1, 1));
    assert(!manager_deleteCar(managerPtr, 1, 0)); /* does not exist */
    assert(!manager_deleteCar(managerPtr, 0, 2)); /* cannot remove that many */
    assert(manager_addCar(managerPtr, 0, 1, 0));
    assert(manager_deleteCar(managerPtr, 0, 1));
    assert(manager_deleteCar(managerPtr, 0, 1));
    assert(!manager_deleteCar(managerPtr, 0, 1)); /* none left */
    assert(manager_queryCar(managerPtr, 0) == -1); /* does not exist */

    /* Test administrative interface for rooms */
    assert(!manager_addRoom(managerPtr, 0, -1, 0)); /* negative num */
    assert(!manager_addRoom(managerPtr, 0, 0, -1)); /* negative price */
    assert(!manager_addRoom(managerPtr, 0, 0, 0)); /* zero num */
    assert(manager_addRoom(managerPtr, 0, 1, 1));
    assert(!manager_deleteRoom(managerPtr, 1, 0)); /* does not exist */
    assert(!manager_deleteRoom(managerPtr, 0, 2)); /* cannot remove that many */
    assert(manager_addRoom(managerPtr, 0, 1, 0));
    assert(manager_deleteRoom(managerPtr, 0, 1));
    assert(manager_deleteRoom(managerPtr, 0, 1));
    assert(!manager_deleteRoom(managerPtr, 0, 1)); /* none left */
    assert(manager_queryRoom(managerPtr, 0) ==  -1); /* does not exist */

    /* Test administrative interface for flights and customers */
    assert(!manager_addFlight(managerPtr, 0, -1, 0));  /* negative num */
    assert(!manager_addFlight(managerPtr, 0, 0, -1));  /* negative price */
    assert(!manager_addFlight(managerPtr, 0, 0, 0));
    assert(manager_addFlight(managerPtr, 0, 1, 0));
    assert(!manager_deleteFlight(managerPtr, 1)); /* does not exist */
    assert(!manager_deleteFlight(managerPtr, 2)); /* cannot remove that many */
    assert(!manager_cancelFlight(managerPtr, 0, 0)); /* do not have reservation */
    assert(!manager_reserveFlight(managerPtr, 0, 0)); /* customer does not exist */
    assert(!manager_deleteCustomer(managerPtr, 0)); /* does not exist */
    assert(manager_addCustomer(managerPtr, 0));
    assert(!manager_addCustomer(managerPtr, 0)); /* already exists */
    assert(manager_reserveFlight(managerPtr, 0, 0));
    assert(manager_addFlight(managerPtr, 0, 1, 0));
    assert(!manager_deleteFlight(managerPtr, 0)); /* someone has reservation */
    assert(manager_cancelFlight(managerPtr, 0, 0));
    assert(manager_deleteFlight(managerPtr, 0));
    assert(!manager_deleteFlight(managerPtr, 0)); /* does not exist */
    assert(manager_queryFlight(managerPtr, 0) ==  -1); /* does not exist */
    assert(manager_deleteCustomer(managerPtr, 0));

    /* Test query interface for cars */
    assert(manager_addCustomer(managerPtr, 0));
    assert(manager_queryCar(managerPtr, 0) == -1); /* does not exist */
    assert(manager_queryCarPrice(managerPtr, 0) == -1); /* does not exist */
    assert(manager_addCar(managerPtr, 0, 1, 2));
    assert(manager_queryCar(managerPtr, 0) == 1);
    assert(manager_queryCarPrice(managerPtr, 0) == 2);
    assert(manager_addCar(managerPtr, 0, 1, -1));
    assert(manager_queryCar(managerPtr, 0) == 2);
    assert(manager_reserveCar(managerPtr, 0, 0));
    assert(manager_queryCar(managerPtr, 0) == 1);
    assert(manager_deleteCar(managerPtr, 0, 1));
    assert(manager_queryCar(managerPtr, 0) == 0);
    assert(manager_queryCarPrice(managerPtr, 0) == 2);
    assert(manager_addCar(managerPtr, 0, 1, 1));
    assert(manager_queryCarPrice(managerPtr, 0) == 1);
    assert(manager_deleteCustomer(managerPtr, 0));
    assert(manager_queryCar(managerPtr, 0) == 2);
    assert(manager_deleteCar(managerPtr, 0, 2));

    /* Test query interface for rooms */
    assert(manager_addCustomer(managerPtr, 0));
    assert(manager_queryRoom(managerPtr, 0) == -1); /* does not exist */
    assert(manager_queryRoomPrice(managerPtr, 0) == -1); /* does not exist */
    assert(manager_addRoom(managerPtr, 0, 1, 2));
    assert(manager_queryRoom(managerPtr, 0) == 1);
    assert(manager_queryRoomPrice(managerPtr, 0) == 2);
    assert(manager_addRoom(managerPtr, 0, 1, -1));
    assert(manager_queryRoom(managerPtr, 0) == 2);
    assert(manager_reserveRoom(managerPtr, 0, 0));
    assert(manager_queryRoom(managerPtr, 0) == 1);
    assert(manager_deleteRoom(managerPtr, 0, 1));
    assert(manager_queryRoom(managerPtr, 0) == 0);
    assert(manager_queryRoomPrice(managerPtr, 0) == 2);
    assert(manager_addRoom(managerPtr, 0, 1, 1));
    assert(manager_queryRoomPrice(managerPtr, 0) == 1);
    assert(manager_deleteCustomer(managerPtr, 0));
    assert(manager_queryRoom(managerPtr, 0) == 2);
    assert(manager_deleteRoom(managerPtr, 0, 2));

    /* Test query interface for flights */
    assert(manager_addCustomer(managerPtr, 0));
    assert(manager_queryFlight(managerPtr, 0) == -1); /* does not exist */
    assert(manager_queryFlightPrice(managerPtr, 0) == -1); /* does not exist */
    assert(manager_addFlight(managerPtr, 0, 1, 2));
    assert(manager_queryFlight(managerPtr, 0) == 1);
    assert(manager_queryFlightPrice(managerPtr, 0) == 2);
    assert(manager_addFlight(managerPtr, 0, 1, -1));
    assert(manager_queryFlight(managerPtr, 0) == 2);
    assert(manager_reserveFlight(managerPtr, 0, 0));
    assert(manager_queryFlight(managerPtr, 0) == 1);
    assert(manager_addFlight(managerPtr, 0, 1, 1));
    assert(manager_queryFlightPrice(managerPtr, 0) == 1);
    assert(manager_deleteCustomer(managerPtr, 0));
    assert(manager_queryFlight(managerPtr, 0) == 3);
    assert(manager_deleteFlight(managerPtr, 0));

    /* Test query interface for customer bill */

    assert(manager_queryCustomerBill(managerPtr, 0) == -1); /* does not exist */
    assert(manager_addCustomer(managerPtr, 0));
    assert(manager_queryCustomerBill(managerPtr, 0) == 0);
    assert(manager_addCar(managerPtr, 0, 1, 1));
    assert(manager_addRoom(managerPtr, 0, 1, 2));
    assert(manager_addFlight(managerPtr, 0, 1, 3));

    assert(manager_reserveCar(managerPtr, 0, 0));
    assert(manager_queryCustomerBill(managerPtr, 0) == 1);
    assert(!manager_reserveCar(managerPtr, 0, 0));
    assert(manager_queryCustomerBill(managerPtr, 0) == 1);
    assert(manager_addCar(managerPtr, 0, 0, 2));
    assert(manager_queryCar(managerPtr, 0) == 0);
    assert(manager_queryCustomerBill(managerPtr, 0) == 1);

    assert(manager_reserveRoom(managerPtr, 0, 0));
    assert(manager_queryCustomerBill(managerPtr, 0) == 3);
    assert(!manager_reserveRoom(managerPtr, 0, 0));
    assert(manager_queryCustomerBill(managerPtr, 0) == 3);
    assert(manager_addRoom(managerPtr, 0, 0, 2));
    assert(manager_queryRoom(managerPtr, 0) == 0);
    assert(manager_queryCustomerBill(managerPtr, 0) == 3);

    assert(manager_reserveFlight(managerPtr, 0, 0));
    assert(manager_queryCustomerBill(managerPtr, 0) == 6);
    assert(!manager_reserveFlight(managerPtr, 0, 0));
    assert(manager_queryCustomerBill(managerPtr, 0) == 6);
    assert(manager_addFlight(managerPtr, 0, 0, 2));
    assert(manager_queryFlight(managerPtr, 0) == 0);
    assert(manager_queryCustomerBill(managerPtr, 0) == 6);

    assert(manager_deleteCustomer(managerPtr, 0));
    assert(manager_deleteCar(managerPtr, 0, 1));
    assert(manager_deleteRoom(managerPtr, 0, 1));
    assert(manager_deleteFlight(managerPtr, 0));

   /* Test reservation interface */

    assert(manager_addCustomer(managerPtr, 0));
    assert(manager_queryCustomerBill(managerPtr, 0) == 0);
    assert(manager_addCar(managerPtr, 0, 1, 1));
    assert(manager_addRoom(managerPtr, 0, 1, 2));
    assert(manager_addFlight(managerPtr, 0, 1, 3));

    assert(!manager_cancelCar(managerPtr, 0, 0)); /* do not have reservation */
    assert(manager_reserveCar(managerPtr, 0, 0));
    assert(manager_queryCar(managerPtr, 0) == 0);
    assert(manager_cancelCar(managerPtr, 0, 0));
    assert(manager_queryCar(managerPtr, 0) == 1);

    assert(!manager_cancelRoom(managerPtr, 0, 0)); /* do not have reservation */
    assert(manager_reserveRoom(managerPtr, 0, 0));
    assert(manager_queryRoom(managerPtr, 0) == 0);
    assert(manager_cancelRoom(managerPtr, 0, 0));
    assert(manager_queryRoom(managerPtr, 0) == 1);

    assert(!manager_cancelFlight(managerPtr, 0, 0)); /* do not have reservation */
    assert(manager_reserveFlight(managerPtr, 0, 0));
    assert(manager_queryFlight(managerPtr, 0) == 0);
    assert(manager_cancelFlight(managerPtr, 0, 0));
    assert(manager_queryFlight(managerPtr, 0) == 1);

    assert(manager_deleteCar(managerPtr, 0, 1));
    assert(manager_deleteRoom(managerPtr, 0, 1));
    assert(manager_deleteFlight(managerPtr, 0));
    assert(manager_deleteCustomer(managerPtr, 0));

    manager_free(managerPtr);

    puts("All tests passed.");

    return 0;
}


#endif /* TEST_MANAGER */


/* =============================================================================
 *
 * End of manager.c
 *
 * =============================================================================
 */

#if !defined(ORIGINAL) && defined(MERGE_MANAGER)
stm_merge_t TMmanager_merge(stm_merge_context_t *params) {
# ifdef MERGE_RBTREE
    extern const stm_op_id_t RBTREE_GET;
# endif /* MERGE_RBTREE */
    stm_op_id_t op = stm_get_op_opcode(params->current);

    const stm_union_t *args;
    const ssize_t nargs = stm_get_op_args(params->current, &args);

    if (STM_SAME_OPID(op, MGR_ADDCUSTOMER) || STM_SAME_OPID(op, MGR_DELCUSTOMER) || STM_SAME_OPID(op, MGR_DELFLIGHT) || STM_SAME_OPID(op, MGR_QUERYFREE) || STM_SAME_OPID(op, MGR_QUERYPRICE)) {
        ASSERT(nargs == 2);
        const MAP_T *tablePtr = args[0].ptr;
        const long id = args[1].sint;

        if (STM_SAME_OPID(op, MGR_ADDCUSTOMER)) {
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);
# ifdef TM_DEBUG
            printf("\nMGR_ADDCUSTOMER addr:%p tablePtr:%p id:%ld\n", params->addr, tablePtr, id);
# endif
        } else if (STM_SAME_OPID(op, MGR_DELCUSTOMER)) {
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);
# ifdef TM_DEBUG
            printf("\nMGR_DELCUSTOMER addr:%p tablePtr:%p id:%ld\n", params->addr, tablePtr, id);
# endif
        } else if (STM_SAME_OPID(op, MGR_DELFLIGHT)) {
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);
# ifdef TM_DEBUG
            printf("\nMGR_DELFLIGHT addr:%p tablePtr:%p id:%ld\n", params->addr, tablePtr, id);
# endif
        } else if (STM_SAME_OPID(op, MGR_QUERYFREE)) {
            long old, new;
            stm_read_t r;
            const reservation_t *reservationPtr;

            if (params->leaf == 1) {
                ASSERT(params->rv.sint >= 0);
                ASSERT(!STM_VALID_OP(params->previous));
# ifdef TM_DEBUG
                printf("\nMGR_QUERYFREE addr:%p tablePtr:%p id:%ld\n", params->addr, tablePtr, id);
# endif
                ASSERT(ENTRY_VALID(params->conflict.entries->e1));
                r = ENTRY_GET_READ(params->conflict.entries->e1);
                ASSERT(STM_VALID_READ(r));
                reservationPtr = (reservation_t *)TM_SHARED_GET_TAG(r);
                ASSERT(reservationPtr);
                ASSERT(STM_SAME_READ(TM_SHARED_DID_READ(reservationPtr->numFree), r));
                ASSERT_FAIL(TM_SHARED_READ_VALUE(r, reservationPtr->numFree, old));
                ASSERT(!((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) || old == params->rv.sint);
                ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, reservationPtr->numFree, new));
# ifdef MERGE_RBTREE
            } else if (STM_SAME_OPID(stm_get_op_opcode(params->previous), RBTREE_GET)) {
#  ifdef TM_DEBUG
                printf("\nMGR_QUERYFREE <- RBTREE_GET %p\n", params->conflict.result.ptr);
#  endif
                reservationPtr = params->conflict.result.ptr;
                /* Get the previous reservation */
                const reservation_t *prev_reservation = params->conflict.previous_result.ptr;
                ASSERT(prev_reservation);

                ASSERT(prev_reservation != reservationPtr);
                if (prev_reservation) {
                    r = TM_SHARED_DID_READ(prev_reservation->numFree);
                    ASSERT(STM_VALID_READ(r));
                    ASSERT_FAIL(TM_SHARED_READ_VALUE(r, prev_reservation->numFree, old));
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                } else
                    old = -1;
                ASSERT(!((stm_get_features() & STM_FEATURE_OPLOG_FULL) == STM_FEATURE_OPLOG_FULL) || old == params->rv.sint);

                if (reservationPtr) {
                    ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(reservationPtr->numFree)));
                    new = (long)TM_SHARED_READ_TAG(reservationPtr->numFree, reservationPtr);
                } else
                    new = -1;
# endif /* MERGE_RBTREE */
            }

# ifdef TM_DEBUG
            printf("MGR_QUERYFREE rv(old):%ld rv(new):%ld\n", old, new);
# endif
            params->conflict.previous_result.sint = old;
            params->rv.sint = new;
            return STM_MERGE_OK;
        } else {
            long old, new;
            stm_read_t r;
            const reservation_t *reservationPtr;

            if (params->leaf == 1) {
                ASSERT(params->rv.sint >= 0);
                ASSERT(!STM_VALID_OP(params->previous));
# ifdef TM_DEBUG
                printf("\nMGR_QUERYPRICE addr:%p tablePtr:%p id:%ld\n", params->addr, tablePtr, id);
# endif
                ASSERT(ENTRY_VALID(params->conflict.entries->e1));
                r = ENTRY_GET_READ(params->conflict.entries->e1);
                ASSERT(STM_VALID_READ(r));
                reservationPtr = (reservation_t *)TM_SHARED_GET_TAG(r);
                ASSERT(reservationPtr);
                ASSERT(STM_SAME_READ(TM_SHARED_DID_READ(reservationPtr->price), r));
                ASSERT_FAIL(TM_SHARED_READ_VALUE(r, reservationPtr->price, old));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, reservationPtr->price, new));
#ifdef MERGE_RBTREE
            } else if (STM_SAME_OPID(stm_get_op_opcode(params->previous), RBTREE_GET)) {
# ifdef TM_DEBUG
                printf("\nMGR_QUERYPRICE <- RBTREE_GET %p\n", params->conflict.result.ptr);
# endif
                reservationPtr = params->conflict.result.ptr;
                /* Get the previous reservation */
                const reservation_t *prev_reservation = params->conflict.previous_result.ptr;
                ASSERT(prev_reservation);

                ASSERT(prev_reservation != reservationPtr);
                if (prev_reservation) {
                    r = TM_SHARED_DID_READ(prev_reservation->price);
                    ASSERT(STM_VALID_READ(r));
                    ASSERT_FAIL(TM_SHARED_READ_VALUE(r, prev_reservation->price, old));
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                } else
                    old = -1;

                if (reservationPtr) {
                    ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(reservationPtr->price)));
                    new = (long)TM_SHARED_READ_TAG(reservationPtr->price, reservationPtr);
                } else
                    new = -1;
#endif /* MERGE_RBTREE */
            }

# ifdef TM_DEBUG
            printf("MGR_QUERYPRICE rv(old):%ld rv(new):%ld\n", params->rv.sint, new);
# endif
            params->conflict.previous_result.sint = old;
            params->rv.sint = new;
            return STM_MERGE_OK;
        }
    } else if (STM_SAME_OPID(op, MGR_ADDRESERVE)) {
        ASSERT(nargs == 4);
        const MAP_T *tablePtr = args[0].ptr;
        const long id = args[1].sint;
        const long num = args[2].sint;
        const long price = args[3].sint;

# ifdef TM_DEBUG
        printf("\nMGR_ADDRESERVE addr:%p tablePtr:%p id:%ld num:%ld price:%ld\n", params->addr, tablePtr, id, num, price);
# endif
    } else if (STM_SAME_OPID(op, MGR_RESERVE) || STM_SAME_OPID(op, MGR_CANCEL)) {
        ASSERT(nargs == 5);
        const MAP_T *tablePtr = args[0].ptr;
        ASSERT(tablePtr);
        const MAP_T *customerTablePtr = args[1].ptr;
        const long customerId = args[2].sint;
        const long id = args[3].sint;
        const reservation_type_t type = args[4].sint;

        if (STM_SAME_OPID(op, MGR_RESERVE)) {
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);
            if (params->leaf == 1) {
# ifdef TM_DEBUG
                printf("\nMGR_RESERVE addr:%p tablePtr:%p customerTablePtr:%p customerId:%ld id:%ld type:%d\n", params->addr, tablePtr, customerTablePtr, customerId, id, type);
# endif
            } else {
# ifdef MERGE_LIST
                extern const stm_op_id_t LIST_INSERT;
                const stm_op_id_t prev = stm_get_op_opcode(params->previous);

                if (STM_SAME_OPID(prev, LIST_INSERT)) {
                    const stm_union_t *prev_args;
                    const ssize_t prev_nargs = stm_get_op_args(params->previous, &prev_args);
                    ASSERT(prev_nargs == 2);
                    const list_t *listPtr = prev_args[0].ptr;
                    ASSERT(listPtr);
#  ifdef TM_DEBUG
                    printf("\nMGR_RESERVE <- LIST_INSERT %ld\n", params->conflict.result.sint);
#  endif
                    const customer_t *customerPtr = TMMAP_FIND((MAP_T *)customerTablePtr, customerId);
                    const stm_read_t r = TM_SHARED_DID_READ(customerPtr->reservationInfoListPtr);
                    const list_t *reservationInfoListPtr;
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, customerPtr->reservationInfoListPtr, reservationInfoListPtr));
                    if (STM_VALID_READ(r) && reservationInfoListPtr == customerPtr->reservationInfoListPtr) {
                        ASSERT(params->rv.sint == TRUE);
                        reservation_t *reservationPtr = TMMAP_FIND((MAP_T *)tablePtr, id);
                        ASSERT(reservationPtr);
                        if (__builtin_expect(RESERVATION_CANCEL(reservationPtr) != TRUE, 0))
                            return STM_MERGE_ABORT;
                        params->rv.sint = FALSE;
                        return STM_MERGE_OK;
                    }
                }
# endif /* MERGE_LIST */
            }
        } else {
            ASSERT(params->rv.sint == TRUE || params->rv.sint == FALSE);
# ifdef TM_DEBUG
            printf("\nMGR_CANCEL addr:%p tablePtr:%p customerTablePtr:%p customerId:%ld id:%ld type:%d\n", params->addr, tablePtr, customerTablePtr, customerId, id, type);
# endif
        }
    }

# ifdef TM_DEBUG
    printf("\nMGR_MERGE UNSUPPORTED addr:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}

# define TMMANAGER_MERGE TMmanager_merge

__attribute__((constructor)) void manager_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*pplli[], {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_slong, &ffi_type_slong, &ffi_type_sint});
    TM_LOG_TYPE_DECLARE_INIT(*plll[], {&ffi_type_pointer, &ffi_type_slong, &ffi_type_slong, &ffi_type_slong});
    TM_LOG_TYPE_DECLARE_INIT(*pl[], {&ffi_type_pointer, &ffi_type_slong});
    # define TM_LOG_OP TM_LOG_OP_INIT
    # include "manager.inc"
    # undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_MANAGER */
