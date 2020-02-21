/* =============================================================================
 *
 * vacation.c
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
#include <stdio.h>
#include <getopt.h>
#include "client.h"
#include "customer.h"
#include "list.h"
#include "manager.h"
#include "map.h"
#include "memory.h"
#include "operation.h"
#include "random.h"
#include "reservation.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"
#include "types.h"
#include "utility.h"

#ifdef PERSISTENT
#include <libpmemobj.h>
#include <sys/stat.h>

#define PERSISTENT_NAME     "/pmem-fs/vacation"
#define PERSISTENT_LAYOUT   POBJ_LAYOUT_NAME(vacation)
#define PERSISTENT_SIZE     8UL * 1024 * 1024 * 1024

POBJ_LAYOUT_BEGIN(vacation);
POBJ_LAYOUT_ROOT(vacation, struct root);
POBJ_LAYOUT_END(vacation);

typedef struct root {
    int glb_mgr_initialized;
    manager_t manager;
} root_t;
#endif /* PERSISTENT */

enum param_types {
    PARAM_CLIENTS      = (unsigned char)'c',
    PARAM_NUMBER       = (unsigned char)'n',
    PARAM_QUERIES      = (unsigned char)'q',
    PARAM_RELATIONS    = (unsigned char)'r',
    PARAM_TRANSACTIONS = (unsigned char)'t',
    PARAM_USER         = (unsigned char)'u',
};

#define PARAM_DEFAULT_CLIENTS      (1)
#define PARAM_DEFAULT_NUMBER       (10)
#define PARAM_DEFAULT_QUERIES      (90)
#ifdef PERSISTENT
# define PARAM_DEFAULT_RELATIONS    ((1<<22) + (1<<20) + (1<<19))
# define PARAM_DEFAULT_TRANSACTIONS (1 << 17)
#else
# define PARAM_DEFAULT_RELATIONS    (1 << 16)
# define PARAM_DEFAULT_TRANSACTIONS (1 << 26)
#endif /* PERSISTENT */
#define PARAM_DEFAULT_USER         (80)

double global_params[256]; /* 256 = ascii limit */


HTM_STATS(global_tsx_status);


/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void
displayUsage (const char* appName)
{
    printf("Usage: %s [options]\n", appName);
    puts("\nOptions:                                             (defaults)\n");
    printf("    c <UINT>   Number of [c]lients                   (%i)\n",
           PARAM_DEFAULT_CLIENTS);
    printf("    n <UINT>   [n]umber of user queries/transaction  (%i)\n",
           PARAM_DEFAULT_NUMBER);
    printf("    q <UINT>   Percentage of relations [q]ueried     (%i)\n",
           PARAM_DEFAULT_QUERIES);
    printf("    r <UINT>   Number of possible [r]elations        (%i)\n",
           PARAM_DEFAULT_RELATIONS);
    printf("    t <UINT>   Number of [t]ransactions              (%i)\n",
           PARAM_DEFAULT_TRANSACTIONS);
    printf("    u <UINT>   Percentage of [u]ser transactions     (%i)\n",
           PARAM_DEFAULT_USER);
    exit(1);
}


/* =============================================================================
 * setDefaultParams
 * =============================================================================
 */
static void
setDefaultParams ()
{
    global_params[PARAM_CLIENTS]      = PARAM_DEFAULT_CLIENTS;
    global_params[PARAM_NUMBER]       = PARAM_DEFAULT_NUMBER;
    global_params[PARAM_QUERIES]      = PARAM_DEFAULT_QUERIES;
    global_params[PARAM_RELATIONS]    = PARAM_DEFAULT_RELATIONS;
    global_params[PARAM_TRANSACTIONS] = PARAM_DEFAULT_TRANSACTIONS;
    global_params[PARAM_USER]         = PARAM_DEFAULT_USER;
}


/* =============================================================================
 * parseArgs
 * =============================================================================
 */
static void
parseArgs (long argc, char* const argv[])
{
    long i;
    long opt;

    opterr = 0;

    setDefaultParams();

    while ((opt = getopt(argc, argv, "c:n:q:r:t:u:")) != -1) {
        switch (opt) {
            case 'c':
            case 'n':
            case 'q':
            case 'r':
            case 't':
            case 'u':
                global_params[(unsigned char)opt] = atol(optarg);
                break;
            case '?':
            default:
                opterr++;
                break;
        }
    }

    for (i = optind; i < argc; i++) {
        fprintf(stderr, "Non-option argument: %s\n", argv[i]);
        opterr++;
    }

    if (opterr) {
        displayUsage(argv[0]);
    }
}


/* =============================================================================
 * addCustomer
 * -- Wrapper function
 * =============================================================================
 */
static bool_t
addCustomer (manager_t* managerPtr, long id, long num, long price)
{
#ifdef PERSISTENT
    return manager_addCustomer(managerPtr, id);
#else
    return manager_addCustomer_seq(managerPtr, id);
#endif /* PERSISTENT */
}


/* =============================================================================
 * initializeManager
 * =============================================================================
 */
static manager_t*
initializeManager (
#ifdef PERSISTENT
    manager_t *managerPtr
#endif /* PERSISTENT */
) {
    long i;
    long numRelation;
    random_t* randomPtr;
    long* ids;
#ifdef PERSISTENT
    bool_t (*manager_add[])(manager_t*, long, long, long) = {
        &manager_addCar,
        &manager_addFlight,
        &manager_addRoom,
        &addCustomer
    };
#else
    bool_t (*manager_add[])(manager_t*, long, long, long) = {
        &manager_addCar_seq,
        &manager_addFlight_seq,
        &manager_addRoom_seq,
        &addCustomer
    };
#endif /* PERSISTENT */
    long t;
    long numTable = sizeof(manager_add) / sizeof(manager_add[0]);

    printf("Initializing manager... ");
    fflush(stdout);

    randomPtr = random_alloc();
    assert(randomPtr != NULL);

#ifdef PERSISTENT
    manager_alloc(managerPtr);
#else
    manager_t* managerPtr = manager_alloc();
#endif /* PERSISTENT */
    assert(managerPtr != NULL);

    numRelation = (long)global_params[PARAM_RELATIONS];
    ids = (long*)malloc(numRelation * sizeof(long));
    for (i = 0; i < numRelation; i++) {
        ids[i] = i + 1;
    }

    for (t = 0; t < numTable; t++) {

        /* Shuffle ids */
        for (i = 0; i < numRelation; i++) {
            long x = random_generate(randomPtr) % numRelation;
            long y = random_generate(randomPtr) % numRelation;
            long tmp = ids[x];
            ids[x] = ids[y];
            ids[y] = tmp;
        }

        /* Populate table */
        for (i = 0; i < numRelation; i++) {
#ifdef PERSISTENT
            if(i % 100000 == 0)
                printf("Table no. %lu : Completed %lu tuples\n", t, i);
#endif /* PERSISTENT */
            bool_t status;
            long id = ids[i];
            long num = ((random_generate(randomPtr) % 5) + 1) * 100;
            long price = ((random_generate(randomPtr) % 5) * 10) + 50;
#ifdef PERSISTENT
            TM_BEGIN_PERSISTENT();
#endif /* PERSISTENT */
            status = manager_add[t](managerPtr, id, num, price);
#ifdef PERSISTENT
            TM_END();
#endif /* PERSISTENT */
            assert(status);
        }

    } /* for t */

    puts("done.");
    fflush(stdout);

    random_free(randomPtr);
    free(ids);

    return managerPtr;
}


/* =============================================================================
 * initializeClients
 * =============================================================================
 */
static client_t**
initializeClients (manager_t* managerPtr)
{
    random_t* randomPtr;
    client_t** clients;
    long i;
    long numClient = (long)global_params[PARAM_CLIENTS];
    long numTransaction = (long)global_params[PARAM_TRANSACTIONS];
    long numTransactionPerClient;
    long numQueryPerTransaction = (long)global_params[PARAM_NUMBER];
    long numRelation = (long)global_params[PARAM_RELATIONS];
    long percentQuery = (long)global_params[PARAM_QUERIES];
    long queryRange;
    long percentUser = (long)global_params[PARAM_USER];

    printf("Initializing clients... ");
    fflush(stdout);

    randomPtr = random_alloc();
    assert(randomPtr != NULL);

    clients = (client_t**)malloc(numClient * sizeof(client_t*));
    assert(clients != NULL);
    numTransactionPerClient = (long)((double)numTransaction / (double)numClient + 0.5);
    queryRange = (long)((double)percentQuery / 100.0 * (double)numRelation + 0.5);

    for (i = 0; i < numClient; i++) {
        clients[i] = client_alloc(i,
                                  managerPtr,
                                  numTransactionPerClient,
                                  numQueryPerTransaction,
                                  queryRange,
                                  percentUser);
        assert(clients[i]  != NULL);
    }

    puts("done.");
    printf("    Transactions        = %li\n", numTransaction);
    printf("    Clients             = %li\n", numClient);
    printf("    Transactions/client = %li\n", numTransactionPerClient);
    printf("    Queries/transaction = %li\n", numQueryPerTransaction);
    printf("    Relations           = %li\n", numRelation);
    printf("    Query percent       = %li\n", percentQuery);
    printf("    Query range         = %li\n", queryRange);
    printf("    Percent user        = %li\n", percentUser);
    fflush(stdout);

    random_free(randomPtr);

    return clients;
}


/* =============================================================================
 * checkTables
 * -- some simple checks (not comprehensive)
 * -- dependent on tasks generated for clients in initializeClients()
 * =============================================================================
 */
bool_t
checkTables (manager_t* managerPtr)
{
    bool_t status = TRUE;
    long i;
    long numRelation = (long)global_params[PARAM_RELATIONS];
    MAP_T* customerTablePtr = managerPtr->customerTablePtr;
    MAP_T* tables[] = {
        managerPtr->carTablePtr,
        managerPtr->flightTablePtr,
        managerPtr->roomTablePtr,
    };
    long numTable = sizeof(tables) / sizeof(tables[0]);
    bool_t (*manager_add[])(manager_t*, long, long, long) = {
        &manager_addCar_seq,
        &manager_addFlight_seq,
        &manager_addRoom_seq
    };
    long t;

    printf("Checking tables... ");
    fflush(stdout);

#ifndef PERSISTENT
    /* Check for unique customer IDs */
    long percentQuery = (long)global_params[PARAM_QUERIES];
    long queryRange = (long)((double)percentQuery / 100.0 * (double)numRelation + 0.5);
    long maxCustomerId = queryRange + 1;
    for (i = 1; i <= maxCustomerId; i++) {
        customer_t *c;
        if ((c = MAP_FIND(customerTablePtr, i))) {
            customer_free_seq(c);
            if (MAP_REMOVE(customerTablePtr, i)) {
                status = status ? !MAP_FIND(customerTablePtr, i) : status;
                assert(status);
            }
        }
    }

    /* Check reservation tables for consistency and unique ids */
    for (t = 0; t < numTable; t++) {
        MAP_T* tablePtr = tables[t];
        for (i = 1; i <= numRelation; i++) {
            reservation_t *r;
            if ((r = MAP_FIND(tablePtr, i))) {
                status = status ? manager_add[t](managerPtr, i, 0, 0) : status; /* validate entry */
                assert(status);
                reservation_free_seq(r);
                if (MAP_REMOVE(tablePtr, i)) {
                    status = status ? !MAP_REMOVE(tablePtr, i) : status;
                    assert(status);
                }
            }
        }
    }
#endif /* PERSISTENT */

    puts("done.");
    fflush(stdout);
    return status;
}


/* =============================================================================
 * freeClients
 * =============================================================================
 */
static void
freeClients (client_t** clients)
{
    long i;
    long numClient = (long)global_params[PARAM_CLIENTS];

    for (i = 0; i < numClient; i++) {
        client_t* clientPtr = clients[i];
        client_free(clientPtr);
    }
    free(clients);
}


/* =============================================================================
 * main
 * =============================================================================
 */
MAIN(argc, argv)
{
    manager_t* managerPtr;
    client_t** clients;
    TIMER_T start;
    TIMER_T stop;

    GOTO_REAL();

    /* Initialization */
#ifdef PERSISTENT
    PMEMobjpool *pop;
    if (!(pop = pmemobj_open(PERSISTENT_NAME, PERSISTENT_LAYOUT))) {
        if (!(pop = pmemobj_create(PERSISTENT_NAME, PERSISTENT_LAYOUT, PERSISTENT_SIZE, S_IWUSR | S_IRUSR)))
            return -1;
    }

    TOID(struct root) root = POBJ_ROOT(pop, struct root);
    root_t *rootp = D_RW(root);
#endif /* PERSISTENT */

    parseArgs(argc, (char** const)argv);
    SIM_GET_NUM_CPU(global_params[PARAM_CLIENTS]);

    long numThread = global_params[PARAM_CLIENTS];

#ifdef PERSISTENT
    TM_STARTUP_PERSISTENT(numThread, pop);
#else
    TM_STARTUP(numThread);
#endif /* PERSISTENT */

    P_MEMORY_STARTUP(numThread);
    thread_startup(numThread);

#ifdef PERSISTENT
    TM_THREAD_ENTER();
    managerPtr = &rootp->manager;
    if (rootp->glb_mgr_initialized) {
        printf("\n***************************************************\n");
        printf("\nRe-using tables from previous incarnation...\n");
        printf("\nPersistent table pointers.\n");
        printf("\n%s = %p\n%s = %p\n%s = %p\n%s = %p\n",                  \
                "Car Table     ", managerPtr->carTablePtr,  \
                "Room Table    ", managerPtr->roomTablePtr,     \
                "Flight Table  ", managerPtr->flightTablePtr,   \
                "Customer Table", managerPtr->customerTablePtr);
        printf("\n***************************************************\n");

        // FIXME: Workaround for function pointers
        rbtree_setCompare(managerPtr->carTablePtr, rbtree_compare);
        rbtree_setCompare(managerPtr->carTablePtr, rbtree_compare);
        rbtree_setCompare(managerPtr->roomTablePtr, rbtree_compare);
        rbtree_setCompare(managerPtr->flightTablePtr, rbtree_compare);
        rbtree_setCompare(managerPtr->customerTablePtr, rbtree_compare);
        rbtree_iterate(managerPtr->customerTablePtr, customer_setCompare);
    } else {
        initializeManager(managerPtr);
        rootp->glb_mgr_initialized = 1;
    }
#else
    managerPtr = initializeManager();
#endif /* PERSISTENT */

    assert(managerPtr != NULL);
    clients = initializeClients(managerPtr);
    assert(clients != NULL);

    /* Run transactions */
    printf("Running clients... ");
    fflush(stdout);
    TIMER_READ(start);
    GOTO_SIM();
#ifdef OTM
#pragma omp parallel
    {
        client_run(clients);
    }
#else
    thread_start(client_run, (void*)clients);
#endif
    GOTO_REAL();
    TIMER_READ(stop);
    puts("done.");
    printf("Transaction time = %f\n",
           TIMER_DIFF_SECONDS(start, stop));
    fflush(stdout);
    bool_t status = checkTables(managerPtr);
    assert(status);

    HTM_STATS_PRINT(global_tsx_status);

    /* Clean up */
    printf("Deallocating memory... ");
    fflush(stdout);
    freeClients(clients);
    manager_free(managerPtr);
    puts("done.");
    fflush(stdout);

    TM_SHUTDOWN();
    P_MEMORY_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();

#ifdef PERSISTENT
    pmemobj_close(pop);
#endif /* PERSISTENT */

    MAIN_RETURN(!status);
}


/* =============================================================================
 *
 * End of vacation.c
 *
 * =============================================================================
 */
