/* =============================================================================
 *
 * intruder.c
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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "decoder.h"
#include "detector.h"
#include "dictionary.h"
#include "packet.h"
#include "stream.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"

enum param_types {
    PARAM_ATTACK = (unsigned char)'a',
    PARAM_LENGTH = (unsigned char)'l',
    PARAM_NUM    = (unsigned char)'n',
    PARAM_SEED   = (unsigned char)'s',
    PARAM_THREAD = (unsigned char)'t',
};

enum param_defaults {
    PARAM_DEFAULT_ATTACK = 10,
    PARAM_DEFAULT_LENGTH = 16,
    PARAM_DEFAULT_NUM    = 1 << 20,
    PARAM_DEFAULT_SEED   = 1,
    PARAM_DEFAULT_THREAD = 1,
};

long global_params[256] = { /* 256 = ascii limit */
    [PARAM_ATTACK] = PARAM_DEFAULT_ATTACK,
    [PARAM_LENGTH] = PARAM_DEFAULT_LENGTH,
    [PARAM_NUM]    = PARAM_DEFAULT_NUM,
    [PARAM_SEED]   = PARAM_DEFAULT_SEED,
    [PARAM_THREAD] = PARAM_DEFAULT_THREAD,
};

typedef struct arg {
  /* input: */
    stream_t* streamPtr;
    decoder_t* decoderPtr;
  /* output: */
    vector_t** errorVectors;
} arg_t;

#if !defined(ORIGINAL) && defined(MERGE_INTRUDER)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "intruder.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_INTRUDER */

HTM_STATS_EXTERN(global_tsx_status);

/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void
displayUsage (const char* appName)
{
    printf("Usage: %s [options]\n", appName);
    puts("\nOptions:                            (defaults)\n");
    printf("    a <UINT>   Percent [a]ttack     (%i)\n", PARAM_DEFAULT_ATTACK);
    printf("    l <UINT>   Max data [l]ength    (%i)\n", PARAM_DEFAULT_LENGTH);
    printf("    n <UINT>   [n]umber of flows    (%i)\n", PARAM_DEFAULT_NUM);
    printf("    s <UINT>   Random [s]eed        (%i)\n", PARAM_DEFAULT_SEED);
    printf("    t <UINT>   Number of [t]hreads  (%i)\n", PARAM_DEFAULT_THREAD);
    exit(1);
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

    while ((opt = getopt(argc, argv, "a:l:n:s:t:")) != -1) {
        switch (opt) {
            case 'a':
            case 'l':
            case 'n':
            case 's':
            case 't':
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
 * processPackets
 * =============================================================================
 */
void
processPackets (void* argPtr)
{
    TM_THREAD_ENTER();

    long threadId = thread_getId();

    stream_t*   streamPtr    = ((arg_t*)argPtr)->streamPtr;
    decoder_t*  decoderPtr   = ((arg_t*)argPtr)->decoderPtr;
    vector_t**  errorVectors = ((arg_t*)argPtr)->errorVectors;

    char *bytes, *data;

#if !defined(ORIGINAL) && defined(MERGE_INTRUDER)
    stm_merge_t merge(stm_merge_context_t *params) {
#ifdef MERGE_QUEUE
        extern const stm_op_id_t QUEUE_POP;
#endif /* MERGE_QUEUE */
#ifdef MERGE_DECODER
        extern const stm_op_id_t DEC_GETCOMPLETE;
#endif /* MERGE_DECODER */
        const stm_op_id_t prev_op = params->leaf ? STM_INVALID_OPID : stm_get_op_opcode(params->previous);

        /* Just-in-time merge */
        ASSERT(STM_SAME_OP(stm_get_current_op(), params->current));

#ifdef TM_DEBUG
        printf("\nINTRUDER_JIT addr:%p streamPtr:%p decoderPtr:%p\n", params->addr, streamPtr, decoderPtr);
#endif

        /* Update from QUEUE_POP or DEC_GETCOMPLETE to INTRUDER_PACKET */
        ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->current), INTRUDER_PACKET));
        if (0) {
#ifdef MERGE_DECODER
        } else if (STM_SAME_OPID(prev_op, DEC_GETCOMPLETE)) {
# ifdef TM_DEBUG
            printf("INTRUDER_JIT <- DEC_GETCOMPLETE %p\n", params->conflict.result.ptr);
# endif
            data = params->conflict.result.ptr;
            return STM_MERGE_OK;
#endif /* MERGE_DECODER */
#ifdef MERGE_QUEUE
        } else if (STM_SAME_OPID(prev_op, QUEUE_POP)) {
# ifdef TM_DEBUG
            printf("INTRUDER_JIT <- QUEUE_POP %p\n", params->conflict.result.ptr);
# endif
            bytes = params->conflict.result.ptr;
            return STM_MERGE_OK;
#endif /* MERGE_QUEUE */
        }

#ifdef TM_DEBUG
        printf("\nINTRUDER_JIT UNSUPPORTED addr:%p\n", params->addr);
#endif
        return STM_MERGE_UNSUPPORTED;
    }
#endif /* !ORIGINAL && MERGE_INTRUDER */

    detector_t* detectorPtr = PDETECTOR_ALLOC();
    assert(detectorPtr);
    PDETECTOR_ADDPREPROCESSOR(detectorPtr, &preprocessor_toLower);

    vector_t* errorVectorPtr = errorVectors[threadId];

    while (1) {
        HTM_TX_INIT;
tsx_begin_getpacket:
        if (HTM_BEGIN(tsx_status, global_tsx_status)) {
            HTM_LOCK_READ();
            bytes = HTMSTREAM_GETPACKET(streamPtr);
            HTM_END(global_tsx_status);
        } else {
            HTM_RETRY(tsx_status, tsx_begin_getpacket);

            TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_INTRUDER)
            TM_LOG_BEGIN(INTRUDER_PACKET, merge);
#endif /* !ORIGINAL && MERGE_INTRUDER */
            bytes = TMSTREAM_GETPACKET(streamPtr);
            /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
            // TM_LOG_END(INTRUDER_PACKET, NULL);
            TM_END();
        }

        if (!bytes) {
            break;
        }

        packet_t* packetPtr = (packet_t*)bytes;
        long flowId = packetPtr->flowId;

        error_t error;
        error = TMDECODER_PROCESS(decoderPtr,
                                  bytes,
                                  (PACKET_HEADER_LENGTH + packetPtr->length));
        if (error) {
            /*
             * Currently, stream_generate() does not create these errors.
             */
            assert(0);
            bool_t status = PVECTOR_PUSHBACK(errorVectorPtr, (void*)flowId);
            assert(status);
        }

        long decodedFlowId;
tsx_begin_getcomplete:
        if (HTM_BEGIN(tsx_status, global_tsx_status)) {
            HTM_LOCK_READ();
            data = HTMDECODER_GETCOMPLETE(decoderPtr, &decodedFlowId);
            HTM_END(global_tsx_status);
        } else {
            HTM_RETRY(tsx_status, tsx_begin_getcomplete);

            TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_INTRUDER)
            TM_LOG_BEGIN(INTRUDER_PACKET, merge);
#endif /* !ORIGINAL && MERGE_INTRUDER */
            data = TMDECODER_GETCOMPLETE(decoderPtr, &decodedFlowId);
            /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
            // TM_LOG_END(INTRUDER_PACKET, NULL);
            TM_END();
        }

        if (data) {
            error_t error = PDETECTOR_PROCESS(detectorPtr, data);
            P_FREE(data);
            if (error) {
                bool_t status = PVECTOR_PUSHBACK(errorVectorPtr,
                                                 (void*)decodedFlowId);
                assert(status);
            }
        }

    }

    PDETECTOR_FREE(detectorPtr);

    TM_THREAD_EXIT();
}


/* =============================================================================
 * main
 * =============================================================================
 */
MAIN(argc, argv)
{
    GOTO_REAL();

    /*
     * Initialization
     */

    parseArgs(argc, (char** const)argv);
    long numThread = global_params[PARAM_THREAD];
    SIM_GET_NUM_CPU(numThread);
    TM_STARTUP(numThread);
    P_MEMORY_STARTUP(numThread);
    thread_startup(numThread);

    long percentAttack = global_params[PARAM_ATTACK];
    long maxDataLength = global_params[PARAM_LENGTH];
    long numFlow       = global_params[PARAM_NUM];
    long randomSeed    = global_params[PARAM_SEED];
    printf("Percent attack  = %li\n", percentAttack);
    printf("Max data length = %li\n", maxDataLength);
    printf("Num flow        = %li\n", numFlow);
    printf("Random seed     = %li\n", randomSeed);

    dictionary_t* dictionaryPtr = dictionary_alloc();
    assert(dictionaryPtr);
    stream_t* streamPtr = stream_alloc(percentAttack);
    assert(streamPtr);
    long numAttack = stream_generate(streamPtr,
                                     dictionaryPtr,
                                     numFlow,
                                     randomSeed,
                                     maxDataLength);
    printf("Num attack      = %li\n", numAttack);

    decoder_t* decoderPtr = decoder_alloc();
    assert(decoderPtr);

    vector_t** errorVectors = (vector_t**)malloc(numThread * sizeof(vector_t*));
    assert(errorVectors);
    long i;
    for (i = 0; i < numThread; i++) {
        vector_t* errorVectorPtr = vector_alloc(numFlow);
        assert(errorVectorPtr);
        errorVectors[i] = errorVectorPtr;
    }

    arg_t arg;
    arg.streamPtr    = streamPtr;
    arg.decoderPtr   = decoderPtr;
    arg.errorVectors = errorVectors;

    /*
     * Run transactions
     */

    TIMER_T startTime;
    TIMER_READ(startTime);
    GOTO_SIM();
#ifdef OTM
#pragma omp parallel
    {
        processPackets((void*)&arg);
    }

#else
    thread_start(processPackets, (void*)&arg);
#endif
    GOTO_REAL();
    TIMER_T stopTime;
    TIMER_READ(stopTime);
    printf("Processing time = %f\n", TIMER_DIFF_SECONDS(startTime, stopTime));

    HTM_STATS_PRINT(global_tsx_status);

    /*
     * Check solution
     */

    bool_t success = TRUE;
    for (long f = 1; f <= numFlow; f++) {
        int found = 0;
        for (i = 0; i < numThread; i++) {
            vector_t *errorVectorPtr = errorVectors[i];
            long numError = vector_getSize(errorVectorPtr);
            for (long e = 0; e < numError; e++) {
                if ((long)vector_at(errorVectorPtr, e) == f) {
                    found = 1;
                    goto out;
                }
            }
        }

out:
        success = success ? stream_isAttack(streamPtr, f) == found : success;
# ifdef TM_DEBUG
        if (!success)
            printf("flowId: %lx, computed: %d, actual: %ld\n", f, found, stream_isAttack(streamPtr, f));
# endif
        assert(success);
    }

    long numFound = 0;
    for (i = 0; i < numThread; i++)
        numFound += vector_getSize(errorVectors[i]);
    printf("Num found       = %li\n", numFound);
    success = success ? numFound == numAttack : success;
    assert(success);

    /*
     * Clean up
     */

    for (i = 0; i < numThread; i++) {
        vector_free(errorVectors[i]);
    }
    free(errorVectors);
    decoder_free(decoderPtr);
    stream_free(streamPtr);
    dictionary_free(dictionaryPtr);

    TM_SHUTDOWN();
    P_MEMORY_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();

    MAIN_RETURN(!success);
}


/* =============================================================================
 *
 * End of intruder.c
 *
 * =============================================================================
 */

#if !defined(ORIGINAL) && defined(MERGE_INTRUDER)
__attribute__((constructor)) void intruder_init() {
    TM_LOG_FFI_DECLARE;
    #define TM_LOG_OP TM_LOG_OP_INIT
    #include "intruder.inc"
    #undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_INTRUDER */
