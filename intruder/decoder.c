/* =============================================================================
 *
 * decoder.c
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
#include "decoder.h"
#include "error.h"
#include "list.h"
#include "map.h"
#include "packet.h"
#include "queue.h"
#include "tm.h"
#include "tmstring.h"
#include "types.h"


struct decoder {
    MAP_T* fragmentedMapPtr;  /* contains list of packet_t* */
    queue_t* decodedQueuePtr; /* contains decoded_t* */
};

typedef struct decoded {
    long flowId;
    char* data;
} decoded_t;

#if !defined(ORIGINAL) && defined(MERGE_DECODER)
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "decoder.inc"
# undef TM_LOG_OP
#endif /* !ORIGINAL && MERGE_DECODER */


HTM_STATS(global_tsx_status);

TM_INIT_GLOBAL;

/* =============================================================================
 * decoder_alloc
 * =============================================================================
 */
decoder_t*
decoder_alloc ()
{
    decoder_t* decoderPtr;

    decoderPtr = (decoder_t*)malloc(sizeof(decoder_t));
    if (decoderPtr) {
        decoderPtr->fragmentedMapPtr = MAP_ALLOC(NULL, NULL);
        assert(decoderPtr->fragmentedMapPtr);
        decoderPtr->decodedQueuePtr = queue_alloc(1024);
        assert(decoderPtr->decodedQueuePtr);
    }

    return decoderPtr;
}


/* =============================================================================
 * decoder_free
 * =============================================================================
 */
void
decoder_free (decoder_t* decoderPtr)
{
    queue_free(decoderPtr->decodedQueuePtr);
    MAP_FREE(decoderPtr->fragmentedMapPtr, NULL);
    free(decoderPtr);
}


/* =============================================================================
 * decoder_process
 * =============================================================================
 */
error_t
decoder_process (decoder_t* decoderPtr, char* bytes, long numByte)
{
    bool_t status;

    /*
     * Basic error checking
     */

    if (numByte < PACKET_HEADER_LENGTH) {
        return ERROR_SHORT;
    }

    packet_t* packetPtr = (packet_t*)bytes;
    long flowId      = packetPtr->flowId;
    long fragmentId  = packetPtr->fragmentId;
    long numFragment = packetPtr->numFragment;
    long length      = packetPtr->length;

    if (flowId < 0) {
        return ERROR_FLOWID;
    }

    if ((fragmentId < 0) || (fragmentId >= numFragment)) {
        return ERROR_FRAGMENTID;
    }

    if (length < 0) {
        return ERROR_LENGTH;
    }

#if 0
    /*
     * With the above checks, this one is redundant
     */
    if (numFragment < 1) {
        return ERROR_NUMFRAGMENT;
    }
#endif

    /*
     * Add to fragmented map for reassembling
     */

    if (numFragment > 1) {

        MAP_T* fragmentedMapPtr = decoderPtr->fragmentedMapPtr;
        list_t* fragmentListPtr =
            (list_t*)MAP_FIND(fragmentedMapPtr, (void*)flowId);

        if (fragmentListPtr == NULL) {

            fragmentListPtr = list_alloc(&packet_compareFragmentId);
            assert(fragmentListPtr);
            status = list_insert(fragmentListPtr, (void*)packetPtr);
            assert(status);
            status = MAP_INSERT(fragmentedMapPtr,
                                (void*)flowId,
                                (void*)fragmentListPtr);
            assert(status);

        } else {

            list_iter_t it;
            list_iter_reset(&it, fragmentListPtr);
            assert(list_iter_hasNext(&it, fragmentListPtr));
            packet_t* firstFragmentPtr =
                (packet_t*)list_iter_next(&it, fragmentListPtr);
            long expectedNumFragment = firstFragmentPtr->numFragment;

            if (numFragment != expectedNumFragment) {
                status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);
                return ERROR_NUMFRAGMENT;
            }

            status = list_insert(fragmentListPtr, (void*)packetPtr);
            assert(status);

            /*
             * If we have all the fragments we can reassemble them
             */

            if (list_getSize(fragmentListPtr) == numFragment) {

                long numByte = 0;
                long i = 0;
                list_iter_reset(&it, fragmentListPtr);
                while (list_iter_hasNext(&it, fragmentListPtr)) {
                    packet_t* fragmentPtr =
                        (packet_t*)list_iter_next(&it, fragmentListPtr);
                    assert(fragmentPtr->flowId == flowId);
                    if (fragmentPtr->fragmentId != i) {
                        status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                        assert(status);
                        return ERROR_INCOMPLETE; /* should be sequential */
                    }
                    numByte += fragmentPtr->length;
                    i++;
                }

                char* data = (char*)malloc(numByte + 1);
                assert(data);
                data[numByte] = '\0';
                char* dst = data;
                list_iter_reset(&it, fragmentListPtr);
                while (list_iter_hasNext(&it, fragmentListPtr)) {
                    packet_t* fragmentPtr =
                        (packet_t*)list_iter_next(&it, fragmentListPtr);
                    memcpy(dst, fragmentPtr->data, fragmentPtr->length);
                    dst += fragmentPtr->length;
                }
                assert(dst == data + numByte);

                decoded_t* decodedPtr = (decoded_t*)malloc(sizeof(decoded_t));
                assert(decodedPtr);
                decodedPtr->flowId = flowId;
                decodedPtr->data = data;

                queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
                status = queue_push(decodedQueuePtr, (void*)decodedPtr);
                assert(status);

                list_free(fragmentListPtr, NULL);
                status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);
            }

        }

    } else {

        /*
         * This is the only fragment, so it is ready
         */

        if (fragmentId != 0) {
            return ERROR_FRAGMENTID;
        }

        char* data = (char*)malloc(length + 1);
        assert(data);
        data[length] = '\0';
        memcpy(data, packetPtr->data, length);

        decoded_t* decodedPtr = (decoded_t*)malloc(sizeof(decoded_t));
        assert(decodedPtr);
        decodedPtr->flowId = flowId;
        decodedPtr->data = data;

        queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
        status = queue_push(decodedQueuePtr, (void*)decodedPtr);
        assert(status);

    }

    return ERROR_NONE;
}


/* =============================================================================
 * TMdecoder_process
 * =============================================================================
 */
error_t
TMdecoder_process (TM_ARGDECL  decoder_t* decoderPtr, char* bytes, long numByte)
{
#if !defined(ORIGINAL) && defined(MERGE_DECODER)
    __label__ assemble, push;
#endif /* !ORIGINAL && MERGE_DECODER */

    bool_t status;
    error_t rv;

    /*
     * Basic error checking
     */

    if (numByte < PACKET_HEADER_LENGTH) {
        rv = ERROR_SHORT;
        goto out;
    }

    packet_t* packetPtr = (packet_t*)bytes;
    long flowId      = packetPtr->flowId;
    long fragmentId  = packetPtr->fragmentId;
    long numFragment = packetPtr->numFragment;
    long length      = packetPtr->length;
    char *data;

    if (flowId < 0) {
        rv = ERROR_FLOWID;
        goto out;
    }

    if ((fragmentId < 0) || (fragmentId >= numFragment)) {
        rv = ERROR_FRAGMENTID;
        goto out;
    }

    if (length < 0) {
        rv = ERROR_LENGTH;
        goto out;
    }

    HTM_TX_INIT;
tsx_begin:
    if (HTM_BEGIN(tsx_status, global_tsx_status)) {
        HTM_LOCK_READ();
#if 0
        /*
         * With the above checks, this one is redundant
         */
        if (numFragment < 1) {
            return ERROR_NUMFRAGMENT;
        }
#endif

        /*
         * Add to fragmented map for reassembling
         */

        if (numFragment > 1) {

            MAP_T* fragmentedMapPtr = decoderPtr->fragmentedMapPtr;
            list_t* fragmentListPtr =
                (list_t*)HTMMAP_FIND(fragmentedMapPtr, (void*)flowId);

            if (fragmentListPtr == NULL) {

                fragmentListPtr = HTMLIST_ALLOC(&packet_compareFragmentId);
                assert(fragmentListPtr);
                status = HTMLIST_INSERT(fragmentListPtr, (void*)packetPtr);
                assert(status);
                status = HTMMAP_INSERT(fragmentedMapPtr,
                                      (void*)flowId,
                                      (void*)fragmentListPtr);
                assert(status);

            } else {
                HTM_RETRY(tsx_status, tsx_begin);

                list_iter_t it;
                HTMLIST_ITER_RESET(&it, fragmentListPtr);
                assert(HTMLIST_ITER_HASNEXT(&it, fragmentListPtr));
                packet_t* firstFragmentPtr =
                    (packet_t*)HTMLIST_ITER_NEXT(&it, fragmentListPtr);
                long expectedNumFragment = firstFragmentPtr->numFragment;

                if (numFragment != expectedNumFragment) {
                    status = HTMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                    assert(status);
                    rv = ERROR_NUMFRAGMENT;
                    goto htm_out;
                }

                status = HTMLIST_INSERT(fragmentListPtr, (void*)packetPtr);
                assert(status);

                /*
                 * If we have all the fragments we can reassemble them
                 */

                if (HTMLIST_GETSIZE(fragmentListPtr) == numFragment) {
                    long numByte = 0;
                    long i = 0;
                    HTMLIST_ITER_RESET(&it, fragmentListPtr);
                    while (HTMLIST_ITER_HASNEXT(&it, fragmentListPtr)) {
                        packet_t* fragmentPtr =
                            (packet_t*)HTMLIST_ITER_NEXT(&it, fragmentListPtr);
                        assert(fragmentPtr->flowId == flowId);
                        if (fragmentPtr->fragmentId != i) {
                            status = HTMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                            assert(status);
                            rv = ERROR_INCOMPLETE;
                            goto htm_out; /* should be sequential */
                        }
                        numByte += fragmentPtr->length;
                        i++;
                    }

                    data = (char*)HTM_MALLOC(numByte + 1);
                    assert(data);
                    data[numByte] = '\0';
                    char* dst = data;
                    HTMLIST_ITER_RESET(&it, fragmentListPtr);
                    while (HTMLIST_ITER_HASNEXT(&it, fragmentListPtr)) {
                        packet_t* fragmentPtr =
                            (packet_t*)HTMLIST_ITER_NEXT(&it, fragmentListPtr);
                        memcpy(dst, fragmentPtr->data, fragmentPtr->length);
                        dst += fragmentPtr->length;
                    }
                    assert(dst == data + numByte);

                    HTMLIST_FREE(fragmentListPtr, NULL);
                    status = HTMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                    assert(status);

                    goto restore;
                }

            }

        } else {

            /*
             * This is the only fragment, so it is ready
             */

            if (fragmentId != 0) {
                rv = ERROR_FRAGMENTID;
                goto htm_out;
            }

            data = (char*)HTM_MALLOC(length + 1);
            assert(data);
            data[length] = '\0';
            memcpy(data, packetPtr->data, length);

            decoded_t* decodedPtr = (decoded_t*)HTM_MALLOC(sizeof(decoded_t));
            assert(decodedPtr);
            decodedPtr->flowId = flowId;
            decodedPtr->data = data;

            queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
            status = HTMQUEUE_PUSH(decodedQueuePtr, (void*)decodedPtr);
            assert(status);
        }

        rv = ERROR_NONE;
htm_out:;
        HTM_END(global_tsx_status);
    } else {
#if !defined(ORIGINAL) && defined(MERGE_DECODER)
# ifdef MERGE_LIST
        stm_merge_t merge(stm_merge_context_t *params) {
            extern const stm_op_id_t LIST_GETSZ, LIST_IT_NEXT;
            const stm_op_id_t prev_op = params->leaf ? STM_INVALID_OPID : stm_get_op_opcode(params->previous);

#ifdef TM_DEBUG
            printf("\nDEC_PROCESS_JIT addr:%p decoderPtr:%p bytes:%p numByte:%ld\n", params->addr, decoderPtr, bytes, numByte);
#endif

            /* Update from LIST_GETSZ or LIST_IT_NEXT */
            if (!params->leaf) {
                if (STM_SAME_OPID(prev_op, LIST_IT_NEXT)) {
# ifdef TM_DEBUG
                    printf("DEC_PROCESS_JIT <- LIST_IT_NEXT %p\n", params->conflict.result.ptr);
# endif
                    packet_t *firstFragmentPtr = params->conflict.result.ptr;

                    /* New packet has the same number of fragments */
                    if (numFragment == firstFragmentPtr->numFragment)
                        return STM_MERGE_OK;
                } else if (STM_SAME_OPID(prev_op, LIST_GETSZ)) {
# ifdef TM_DEBUG
                    printf("DEC_PROCESS_JIT <- LIST_GETSZ %ld\n", params->conflict.result.sint);
# endif
                    const long oldFragment = params->conflict.previous_result.sint;
                    ASSERT(params->conflict.result.sint > oldFragment);
                    ASSERT(oldFragment != numFragment);

                    /* Check if the new list size satisfies the reassembly criterion */
                    if (oldFragment == numFragment)
                        return STM_MERGE_ABORT;
                    else if (params->conflict.result.sint == numFragment) {
                        ASSERT(stm_committing());
                        TM_FINISH_MERGE();
                        goto assemble;
                    }

                    return STM_MERGE_OK;
                }
            }

            return STM_MERGE_UNSUPPORTED;
        }
# endif /* MERGE_LIST */
#endif /* !ORIGINAL && MERGE_DECODER */

        STM_HTM_LOCK_SET();
        TM_BEGIN();
#if !defined(ORIGINAL) && defined(MERGE_DECODER)
        TM_LOG_BEGIN(DEC_PROCESS,
# ifdef MERGE_LIST
            merge
# else
            NULL
# endif /* MERGE_LIST */
            , decoderPtr, bytes, numByte);
#endif /* !ORIGINAL && MERGE_DECODER */

#if 0
        /*
         * With the above checks, this one is redundant
         */
        if (numFragment < 1) {
            return ERROR_NUMFRAGMENT;
        }
#endif

        /*
         * Add to fragmented map for reassembling
         */

        if (numFragment > 1) {

            MAP_T* fragmentedMapPtr = decoderPtr->fragmentedMapPtr;
            list_t* fragmentListPtr =
                (list_t*)TMMAP_FIND(fragmentedMapPtr, (void*)flowId);

            if (fragmentListPtr == NULL) {

                fragmentListPtr = TMLIST_ALLOC(&packet_compareFragmentId);
                assert(fragmentListPtr);
                status = TMLIST_INSERT(fragmentListPtr, (void*)packetPtr);
                assert(status);
                status = TMMAP_INSERT(fragmentedMapPtr,
                                      (void*)flowId,
                                      (void*)fragmentListPtr);
                assert(status);

            } else {

                list_iter_t it;
                TMLIST_ITER_RESET(&it, fragmentListPtr);
                assert(TMLIST_ITER_HASNEXT(&it, fragmentListPtr));
                packet_t* firstFragmentPtr =
                    (packet_t*)TMLIST_ITER_NEXT(&it, fragmentListPtr);
                long expectedNumFragment = firstFragmentPtr->numFragment;

                if (numFragment != expectedNumFragment) {
                    status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                    assert(status);
                    rv = ERROR_NUMFRAGMENT;
                    goto stm_out;
                }

                status = TMLIST_INSERT(fragmentListPtr, (void*)packetPtr);
                assert(status);

                /*
                 * If we have all the fragments we can reassemble them
                 */

                if (TMLIST_GETSIZE(fragmentListPtr) == numFragment) {
assemble:;
                    long numByte = 0;
                    long i = 0;
                    TMLIST_ITER_RESET(&it, fragmentListPtr);
                    while (TMLIST_ITER_HASNEXT(&it, fragmentListPtr)) {
                        packet_t* fragmentPtr =
                            (packet_t*)TMLIST_ITER_NEXT(&it, fragmentListPtr);
                        assert(fragmentPtr->flowId == flowId);
                        if (fragmentPtr->fragmentId != i) {
                            status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                            assert(status);
                            rv = ERROR_INCOMPLETE;
                            goto stm_out; /* should be sequential */
                        }
                        numByte += fragmentPtr->length;
                        i++;
                    }

                    data = (char*)TM_MALLOC(numByte + 1);
                    assert(data);
                    data[numByte] = '\0';
                    char* dst = data;
                    TMLIST_ITER_RESET(&it, fragmentListPtr);
                    while (TMLIST_ITER_HASNEXT(&it, fragmentListPtr)) {
                        packet_t* fragmentPtr =
                            (packet_t*)TMLIST_ITER_NEXT(&it, fragmentListPtr);
                        Pmemcpy(dst, fragmentPtr->data, fragmentPtr->length);
                        dst += fragmentPtr->length;
                    }
                    assert(dst == data + numByte);

                    TMLIST_FREE(fragmentListPtr, NULL);
                    status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                    assert(status);

                    goto restore;
                }

            }

        } else {

            /*
             * This is the only fragment, so it is ready
             */

            if (fragmentId != 0) {
                rv = ERROR_FRAGMENTID;
                goto stm_out;
            }

            data = (char*)TM_MALLOC(length + 1);
            assert(data);
            data[length] = '\0';
            Pmemcpy(data, packetPtr->data, length);

restore:;
            decoded_t* decodedPtr = (decoded_t*)TM_MALLOC(sizeof(decoded_t));

#if !defined(ORIGINAL) && defined(STM_HTM)
            STM_HTM_TX_INIT;
            if (STM_HTM_BEGIN()) {
stm_tsx_begin:
                STM_HTM_START(tsx_status, global_tsx_status);
                if (STM_HTM_STARTED(tsx_status)) {
                    STM_HTM_LOCK_READ();
                    assert(decodedPtr);
                    decodedPtr->flowId = flowId;
                    decodedPtr->data = data;

                    queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
                    tsx_status = HTMQUEUE_PUSH(decodedQueuePtr, (void*)decodedPtr);
                    assert(tsx_status);

                    STM_HTM_END(global_tsx_status);
                    STM_HTM_EXIT();
                    goto done;
                } else {
                    HTM_RETRY(tsx_status, stm_tsx_begin);
                    STM_HTM_EXIT();
                }
            }
#endif /* !ORIGINAL && STM_HTM */

push:;
            assert(decodedPtr);
            decodedPtr->flowId = flowId;
            decodedPtr->data = data;

            queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
            status = TMQUEUE_PUSH(decodedQueuePtr, (void*)decodedPtr);
            assert(status);
        }

done:
        rv = ERROR_NONE;
stm_out:;
        /* Since the merge function remains in scope, do not explicitly end the operation; it will be done implicitly when the transaction ends */
        // TM_LOG_END(DEC_PROCESS, &rv);
        TM_END();
        STM_HTM_LOCK_UNSET();
    }

out:
    return rv;
}


/* =============================================================================
 * decoder_getComplete
 * -- If none, returns NULL
 * =============================================================================
 */
char*
decoder_getComplete (decoder_t* decoderPtr, long* decodedFlowIdPtr)
{
    char* data;
    decoded_t* decodedPtr = queue_pop(decoderPtr->decodedQueuePtr);

    if (decodedPtr) {
        *decodedFlowIdPtr = decodedPtr->flowId;
        data = decodedPtr->data;
        free(decodedPtr);
    } else {
        *decodedFlowIdPtr = -1;
        data = NULL;
    }

    return data;
}


/* =============================================================================
 * HTMdecoder_getComplete
 * -- If none, returns NULL
 * =============================================================================
 */
char*
HTMdecoder_getComplete (decoder_t* decoderPtr, long* decodedFlowIdPtr)
{
    char* data;
    decoded_t* decodedPtr = HTMQUEUE_POP(decoderPtr->decodedQueuePtr);

    if (decodedPtr) {
        *decodedFlowIdPtr = HTM_SHARED_READ(decodedPtr->flowId);
        data = HTM_SHARED_READ_P(decodedPtr->data);
        HTM_FREE(decodedPtr);
    } else {
        *decodedFlowIdPtr = -1;
        data = NULL;
    }

    return data;
}


/* =============================================================================
 * TMdecoder_getComplete
 * -- If none, returns NULL
 * =============================================================================
 */
TM_CALLABLE
char*
TMdecoder_getComplete (TM_ARGDECL  decoder_t* decoderPtr, long* decodedFlowIdPtr)
{
    __label__ read;
    char* data;
    decoded_t* decodedPtr;

#if !defined(ORIGINAL) && defined(MERGE_DECODER)
# ifdef MERGE_QUEUE
    stm_merge_t merge(stm_merge_context_t *params) {
        extern const stm_op_id_t QUEUE_POP;

        /* Just-in-time merge */
        ASSERT(STM_SAME_OP(stm_get_current_op(), params->current));

        /* Update from QUEUE_POP to DEC_GETCOMPLETE */
        if (!params->leaf && STM_SAME_OPID(stm_get_op_opcode(params->current), DEC_GETCOMPLETE) && STM_SAME_OPID(stm_get_op_opcode(params->previous), QUEUE_POP)) {

#ifdef TM_DEBUG
            printf("\nDEC_GETCOMPLETE_JIT addr:%p decoderPtr:%p decodedFlowIdPtr:%p\n", params->addr, decoderPtr, decodedFlowIdPtr);
            printf("\nDEC_GETCOMPLETE_JIT <- QUEUE_POP %p\n", params->conflict.result.ptr);
#endif
            if (decodedPtr) {
                stm_read_t r = TM_SHARED_DID_READ(decodedPtr->flowId);
                if (STM_VALID_READ(r)) {
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));

                    r = TM_SHARED_DID_READ(decodedPtr->data);
                    if (STM_VALID_READ(r)) {
                        ASSERT_FAIL(TM_SHARED_UNDO_READ(r));

                        stm_free_t f = TM_DID_FREE(decodedPtr);
                        if (STM_VALID_FREE(f))
                            TM_UNDO_FREE(f);
                    }
                }
            }

            decodedPtr = params->conflict.result.ptr;
            TM_FINISH_MERGE();
            goto read;
        }

#ifdef TM_DEBUG
        printf("\nDEC_GETCOMPLETE_JIT UNSUPPORTED addr:%p\n", params->addr);
#endif
        return STM_MERGE_UNSUPPORTED;
    }
# endif /* MERGE_QUEUE */

    TM_LOG_BEGIN(DEC_GETCOMPLETE,
# ifdef MERGE_QUEUE
        merge
# else
        NULL
# endif /* MERGE_QUEUE */
        , decoderPtr, decodedFlowIdPtr);
#endif /* !ORIGINAL && MERGE_DECODER */

    decodedPtr = TMQUEUE_POP(decoderPtr->decodedQueuePtr);

read:
    if (decodedPtr) {
        *decodedFlowIdPtr = TM_SHARED_READ_TAG(decodedPtr->flowId, (uintptr_t)decodedPtr);
        data = TM_SHARED_READ_TAG_P(decodedPtr->data, (uintptr_t)decodedPtr);
        TM_FREE(decodedPtr);
    } else {
        *decodedFlowIdPtr = -1;
        data = NULL;
    }

#if !defined(ORIGINAL) && defined(MERGE_DECODER)
    TM_LOG_END(DEC_GETCOMPLETE, &data);
#endif /* !ORIGINAL && MERGE_DECODER */
    return data;
}


/* #############################################################################
 * TEST_DECODER
 * #############################################################################
 */
#ifdef TEST_DECODER

#include <stdio.h>


int
main ()
{
    decoder_t* decoderPtr;

    puts("Starting...");

    decoderPtr = decoder_alloc();
    assert(decoderPtr);

    long numDataByte = 3;
    long numPacketByte = PACKET_HEADER_LENGTH + numDataByte;

    char* abcBytes = (char*)malloc(numPacketByte);
    assert(abcBytes);
    packet_t* abcPacketPtr;
    abcPacketPtr = (packet_t*)abcBytes;
    abcPacketPtr->flowId = 1;
    abcPacketPtr->fragmentId = 0;
    abcPacketPtr->numFragment = 2;
    abcPacketPtr->length = numDataByte;
    abcPacketPtr->data[0] = 'a';
    abcPacketPtr->data[1] = 'b';
    abcPacketPtr->data[2] = 'c';

    char* defBytes = (char*)malloc(numPacketByte);
    assert(defBytes);
    packet_t* defPacketPtr;
    defPacketPtr = (packet_t*)defBytes;
    defPacketPtr->flowId = 1;
    defPacketPtr->fragmentId = 1;
    defPacketPtr->numFragment = 2;
    defPacketPtr->length = numDataByte;
    defPacketPtr->data[0] = 'd';
    defPacketPtr->data[1] = 'e';
    defPacketPtr->data[2] = 'f';

    assert(decoder_process(decoderPtr, abcBytes, numDataByte) == ERROR_SHORT);

    abcPacketPtr->flowId = -1;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_FLOWID);
    abcPacketPtr->flowId = 1;

    abcPacketPtr->fragmentId = -1;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_FRAGMENTID);
    abcPacketPtr->fragmentId = 0;

    abcPacketPtr->fragmentId = 2;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_FRAGMENTID);
    abcPacketPtr->fragmentId = 0;

    abcPacketPtr->fragmentId = 2;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_FRAGMENTID);
    abcPacketPtr->fragmentId = 0;

    abcPacketPtr->length = -1;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_LENGTH);
    abcPacketPtr->length = numDataByte;

    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_NONE);
    defPacketPtr->numFragment = 3;
    assert(decoder_process(decoderPtr, defBytes, numPacketByte) == ERROR_NUMFRAGMENT);
    defPacketPtr->numFragment = 2;

    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_NONE);
    defPacketPtr->fragmentId = 0;
    assert(decoder_process(decoderPtr, defBytes, numPacketByte) == ERROR_INCOMPLETE);
    defPacketPtr->fragmentId = 1;

    long flowId;
    assert(decoder_process(decoderPtr, defBytes, numPacketByte) == ERROR_NONE);
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_NONE);
    char* str = decoder_getComplete(decoderPtr, &flowId);
    assert(strcmp(str, "abcdef") == 0);
    free(str);
    assert(flowId == 1);

    abcPacketPtr->numFragment = 1;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_NONE);
    str = decoder_getComplete(decoderPtr, &flowId);
    assert(strcmp(str, "abc") == 0);
    free(str);
    abcPacketPtr->numFragment = 2;
    assert(flowId == 1);

    str = decoder_getComplete(decoderPtr, &flowId);
    assert(str == NULL);
    assert(flowId == -1);

    decoder_free(decoderPtr);

    free(abcBytes);
    free(defBytes);

    puts("All tests passed.");

    return 0;
}


#endif /* TEST_DECODER */


/* =============================================================================
 *
 * End of decoder.c
 *
 * =============================================================================
 */

#if !defined(ORIGINAL) && defined(MERGE_DECODER)
# ifdef MERGE_QUEUE
stm_merge_t TMdecoder_merge(stm_merge_context_t *params) {
    extern const stm_op_id_t QUEUE_POP;

    /* Delayed merge only */
    ASSERT(!STM_SAME_OP(stm_get_current_op(), params->current));

    const stm_union_t *args;
    const ssize_t nargs = stm_get_op_args(params->current, &args);

    /* Update from QUEUE_POP to DEC_GETCOMPLETE */
    if (!params->leaf && STM_SAME_OPID(stm_get_op_opcode(params->current), DEC_GETCOMPLETE) && STM_SAME_OPID(stm_get_op_opcode(params->previous), QUEUE_POP)) {
        if (nargs != 2)
            return STM_MERGE_ABORT;
        const decoder_t* decoderPtr = args[0].ptr;
        long *decodedFlowIdPtr = args[1].ptr;

        stm_read_t r;

#ifdef TM_DEBUG
        printf("\nDEC_GETCOMPLETE addr:%p decoderPtr:%p decodedFlowIdPtr:%p\n", params->addr, decoderPtr, decodedFlowIdPtr);
#endif

#ifdef TM_DEBUG
        printf("DEC_GETCOMPLETE <- QUEUE_POP %p\n", params->conflict.result.ptr);
#endif
        char *rv = params->rv.ptr;

        decoded_t *oldDecodedPtr = params->conflict.previous_result.ptr;
        decoded_t *decodedPtr = params->conflict.result.ptr;

        if (oldDecodedPtr != decodedPtr) {
            if (oldDecodedPtr) {
                r = TM_SHARED_DID_READ(oldDecodedPtr->flowId);
                ASSERT(STM_VALID_READ(r));
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));

                r = TM_SHARED_DID_READ(oldDecodedPtr->data);
                ASSERT(STM_VALID_READ(r));
                ASSERT_FAIL(TM_SHARED_UNDO_READ(r));

                stm_free_t f = TM_DID_FREE(oldDecodedPtr);
                ASSERT(STM_VALID_FREE(f));
                ASSERT_FAIL(TM_UNDO_FREE(f));
            }

            if (decodedPtr) {
                ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(decodedPtr->flowId)));
                *decodedFlowIdPtr = TM_SHARED_READ_TAG(decodedPtr->flowId, (uintptr_t)decodedPtr);
                ASSERT(!STM_VALID_READ(TM_SHARED_DID_READ(decodedPtr->data)));
                rv = TM_SHARED_READ_TAG_P(decodedPtr->data, (uintptr_t)decodedPtr);
                ASSERT(!STM_VALID_FREE(TM_DID_FREE(decodedPtr)));
                TM_FREE(decodedPtr);
            } else {
                *decodedFlowIdPtr = -1;
                rv = NULL;
            }
        }

#ifdef TM_DEBUG
        printf("DEC_GETCOMPLETE rv(old):%p rv(new):%p\n", params->rv.ptr, rv);
#endif
        params->rv.ptr = rv;
        return STM_MERGE_OK;
    }

#ifdef TM_DEBUG
    printf("\nDEC_MERGE UNSUPPORTED addr:%p\n", params->addr);
#endif
    return STM_MERGE_UNSUPPORTED;
}
# define TMDECODER_MERGE TMdecoder_merge
# else
# define TMDECODER_MERGE NULL
# endif /* MERGE_QUEUE */

__attribute__((constructor)) void decoder_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*pp[], {&ffi_type_pointer, &ffi_type_pointer});
    TM_LOG_TYPE_DECLARE_INIT(*ppl[], {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_slong});
    #define TM_LOG_OP TM_LOG_OP_INIT
    #include "decoder.inc"
    #undef TM_LOG_OP
}
#endif /* !ORIGINAL && MERGE_DECODER */
