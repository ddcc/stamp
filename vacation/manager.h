/* =============================================================================
 *
 * manager.h
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


#ifndef MANAGER_H
#define MANAGER_H 1


#include "map.h"
#include "tm.h"
#include "types.h"

typedef struct manager {
    MAP_T* carTablePtr;
    MAP_T* roomTablePtr;
    MAP_T* flightTablePtr;
    MAP_T* customerTablePtr;
} manager_t;


/* =============================================================================
 * manager_alloc
 * =============================================================================
 */
manager_t*
manager_alloc ();


/* =============================================================================
 * manager_free
 * =============================================================================
 */
void
manager_free (manager_t* managerPtr);


/* =============================================================================
 * ADMINISTRATIVE INTERFACE
 * =============================================================================
 */


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
                manager_t* managerPtr, long carId, long numCar, long price);

bool_t
HTMmanager_addCar (manager_t* managerPtr, long carId, long numCar, long price);

bool_t
manager_addCar_seq (manager_t* managerPtr, long carId, long numCar, long price);


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
manager_deleteCar (TM_ARGDECL  manager_t* managerPtr, long carId, long numCar);

bool_t
HTMmanager_deleteCar (manager_t* managerPtr, long carId, long numCar);

bool_t
manager_deleteCar_seq (manager_t* managerPtr, long carId, long numCar);


/* =============================================================================
 * manager_addRoom
 * -- Add rooms to a city
 * -- Adding to an existing room overwrites the price if 'price' >= 0
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_addRoom (TM_ARGDECL
                 manager_t* managerPtr, long roomId, long numRoom, long price);

bool_t
HTMmanager_addRoom (manager_t* managerPtr, long roomId, long numRoom, long price);

bool_t
manager_addRoom_seq (manager_t* managerPtr, long roomId, long numRoom, long price);


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
manager_deleteRoom (TM_ARGDECL  manager_t* managerPtr, long roomId, long numRoom);

bool_t
HTMmanager_deleteRoom (manager_t* managerPtr, long roomId, long numRoom);

bool_t
manager_deleteRoom_seq (manager_t* managerPtr, long roomId, long numRoom);


/* =============================================================================
 * manager_addFlight
 * -- Add seats to a flight
 * -- Adding to an existing flight overwrites the price if 'price' >= 0
 * -- Returns TRUE on success, FALSE on failure
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_addFlight (TM_ARGDECL
                   manager_t* managerPtr, long flightId, long numSeat, long price);

bool_t
HTMmanager_addFlight (manager_t* managerPtr, long flightId, long numSeat, long price);

bool_t
manager_addFlight_seq (manager_t* managerPtr, long flightId, long numSeat, long price);


/* =============================================================================
 * manager_deleteFlight
 * -- Delete an entire flight
 * -- Fails if customer has reservation on this flight
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_deleteFlight (TM_ARGDECL  manager_t* managerPtr, long flightId);

bool_t
HTMmanager_deleteFlight (manager_t* managerPtr, long flightId);

bool_t
manager_deleteFlight_seq (manager_t* managerPtr, long flightId);


/* =============================================================================
 * manager_addCustomer
 * -- If customer already exists, returns success
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_addCustomer (TM_ARGDECL  manager_t* managerPtr, long customerId);

bool_t
HTMmanager_addCustomer (manager_t* managerPtr, long customerId);

bool_t
manager_addCustomer_seq (manager_t* managerPtr, long customerId);


/* =============================================================================
 * manager_deleteCustomer
 * -- Delete this customer and associated reservations
 * -- If customer does not exist, returns success
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_deleteCustomer (TM_ARGDECL  manager_t* managerPtr, long customerId);

bool_t
HTMmanager_deleteCustomer (manager_t* managerPtr, long customerId);

bool_t
manager_deleteCustomer_seq (manager_t* managerPtr, long customerId);


/* =============================================================================
 * QUERY INTERFACE
 * =============================================================================
 */


/* =============================================================================
 * manager_queryCar
 * -- Return the number of empty seats on a car
 * -- Returns -1 if the car does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryCar (TM_ARGDECL  manager_t* managerPtr, long carId);

long
HTMmanager_queryCar (manager_t* managerPtr, long carId);

long
manager_queryCar_seq (manager_t* managerPtr, long carId);


/* =============================================================================
 * manager_queryCarPrice
 * -- Return the price of the car
 * -- Returns -1 if the car does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryCarPrice (TM_ARGDECL  manager_t* managerPtr, long carId);

long
HTMmanager_queryCarPrice (manager_t* managerPtr, long carId);

long
manager_queryCarPrice_seq (manager_t* managerPtr, long carId);


/* =============================================================================
 * manager_queryRoom
 * -- Return the number of empty seats on a room
 * -- Returns -1 if the room does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryRoom (TM_ARGDECL  manager_t* managerPtr, long roomId);

long
HTMmanager_queryRoom (manager_t* managerPtr, long roomId);

long
manager_queryRoom_seq (manager_t* managerPtr, long roomId);


/* =============================================================================
 * manager_queryRoomPrice
 * -- Return the price of the room
 * -- Returns -1 if the room does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryRoomPrice (TM_ARGDECL  manager_t* managerPtr, long roomId);

long
HTMmanager_queryRoomPrice (manager_t* managerPtr, long roomId);

long
manager_queryRoomPrice_seq (manager_t* managerPtr, long roomId);


/* =============================================================================
 * manager_queryFlight
 * -- Return the number of empty seats on a flight
 * -- Returns -1 if the flight does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryFlight (TM_ARGDECL  manager_t* managerPtr, long flightId);

long
HTMmanager_queryFlight (manager_t* managerPtr, long flightId);

long
manager_queryFlight_seq (manager_t* managerPtr, long flightId);


/* =============================================================================
 * manager_queryFlightPrice
 * -- Return the price of the flight
 * -- Returns -1 if the flight does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryFlightPrice (TM_ARGDECL  manager_t* managerPtr, long flightId);

long
HTMmanager_queryFlightPrice (manager_t* managerPtr, long flightId);

long
manager_queryFlightPrice_seq (manager_t* managerPtr, long flightId);


/* =============================================================================
 * manager_queryCustomerBill
 * -- Return the total price of all reservations held for a customer
 * -- Returns -1 if the customer does not exist
 * =============================================================================
 */
TM_CALLABLE
long
manager_queryCustomerBill (TM_ARGDECL  manager_t* managerPtr, long customerId);

long
HTMmanager_queryCustomerBill (manager_t* managerPtr, long customerId);

long
manager_queryCustomerBill_seq (manager_t* managerPtr, long customerId);


/* =============================================================================
 * RESERVATION INTERFACE
 * =============================================================================
 */


/* =============================================================================
 * manager_reserveCar
 * -- Returns failure if the car or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_reserveCar (TM_ARGDECL
                    manager_t* managerPtr, long customerId, long carId);

bool_t
HTMmanager_reserveCar (manager_t* managerPtr, long customerId, long carId);

bool_t
manager_reserveCar_seq (manager_t* managerPtr, long customerId, long carId);


/* =============================================================================
 * manager_reserveRoom
 * -- Returns failure if the room or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_reserveRoom (TM_ARGDECL
                     manager_t* managerPtr, long customerId, long roomId);

bool_t
HTMmanager_reserveRoom (manager_t* managerPtr, long customerId, long roomId);

bool_t
manager_reserveRoom_seq (manager_t* managerPtr, long customerId, long roomId);


/* =============================================================================
 * manager_reserveFlight
 * -- Returns failure if the flight or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_reserveFlight (TM_ARGDECL
                       manager_t* managerPtr, long customerId, long flightId);

bool_t
HTMmanager_reserveFlight (manager_t* managerPtr, long customerId, long flightId);

bool_t
manager_reserveFlight_seq (manager_t* managerPtr, long customerId, long flightId);


/* =============================================================================
 * manager_cancelCar
 * -- Returns failure if the car, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_cancelCar (TM_ARGDECL
                   manager_t* managerPtr, long customerId, long carId);


/* =============================================================================
 * manager_cancelRoom
 * -- Returns failure if the room, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_cancelRoom (TM_ARGDECL
                    manager_t* managerPtr, long customerId, long roomId);


/* =============================================================================
 * manager_cancelFlight
 * -- Returns failure if the flight, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
TM_CANCELLABLE
bool_t
manager_cancelFlight (TM_ARGDECL
                      manager_t* managerPtr, long customerId, long flightId);


#define MANAGER_ADD_CAR_SEQ(mgr, id, num, price) \
    manager_addCar_seq(mgr, id, num, price)
#define MANAGER_DELETE_CAR_SEQ(mgr, id, num) \
    manager_deleteCar_seq(mgr, id, num)
#define MANAGER_ADD_ROOM_SEQ(mgr, id, num, price) \
    manager_addRoom_seq(mgr, id, num, price)
#define MANAGER_DELETE_ROOM_SEQ(mgr, id, num) \
    manager_deleteRoom_seq(mgr, id, num)
#define MANAGER_ADD_FLIGHT_SEQ(mgr, id, num, price) \
    manager_addFlight_seq(mgr, id, num, price)
#define MANAGER_DELETE_FLIGHT_SEQ(mgr, id) \
    manager_deleteFlight_seq(mgr, id)
#define MANAGER_ADD_CUSTOMER_SEQ(mgr, id) \
    manager_addCustomer_seq(mgr, id)
#define MANAGER_DELETE_CUSTOMER_SEQ(mgr, id) \
    manager_deleteCustomer_seq(mgr, id)
#define MANAGER_QUERY_CAR_SEQ(mgr, id) \
    manager_queryCar_seq(mgr, id)
#define MANAGER_QUERY_CAR_PRICE_SEQ(mgr, id) \
    manager_queryCarPrice_seq(mgr, id)
#define MANAGER_QUERY_ROOM_SEQ(mgr, id) \
    manager_queryRoom_seq(mgr, id)
#define MANAGER_QUERY_ROOM_PRICE_SEQ(mgr, id) \
    manager_queryRoomPrice_seq(mgr, id)
#define MANAGER_QUERY_FLIGHT_SEQ(mgr, id) \
    manager_queryFlight_seq(mgr, id)
#define MANAGER_QUERY_FLIGHT_PRICE_SEQ(mgr, id) \
    manager_queryFlightPrice_seq(mgr, id)
#define MANAGER_QUERY_CUSTOMER_BILL_SEQ(mgr, id) \
    manager_queryCustomerBill_seq(mgr, id)
#define MANAGER_RESERVE_CAR_SEQ(mgr, cust, id) \
    manager_reserveCar_seq(mgr, cust, id)
#define MANAGER_RESERVE_ROOM_SEQ(mgr, cust, id) \
    manager_reserveRoom_seq(mgr, cust, id)
#define MANAGER_RESERVE_FLIGHT_SEQ(mgr, cust, id) \
    manager_reserveFlight_seq(mgr, cust, id)
#define MANAGER_CANCEL_CAR_SEQ(mgr, cust, id) \
    manager_cancelCar_seq(mgr, cust, id)
#define MANAGER_CANCEL_ROOM_SEQ(mgr, cust, id) \
    manager_cancelRoom_seq(mgr, cust, id)
#define MANAGER_CANCEL_FLIGHT_SEQ(mgr, cust, id) \
    manager_cancelFlight_seq(mgr, cust, id)

#define HTMMANAGER_ADD_CAR(mgr, id, num, price) \
    HTMmanager_addCar(mgr, id, num, price)
#define HTMMANAGER_DELETE_CAR(mgr, id, num) \
    HTMmanager_deleteCar(mgr, id, num)
#define HTMMANAGER_ADD_ROOM(mgr, id, num, price) \
    HTMmanager_addRoom(mgr, id, num, price)
#define HTMMANAGER_DELETE_ROOM(mgr, id, num) \
    HTMmanager_deleteRoom(mgr, id, num)
#define HTMMANAGER_ADD_FLIGHT(mgr, id, num, price) \
    HTMmanager_addFlight(mgr, id, num, price)
#define HTMMANAGER_DELETE_FLIGHT(mgr, id) \
    HTMmanager_deleteFlight(mgr, id)
#define HTMMANAGER_ADD_CUSTOMER(mgr, id) \
    HTMmanager_addCustomer(mgr, id)
#define HTMMANAGER_DELETE_CUSTOMER(mgr, id) \
    HTMmanager_deleteCustomer(mgr, id)
#define HTMMANAGER_QUERY_CAR(mgr, id) \
    HTMmanager_queryCar(mgr, id)
#define HTMMANAGER_QUERY_CAR_PRICE(mgr, id) \
    HTMmanager_queryCarPrice(mgr, id)
#define HTMMANAGER_QUERY_ROOM(mgr, id) \
    HTMmanager_queryRoom(mgr, id)
#define HTMMANAGER_QUERY_ROOM_PRICE(mgr, id) \
    HTMmanager_queryRoomPrice(mgr, id)
#define HTMMANAGER_QUERY_FLIGHT(mgr, id) \
    HTMmanager_queryFlight(mgr, id)
#define HTMMANAGER_QUERY_FLIGHT_PRICE(mgr, id) \
    HTMmanager_queryFlightPrice(mgr, id)
#define HTMMANAGER_QUERY_CUSTOMER_BILL(mgr, id) \
    HTMmanager_queryCustomerBill(mgr, id)
#define HTMMANAGER_RESERVE_CAR(mgr, cust, id) \
    HTMmanager_reserveCar(mgr, cust, id)
#define HTMMANAGER_RESERVE_ROOM(mgr, cust, id) \
    HTMmanager_reserveRoom(mgr, cust, id)
#define HTMMANAGER_RESERVE_FLIGHT(mgr, cust, id) \
    HTMmanager_reserveFlight(mgr, cust, id)
#define HTMMANAGER_CANCEL_CAR(mgr, cust, id) \
    HTMmanager_cancelCar(mgr, cust, id)
#define HTMMANAGER_CANCEL_ROOM(mgr, cust, id) \
    HTMmanager_cancelRoom(mgr, cust, id)
#define HTMMANAGER_CANCEL_FLIGHT(mgr, cust, id) \
    HTMmanager_cancelFlight(mgr, cust, id)

#define MANAGER_ADD_CAR(mgr, id, num, price) \
    manager_addCar(TM_ARG  mgr, id, num, price)
#define MANAGER_DELETE_CAR(mgr, id, num) \
    manager_deleteCar(TM_ARG  mgr, id, num)
#define MANAGER_ADD_ROOM(mgr, id, num, price) \
    manager_addRoom(TM_ARG  mgr, id, num, price)
#define MANAGER_DELETE_ROOM(mgr, id, num) \
    manager_deleteRoom(TM_ARG  mgr, id, num)
#define MANAGER_ADD_FLIGHT(mgr, id, num, price) \
    manager_addFlight(TM_ARG  mgr, id, num, price)
#define MANAGER_DELETE_FLIGHT(mgr, id) \
    manager_deleteFlight(TM_ARG  mgr, id)
#define MANAGER_ADD_CUSTOMER(mgr, id) \
    manager_addCustomer(TM_ARG  mgr, id)
#define MANAGER_DELETE_CUSTOMER(mgr, id) \
    manager_deleteCustomer(TM_ARG  mgr, id)
#define MANAGER_QUERY_CAR(mgr, id) \
    manager_queryCar(TM_ARG  mgr, id)
#define MANAGER_QUERY_CAR_PRICE(mgr, id) \
    manager_queryCarPrice(TM_ARG  mgr, id)
#define MANAGER_QUERY_ROOM(mgr, id) \
    manager_queryRoom(TM_ARG  mgr, id)
#define MANAGER_QUERY_ROOM_PRICE(mgr, id) \
    manager_queryRoomPrice(TM_ARG  mgr, id)
#define MANAGER_QUERY_FLIGHT(mgr, id) \
    manager_queryFlight(TM_ARG  mgr, id)
#define MANAGER_QUERY_FLIGHT_PRICE(mgr, id) \
    manager_queryFlightPrice(TM_ARG  mgr, id)
#define MANAGER_QUERY_CUSTOMER_BILL(mgr, id) \
    manager_queryCustomerBill(TM_ARG  mgr, id)
#define MANAGER_RESERVE_CAR(mgr, cust, id) \
    manager_reserveCar(TM_ARG  mgr, cust, id)
#define MANAGER_RESERVE_ROOM(mgr, cust, id) \
    manager_reserveRoom(TM_ARG  mgr, cust, id)
#define MANAGER_RESERVE_FLIGHT(mgr, cust, id) \
    manager_reserveFlight(TM_ARG  mgr, cust, id)
#define MANAGER_CANCEL_CAR(mgr, cust, id) \
    manager_cancelCar(TM_ARG  mgr, cust, id)
#define MANAGER_CANCEL_ROOM(mgr, cust, id) \
    manager_cancelRoom(TM_ARG  mgr, cust, id)
#define MANAGER_CANCEL_FLIGHT(mgr, cust, id) \
    manager_cancelFlight(TM_ARG  mgr, cust, id)


#endif /* MANAGER_H */


/* =============================================================================
 *
 * End of manager.h
 *
 * =============================================================================
 */
