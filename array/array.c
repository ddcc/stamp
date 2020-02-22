#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "random.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"

#if !defined(ORIGINAL) && defined(MERGE_ARRAY)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "array.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_ARRAY */

typedef struct {
    uint64_t *array;
    random_t *random;

    unsigned long size;
    unsigned long ops_per_tx;
    unsigned long reads;
    unsigned long spin;
    unsigned long threads;
    unsigned long transactions;
    double time;
} data_t;

HTM_STATS(global_tsx_status);

TM_INIT_GLOBAL;

void usage(char* argv0) {
    const char help[] =
        "Usage: %s [switches] -i filename\n"
        "       -a               size of array\n"
        "       -n               number of operations per transaction\n"
        "       -o               total number of transactions\n"
        "       -r               percentage of reads\n"
        "       -s               spin time\n"
        "       -t               number of threads\n";
    fprintf(stderr, help, argv0);
    exit(-1);
}

static unsigned long random_uniform(random_t *r, unsigned long range) {
    const unsigned long max = (((0xffffffffUL + 1) / range) * range);
    unsigned long v;

    do {
        v = random_generate(r);
    } while (v > max);

    return v % range;
}

void work(data_t *data) {
    TM_THREAD_ENTER();

#if !defined(ORIGINAL) && defined(MERGE_ARRAY)
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Check the conflict address is at an in-bounds array index */
        if ((unsigned long *)params->addr >= data->array && (unsigned long *)params->addr < data->array + data->size) {
            /* Compute the array index */
            unsigned long offset = (unsigned long *)params->addr - data->array;

            ASSERT(params->leaf == 1);
            ASSERT(ENTRY_VALID(params->conflict.entries->e1));
            /* Opaque objects representing the conficting read, and an optional write at that array index */
            const stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
            const stm_write_t w = TM_SHARED_DID_WRITE(data->array[offset]);

            /* Fetch the old and new values of the read */
            unsigned long old, new, write = 0;
            ASSERT_FAIL(TM_SHARED_READ_VALUE(r, data->array[offset], old));
            ASSERT_FAIL(TM_SHARED_READ_UPDATE(r, data->array[offset], new));

            /* If the values are the same, we are done */
            if (old == new)
                return STM_MERGE_OK;

            /* If the write is valid, fetch its value */
            if (STM_VALID_WRITE(w))
                ASSERT_FAIL(TM_SHARED_WRITE_VALUE(w, data->array[offset], write));

#  ifdef TM_DEBUG
            printf("\nARRAY_ADD_JIT data->array[%ld] read (old):%ld (new):%ld write (old):%ld\n", offset, old, new, write);
#  endif

            /* If the write is valid, update it now by re-incrementing */
            if (STM_VALID_WRITE(w))
                ASSERT_FAIL(TM_SHARED_WRITE_UPDATE(w, data->array[offset], (new - old) + write));

            /* Repair succeeded */
            return STM_MERGE_OK;
        }

# ifdef TM_DEBUG
        printf("\nARRAY_ADD_JIT UNSUPPORTED addr:%p\n", params->addr);
# endif
        return STM_MERGE_UNSUPPORTED;
    }
#endif /* !ORIGINAL && MERGE_ARRAY */

    unsigned long offsets[data->ops_per_tx];
    unsigned long ops[data->ops_per_tx];

    random_t *random = random_alloc();
    random_seed(random, thread_getId());

    for (unsigned int i = 0; i < data->ops_per_tx; ++i) {
        offsets[i] = random_uniform(random, data->size);
        ops[i] = random_uniform(random, 100);
        if (ops[i] < data->reads)
            ops[i] = 0;
    }

    HTM_TX_INIT;
    for (unsigned int i = 0; i < data->transactions / data->threads; ++i) {
tsx_begin:
        if (HTM_BEGIN(tsx_status, global_tsx_status)) {
            HTM_LOCK_READ();

            for (unsigned int j = 0; j < data->ops_per_tx; ++j) {
                unsigned long offset = offsets[j];
                unsigned long v = HTM_SHARED_READ(data->array[offset]);
                if (ops[j])
                    HTM_SHARED_WRITE(data->array[offset], v + ops[j]);

                for (unsigned int k = 0; k < data->spin; ++k) {
                    // spin
                }
            }
            HTM_END(global_tsx_status);
        } else {
            HTM_RETRY(tsx_status, tsx_begin);

            TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_ARRAY)
            TM_LOG_BEGIN(ARRAY_ADD, merge);
#endif /* !ORIGINAL && MERGE_ARRAY */

            for (unsigned int j = 0; j < data->ops_per_tx; ++j) {
                unsigned long offset = offsets[j];
                unsigned long v = TM_SHARED_READ(data->array[offset]);
                if (ops[j])
                    TM_SHARED_WRITE(data->array[offset], v + ops[j]);

                for (unsigned int k = 0; k < data->spin; ++k) {
                    // spin
                }
            }

            /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
            // TM_LOG_END(ARRAY_ADD, NULL);
            TM_END();
        }
    }

    random_free(random);

    TM_THREAD_EXIT();
}

void init(data_t *data) {
    /* Allocate storage */
    data->array = malloc(data->size * sizeof(*data->array));
    data->random = random_alloc();
    random_seed(data->random, 0);

    /* Generate the array */
    for (unsigned long i = 0; i < data->size; ++i)
        data->array[i] = random_generate(data->random);
}

void compute(data_t *data) {
    TIMER_T start, stop;

    TIMER_READ(start);
    GOTO_SIM();

    {
#ifdef OTM
#pragma omp parallel
        work((void (*)(void *))data);
#else
        thread_start((void (*)(void *))work, data);
#endif
    }

    GOTO_REAL();

    TIMER_READ(stop);
    data->time = TIMER_DIFF_SECONDS(start, stop);
}

void cleanup(data_t *data) {
    free(data->array);
    random_free(data->random);
}

int main(int argc, char **argv) {
    data_t data = { .size = 1024, .ops_per_tx = 1000, .reads = 25, .spin = 1000, .transactions = 16000, .threads = 1 };
    int opt;

    GOTO_REAL();

    while ((opt = getopt(argc,(char**)argv,"a:n:o:r:s:t:")) != EOF) {
        switch (opt) {
            case 'a': data.size = atol(optarg);
                break;
            case 'n': data.ops_per_tx = atol(optarg);
                break;
            case 'o': data.transactions = atol(optarg);
                break;
            case 'r': data.reads = atol(optarg);
                break;
            case 's': data.spin = atol(optarg);
                break;
            case 't': data.threads = atol(optarg);
                break;
            default:  usage((char*)argv[0]);
                break;
        }
    }

    if (!data.size || !data.ops_per_tx || data.reads > 100 || !data.threads || !data.transactions) {
        fprintf(stderr, "Error: invalid parameter values\n");
        usage((char*)argv[0]);
    }
    printf("Array size = %ld\nOperations per TX = %ld\nPercent Read-Only = %ld\nSpin Cycles = %ld\nTransactions = %ld\nThreads = %ld\n", data.size, data.ops_per_tx, data.reads, data.spin, data.transactions, data.threads);

    init(&data);

    SIM_GET_NUM_CPU(data.threads);
    TM_STARTUP(data.threads);
    thread_startup(data.threads);

    compute(&data);
    printf("Time = %f\n", data.time);

    HTM_STATS_PRINT(global_tsx_status);

    TM_SHUTDOWN();
    GOTO_SIM();

    cleanup(&data);

    thread_shutdown();
    return 0;
}

#if !defined(ORIGINAL) && defined(MERGE_ARRAY)
__attribute__((constructor)) void array_init() {
    TM_LOG_FFI_DECLARE;
    # define TM_LOG_OP TM_LOG_OP_INIT
    # include "array.inc"
    # undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_ARRAY */
