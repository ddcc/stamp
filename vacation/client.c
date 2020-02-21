/* =============================================================================
 *
 * client.c
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
#include "action.h"
#include "client.h"
#include "manager.h"
#include "reservation.h"
#include "thread.h"
#include "types.h"

#if !defined(ORIGINAL) && defined(MERGE_CLIENT)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "client.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_CLIENT */

TM_INIT_GLOBAL;

HTM_STATS_EXTERN(global_tsx_status);

/* =============================================================================
 * client_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
client_t*
client_alloc (long id,
              manager_t* managerPtr,
              long numOperation,
              long numQueryPerTransaction,
              long queryRange,
              long percentUser)
{
    client_t* clientPtr;

    clientPtr = (client_t*)malloc(sizeof(client_t));
    if (clientPtr == NULL) {
        return NULL;
    }

    clientPtr->randomPtr = random_alloc();
    if (clientPtr->randomPtr == NULL) {
        return NULL;
    }

    clientPtr->id = id;
    clientPtr->managerPtr = managerPtr;
    random_seed(clientPtr->randomPtr, id);
    clientPtr->numOperation = numOperation;
    clientPtr->numQueryPerTransaction = numQueryPerTransaction;
    clientPtr->queryRange = queryRange;
    clientPtr->percentUser = percentUser;

    return clientPtr;
}


/* =============================================================================
 * client_free
 * =============================================================================
 */
void
client_free (client_t* clientPtr)
{
    free(clientPtr->randomPtr);
    free(clientPtr);
}


/* =============================================================================
 * selectAction
 * =============================================================================
 */
static action_t
selectAction (long r, long percentUser)
{
    action_t action;

    if (r < percentUser) {
        action = ACTION_MAKE_RESERVATION;
    } else if (r & 1) {
        action = ACTION_DELETE_CUSTOMER;
    } else {
        action = ACTION_UPDATE_TABLES;
    }

    return action;
}


/* =============================================================================
 * client_run
 * -- Execute list operations on the database
 * =============================================================================
 */

void
client_run (void* argPtr)
{
#if !defined(ORIGINAL) && defined(MERGE_CLIENT)
    stm_merge_t merge(stm_merge_context_t *params) {
        const stm_op_id_t op = stm_get_op_opcode(params->current);

        if (params->leaf)
            return STM_MERGE_UNSUPPORTED;

        if (STM_SAME_OPID(op, CLT_RESERVE)) {
#ifdef MERGE_MANAGER
            extern const stm_op_id_t MGR_ADDCUSTOMER, MGR_RESERVE, MGR_QUERYFREE;
            const stm_op_id_t prev = stm_get_op_opcode(params->previous);

            if (STM_SAME_OPID(prev, MGR_ADDCUSTOMER)) {
# ifdef TM_DEBUG
                printf("\nCLT_RESERVE_JIT <- MGR_ADDCUSTOMER %ld\n", params->conflict.result.sint);
# endif
                /* Return value is unused */
                return STM_MERGE_OK;
            } else if (STM_SAME_OPID(prev, MGR_RESERVE)) {
# ifdef TM_DEBUG
                printf("\nCLT_RESERVE_JIT <- MGR_RESERVE %ld\n", params->conflict.result.sint);
# endif
                /* Return value is unused */
                return STM_MERGE_OK;
            } else if (STM_SAME_OPID(prev, MGR_QUERYFREE)) {
# ifdef TM_DEBUG
                printf("\nCLT_RESERVE_JIT <- MGR_QUERYFREE %ld\n", params->conflict.previous_result.sint);
# endif
                /* Control flow is unchanged */
                if ((params->conflict.previous_result.sint >= 0) == (params->conflict.result.sint >= 0))
                    return STM_MERGE_OK;
            }
#endif /* MERGE_MANAGER */
# ifdef TM_DEBUG
            printf("\nCLT_RESERVE_JIT UNSUPPORTED addr:%p\n", params->addr);
# endif
        } else if (STM_SAME_OPID(op, CLT_UPDATE)) {
#ifdef MERGE_MANAGER
            extern const stm_op_id_t MGR_ADDRESERVE;
            ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->previous), MGR_ADDRESERVE));
# ifdef TM_DEBUG
            printf("\nCLT_UPDATE_JIT <- MGR_ADDRESERVE %ld\n", params->conflict.result.sint);
# endif
            /* Return value is unused */
            return STM_MERGE_OK;
#endif /* MERGE_MANAGER */
        } else if (STM_SAME_OPID(op, CLT_DELCUSTOMER)) {
# ifdef TM_DEBUG
            printf("\nCLT_DELCUSTOMER_JIT addr:%p\n", params->addr);
# endif
        }

# ifdef TM_DEBUG
        printf("\nCLT_MERGE_JIT UNSUPPORTED addr:%p\n", params->addr);
# endif
        return STM_MERGE_UNSUPPORTED;
    }
#endif /* !ORIGINAL && MERGE_CLIENT */

    TM_THREAD_ENTER();

    long myId = thread_getId();
    client_t* clientPtr = ((client_t**)argPtr)[myId];

    manager_t* managerPtr = clientPtr->managerPtr;
    random_t*  randomPtr  = clientPtr->randomPtr;

    long numOperation           = clientPtr->numOperation;
    long numQueryPerTransaction = clientPtr->numQueryPerTransaction;
    long queryRange             = clientPtr->queryRange;
    long percentUser            = clientPtr->percentUser;

    long* types  = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* ids    = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* ops    = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* prices = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));

    long i;

    for (i = 0; i < numOperation; i++) {

        long r = random_generate(randomPtr) % 100;
        action_t action = selectAction(r, percentUser);

        switch (action) {

            case ACTION_MAKE_RESERVATION: {
                long maxPrices[NUM_RESERVATION_TYPE] = { -1, -1, -1 };
                long maxIds[NUM_RESERVATION_TYPE] = { -1, -1, -1 };
                long n;
                long numQuery = random_generate(randomPtr) % numQueryPerTransaction + 1;
                long customerId = random_generate(randomPtr) % queryRange + 1;
                for (n = 0; n < numQuery; n++) {
                    types[n] = random_generate(randomPtr) % NUM_RESERVATION_TYPE;
                    ids[n] = (random_generate(randomPtr) % queryRange) + 1;
                }
                bool_t isFound = FALSE;
                HTM_TX_INIT;
tsx_begin_make:
                if (HTM_BEGIN(tsx_status, global_tsx_status)) {
                    HTM_LOCK_READ();
                    for (n = 0; n < numQuery; n++) {
                        long t = types[n];
                        long id = ids[n];
                        long price = -1;
                        switch (t) {
                            case RESERVATION_CAR:
                                if (HTMMANAGER_QUERY_CAR(managerPtr, id) >= 0) {
                                    price = HTMMANAGER_QUERY_CAR_PRICE(managerPtr, id);
                                }
                                break;
                            case RESERVATION_FLIGHT:
                                if (HTMMANAGER_QUERY_FLIGHT(managerPtr, id) >= 0) {
                                    price = HTMMANAGER_QUERY_FLIGHT_PRICE(managerPtr, id);
                                }
                                break;
                            case RESERVATION_ROOM:
                                if (HTMMANAGER_QUERY_ROOM(managerPtr, id) >= 0) {
                                    price = HTMMANAGER_QUERY_ROOM_PRICE(managerPtr, id);
                                }
                                break;
                            default:
                                assert(0);
                        }
                        if (price > maxPrices[t]) {
                            maxPrices[t] = price;
                            maxIds[t] = id;
                            isFound = TRUE;
                        }
                    } /* for n */
                    if (isFound) {
                        HTMMANAGER_ADD_CUSTOMER(managerPtr, customerId);
                    }
                    if (maxIds[RESERVATION_CAR] > 0) {
                        HTMMANAGER_RESERVE_CAR(managerPtr,
                                            customerId, maxIds[RESERVATION_CAR]);
                    }
                    if (maxIds[RESERVATION_FLIGHT] > 0) {
                        HTMMANAGER_RESERVE_FLIGHT(managerPtr,
                                               customerId, maxIds[RESERVATION_FLIGHT]);
                    }
                    if (maxIds[RESERVATION_ROOM] > 0) {
                        HTMMANAGER_RESERVE_ROOM(managerPtr,
                                             customerId, maxIds[RESERVATION_ROOM]);
                    }
                    HTM_END(global_tsx_status);
                } else {
                    HTM_RETRY(tsx_status, tsx_begin_make);

                    TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_CLIENT)
                    TM_LOG_BEGIN(CLT_RESERVE, merge);
#endif /* !ORIGINAL && MERGE_CLIENT */
                    for (n = 0; n < numQuery; n++) {
                        long t = types[n];
                        long id = ids[n];
                        long price = -1;
                        switch (t) {
                            case RESERVATION_CAR:
                                if (MANAGER_QUERY_CAR(managerPtr, id) >= 0) {
                                    price = MANAGER_QUERY_CAR_PRICE(managerPtr, id);
                                }
                                break;
                            case RESERVATION_FLIGHT:
                                if (MANAGER_QUERY_FLIGHT(managerPtr, id) >= 0) {
                                    price = MANAGER_QUERY_FLIGHT_PRICE(managerPtr, id);
                                }
                                break;
                            case RESERVATION_ROOM:
                                if (MANAGER_QUERY_ROOM(managerPtr, id) >= 0) {
                                    price = MANAGER_QUERY_ROOM_PRICE(managerPtr, id);
                                }
                                break;
                            default:
                                assert(0);
                        }
                        if (price > maxPrices[t]) {
                            maxPrices[t] = price;
                            maxIds[t] = id;
                            isFound = TRUE;
                        }
                    } /* for n */
                    if (isFound) {
                        MANAGER_ADD_CUSTOMER(managerPtr, customerId);
                    }
                    if (maxIds[RESERVATION_CAR] > 0) {
                        MANAGER_RESERVE_CAR(managerPtr,
                                            customerId, maxIds[RESERVATION_CAR]);
                    }
                    if (maxIds[RESERVATION_FLIGHT] > 0) {
                        MANAGER_RESERVE_FLIGHT(managerPtr,
                                               customerId, maxIds[RESERVATION_FLIGHT]);
                    }
                    if (maxIds[RESERVATION_ROOM] > 0) {
                        MANAGER_RESERVE_ROOM(managerPtr,
                                             customerId, maxIds[RESERVATION_ROOM]);
                    }
                    /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
                    // TM_LOG_END(CLT_RESERVE, NULL);
                    TM_END();
                }

                break;
            }

            case ACTION_DELETE_CUSTOMER: {
                long customerId = random_generate(randomPtr) % queryRange + 1;
                HTM_TX_INIT;
tsx_begin_delete:
                if (HTM_BEGIN(tsx_status, global_tsx_status)) {
                    HTM_LOCK_READ();
                    long bill = HTMMANAGER_QUERY_CUSTOMER_BILL(managerPtr, customerId);
                    if (bill >= 0) {
                        HTMMANAGER_DELETE_CUSTOMER(managerPtr, customerId);
                    }
                    HTM_END(global_tsx_status);
                } else {
                    HTM_RETRY(tsx_status, tsx_begin_delete);

                    TM_BEGIN();

#if !defined(ORIGINAL) && defined(MERGE_CLIENT)
                    TM_LOG_BEGIN(CLT_DELCUSTOMER, NULL);
#endif /* !ORIGINAL && MERGE_CLIENT */
                    long bill = MANAGER_QUERY_CUSTOMER_BILL(managerPtr, customerId);
                    if (bill >= 0) {
                        MANAGER_DELETE_CUSTOMER(managerPtr, customerId);
                    }
                    /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
                    // TM_LOG_END(CLT_DELCUSTOMER, NULL);
                    TM_END();
                }

                break;
            }

            case ACTION_UPDATE_TABLES: {
                long numUpdate = random_generate(randomPtr) % numQueryPerTransaction + 1;
                long n;
                for (n = 0; n < numUpdate; n++) {
                    types[n] = random_generate(randomPtr) % NUM_RESERVATION_TYPE;
                    ids[n] = (random_generate(randomPtr) % queryRange) + 1;
                    ops[n] = random_generate(randomPtr) % 2;
                    if (ops[n]) {
                        prices[n] = ((random_generate(randomPtr) % 5) * 10) + 50;
                    }
                }
                HTM_TX_INIT;
tsx_begin_update:
                if (HTM_BEGIN(tsx_status, global_tsx_status)) {
                    HTM_LOCK_READ();
                    for (n = 0; n < numUpdate; n++) {
                        long t = types[n];
                        long id = ids[n];
                        long doAdd = ops[n];
                        if (doAdd) {
                            long newPrice = prices[n];
                            switch (t) {
                                case RESERVATION_CAR:
                                    HTMMANAGER_ADD_CAR(managerPtr, id, 100, newPrice);
                                    break;
                                case RESERVATION_FLIGHT:
                                    HTMMANAGER_ADD_FLIGHT(managerPtr, id, 100, newPrice);
                                    break;
                                case RESERVATION_ROOM:
                                    HTMMANAGER_ADD_ROOM(managerPtr, id, 100, newPrice);
                                    break;
                                default:
                                    assert(0);
                            }
                        } else { /* do delete */
                            switch (t) {
                                case RESERVATION_CAR:
                                    HTMMANAGER_DELETE_CAR(managerPtr, id, 100);
                                    break;
                                case RESERVATION_FLIGHT:
                                    HTMMANAGER_DELETE_FLIGHT(managerPtr, id);
                                    break;
                                case RESERVATION_ROOM:
                                    HTMMANAGER_DELETE_ROOM(managerPtr, id, 100);
                                    break;
                                default:
                                    assert(0);
                            }
                        }
                    }
                    HTM_END(global_tsx_status);
                } else {
                    HTM_RETRY(tsx_status, tsx_begin_update);

                    TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_CLIENT)
                    TM_LOG_BEGIN(CLT_UPDATE, merge);
#endif /* !ORIGINAL && MERGE_CLIENT */
                    for (n = 0; n < numUpdate; n++) {
                        long t = types[n];
                        long id = ids[n];
                        long doAdd = ops[n];
                        if (doAdd) {
                            long newPrice = prices[n];
                            switch (t) {
                                case RESERVATION_CAR:
                                    MANAGER_ADD_CAR(managerPtr, id, 100, newPrice);
                                    break;
                                case RESERVATION_FLIGHT:
                                    MANAGER_ADD_FLIGHT(managerPtr, id, 100, newPrice);
                                    break;
                                case RESERVATION_ROOM:
                                    MANAGER_ADD_ROOM(managerPtr, id, 100, newPrice);
                                    break;
                                default:
                                    assert(0);
                            }
                        } else { /* do delete */
                            switch (t) {
                                case RESERVATION_CAR:
                                    MANAGER_DELETE_CAR(managerPtr, id, 100);
                                    break;
                                case RESERVATION_FLIGHT:
                                    MANAGER_DELETE_FLIGHT(managerPtr, id);
                                    break;
                                case RESERVATION_ROOM:
                                    MANAGER_DELETE_ROOM(managerPtr, id, 100);
                                    break;
                                default:
                                    assert(0);
                            }
                        }
                    }
                    /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
                    // TM_LOG_END(CLT_UPDATE, NULL);
                    TM_END();
                }

                break;
            }

            default:
                assert(0);

        } /* switch (action) */

    } /* for i */

    TM_THREAD_EXIT();

    P_FREE(types);
    P_FREE(ids);
    P_FREE(ops);
    P_FREE(prices);
}


/* =============================================================================
 *
 * End of client.c
 *
 * =============================================================================
 */

#if !defined(ORIGINAL) && defined(MERGE_CLIENT)
__attribute__((constructor)) void client_init() {
    TM_LOG_FFI_DECLARE;
    # define TM_LOG_OP TM_LOG_OP_INIT
    # include "client.inc"
    # undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_CLIENT */
