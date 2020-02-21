/* =============================================================================
 *
 * rbtree.c
 * -- Red-black balanced binary search tree
 *
 * =============================================================================
 *
 * Copyright (C) Sun Microsystems Inc., 2006.  All Rights Reserved.
 * Authors: Dave Dice, Nir Shavit, Ori Shalev.
 *
 * STM: Transactional Locking for Disjoint Access Parallelism
 *
 * Transactional Locking II,
 * Dave Dice, Ori Shalev, Nir Shavit
 * DISC 2006, Sept 2006, Stockholm, Sweden.
 *
 * =============================================================================
 *
 * Modified by Chi Cao Minh, Aug 2006
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "memory.h"
#include "rbtree.h"
#include "tm.h"


typedef struct node {
    void* k;
    void* v;
    struct node* p;
    struct node* l;
    struct node* r;
    long c;
} node_t;


struct rbtree {
    node_t* root;
    TM_PURE long (*compare)(const void*, const void*);   /* returns {-1,0,1}, 0 -> equal */
};

#define LDA(a)              *(a)
#define STA(a,v)            *(a) = (v)
#define LDV(a)              (a)
#define STV(a,v)            (a) = (v)
#define LDF(o,f)            ((o)->f)
#define STF(o,f,v)          ((o)->f) = (v)
#define LDNODE(o,f)         ((node_t*)(LDF((o),f)))

#define HTX_LDA(a)          HTM_SHARED_READ(*(a))
#define HTX_STA(a,v)        HTM_SHARED_WRITE(*(a), v)
#define HTX_LDV(a)          HTM_SHARED_READ(a)
#define HTX_STV(a,v)        HTM_SHARED_WRITE_P(a, v)
#define HTX_LDF(o,f)        ((long)HTM_SHARED_READ((o)->f))
#define HTX_LDF_P(o,f)      ((void*)HTM_SHARED_READ_P((o)->f))
#define HTX_STF(o,f,v)      HTM_SHARED_WRITE((o)->f, v)
#define HTX_STF_P(o,f,v)    HTM_SHARED_WRITE_P((o)->f, v)
#define HTX_LDNODE(o,f)     ((node_t*)(HTX_LDF_P((o),f)))

#define TX_LDA(a)           TM_SHARED_READ(*(a))
#define TX_STA(a,v)         TM_SHARED_WRITE(*(a), v)
#define TX_LDV(a)           TM_SHARED_READ(a)
#define TX_STV(a,v)         TM_SHARED_WRITE_P(a, v)
#define TX_LDF(o,f)         ((long)TM_SHARED_READ_TAG((o)->f, (uintptr_t)o))
#define TX_LDF_P(o,f)       ((void*)TM_SHARED_READ_TAG_P((o)->f, (uintptr_t)o))
#define TX_STF(o,f,v)       TM_SHARED_WRITE((o)->f, v)
#define TX_STF_P(o,f,v)     TM_SHARED_WRITE_P((o)->f, v)
#define TX_LDNODE(o,f)      ((node_t*)(TX_LDF_P((o),f)))

#ifndef ORIGINAL
# define TM_LOG_OP TM_LOG_OP_DECLARE
# include "rbtree.inc"
# undef TM_LOG_OP

#define TMRBTREE_BAD_ADDR                 (void *)-1
#endif /* ORIGINAL */

/* =============================================================================
 * DECLARATION OF TM_CALLABLE FUNCTIONS
 * =============================================================================
 */

TM_CALLABLE
static node_t*
TMlookup (TM_ARGDECL  rbtree_t* s, void* k);

TM_CALLABLE
static void
TMrotateLeft (TM_ARGDECL  rbtree_t* s, node_t* x);

TM_CALLABLE
static void
TMrotateRight (TM_ARGDECL  rbtree_t* s, node_t* x);

TM_CALLABLE
static inline node_t*
TMparentOf (TM_ARGDECL  node_t* n);

TM_CALLABLE
static inline node_t*
TMleftOf (TM_ARGDECL  node_t* n);

TM_CALLABLE
static inline node_t*
TMrightOf (TM_ARGDECL  node_t* n);

TM_CALLABLE
static inline long
TMcolorOf (TM_ARGDECL  node_t* n);

TM_CALLABLE
static inline void
TMsetColor (TM_ARGDECL  node_t* n, long c);

TM_CALLABLE
static void
TMfixAfterInsertion (TM_ARGDECL  rbtree_t* s, node_t* x);

TM_CALLABLE
static node_t*
TMsuccessor  (TM_ARGDECL  node_t* t);

TM_CALLABLE
static void
TMfixAfterDeletion  (TM_ARGDECL  rbtree_t* s, node_t*  x);

TM_CALLABLE
static node_t*
TMinsert (TM_ARGDECL  rbtree_t* s, void* k, void* v, node_t* n);

TM_CALLABLE
static node_t*
TMgetNode (TM_ARGDECL_ALONE);

TM_CALLABLE
static node_t*
TMdelete (TM_ARGDECL  rbtree_t* s, node_t* p);

enum {
    RED   = 0,
    BLACK = 1
};


/*
 * See also:
 * - Doug Lea's j.u.TreeMap
 * - Keir Fraser's rb_stm.c and rb_lock_serialisedwriters.c in libLtx.
 *
 * Following Doug Lea's TreeMap example, we avoid the use of the magic
 * "nil" sentinel pointers.  The sentinel is simply a convenience and
 * is not fundamental to the algorithm.  We forgo the sentinel as
 * it is a source of false+ data conflicts in transactions.  Relatedly,
 * even with locks, use of a nil sentil can result in considerable
 * cache coherency traffic on traditional SMPs.
 */


/* =============================================================================
 * lookup
 * =============================================================================
 */
static node_t*
lookup (rbtree_t* s, void* k)
{
    node_t* p = LDNODE(s, root);

    while (p != NULL) {
        long cmp = s->compare(k, LDF(p, k));
        if (cmp == 0) {
            return p;
        }
        p = ((cmp < 0) ? LDNODE(p, l) : LDNODE(p, r));
    }

    return NULL;
}
#define LOOKUP(set, key)  lookup(set, key)


/* =============================================================================
 * HTMlookup
 * =============================================================================
 */
static node_t*
HTMlookup (rbtree_t* s, void* k)
{
    node_t* p = HTX_LDNODE(s, root);

    while (p != NULL) {
        long cmp = s->compare(k, HTX_LDF_P(p, k));
        if (cmp == 0)
            goto out;
        p = ((cmp < 0) ? HTX_LDNODE(p, l) : HTX_LDNODE(p, r));
    }

out:
    return p;
}
#define HTX_LOOKUP(set, key)  HTMlookup(set, key)


/* =============================================================================
 * TMlookup
 * =============================================================================
 */
TM_CALLABLE
static node_t*
TMlookup (TM_ARGDECL  rbtree_t* s, void* k)
{
#if !defined(ORIGINAL) && defined(MERGE_RBTREE)
    __label__ replay;
#endif /* !ORIGINAL && MERGE_RBTREE */

    node_t* p;

#ifndef ORIGINAL
# ifdef MERGE_RBTREE
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Conflict occurred directly inside RBTREE_LOOKUP */
        ASSERT(params->leaf == 1);
        ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->current), RBTREE_LOOKUP));

# ifdef TM_DEBUG
        printf("RBTREE_LOOKUP_JIT s:%p key:%p rv:%p\n", s, k, params->rv.ptr);
# endif
        p = params->rv.ptr;

        TM_FINISH_MERGE();
        goto replay;
    }
# endif /* MERGE_RBTREE */
    TM_LOG_BEGIN(RBTREE_LOOKUP,
# ifdef MERGE_RBTREE
        merge
# else
        NULL
# endif /* MERGE_RBTREE */
        , s, k);
#endif /* ORIGINAL */

    p = TX_LDNODE(s, root);

    while (p != NULL) {
        long cmp = s->compare(k, TX_LDF_P(p, k));
        if (cmp == 0)
            goto out;
        p = ((cmp < 0) ? TX_LDNODE(p, l) : TX_LDNODE(p, r));
    }

out:
#ifndef ORIGINAL
    TM_LOG_END(RBTREE_LOOKUP, &p);
replay:
#endif /* ORIGINAL */
    return p;
}
#define TX_LOOKUP(set, key)  TMlookup(TM_ARG  set, key)


/*
 * Balancing operations.
 *
 * Implementations of rebalancings during insertion and deletion are
 * slightly different than the CLR version.  Rather than using dummy
 * nilnodes, we use a set of accessors that deal properly with null.  They
 * are used to avoid messiness surrounding nullness checks in the main
 * algorithms.
 *
 * From CLR
 */


/* =============================================================================
 * rotateLeft
 * =============================================================================
 */
static void
rotateLeft (rbtree_t* s, node_t* x)
{
    node_t* r = LDNODE(x, r); /* AKA r, y */
    node_t* rl = LDNODE(r, l);
    STF(x, r, rl);
    if (rl != NULL) {
        STF(rl, p, x);
    }
    /* TODO: compute p = xp = x->p.  Use xp for R-Values in following */
    node_t* xp = LDNODE(x, p);
    STF(r, p, xp);
    if (xp == NULL) {
        STF(s, root, r);
    } else if (LDNODE(xp, l) == x) {
        STF(xp, l, r);
    } else {
        STF(xp, r, r);
    }
    STF(r, l, x);
    STF(x, p, r);
}
#define ROTATE_LEFT(set, node)  rotateLeft(set, node)


/* =============================================================================
 * HTMrotateLeft
 * =============================================================================
 */
static void
HTMrotateLeft (rbtree_t* s, node_t* x)
{
    node_t* r = HTX_LDNODE(x, r); /* AKA r, y */
    node_t* rl = HTX_LDNODE(r, l);
    HTX_STF_P(x, r, rl);
    if (rl != NULL) {
        HTX_STF_P(rl, p, x);
    }
    /* TODO: compute p = xp = x->p.  Use xp for R-Values in following */
    node_t* xp = HTX_LDNODE(x, p);
    HTX_STF_P(r, p, xp);
    if (xp == NULL) {
        HTX_STF_P(s, root, r);
    } else if (HTX_LDNODE(xp, l) == x) {
        HTX_STF_P(xp, l, r);
    } else {
        HTX_STF_P(xp, r, r);
    }
    HTX_STF_P(r, l, x);
    HTX_STF_P(x, p, r);
}
#define HTX_ROTATE_LEFT(set, node)  HTMrotateLeft(set, node)


/* =============================================================================
 * TMrotateLeft
 * =============================================================================
 */
TM_CALLABLE
static void
TMrotateLeft (TM_ARGDECL  rbtree_t* s, node_t* x)
{
    node_t* r = TX_LDNODE(x, r); /* AKA r, y */
    node_t* rl = TX_LDNODE(r, l);
    TX_STF_P(x, r, rl);
    if (rl != NULL) {
        TX_STF_P(rl, p, x);
    }
    /* TODO: compute p = xp = x->p.  Use xp for R-Values in following */
    node_t* xp = TX_LDNODE(x, p);
    TX_STF_P(r, p, xp);
    if (xp == NULL) {
        TX_STF_P(s, root, r);
    } else if (TX_LDNODE(xp, l) == x) {
        TX_STF_P(xp, l, r);
    } else {
        TX_STF_P(xp, r, r);
    }
    TX_STF_P(r, l, x);
    TX_STF_P(x, p, r);
}
#define TX_ROTATE_LEFT(set, node)  TMrotateLeft(TM_ARG  set, node)


/* =============================================================================
 * rotateRight
 * =============================================================================
 */
static void
rotateRight (rbtree_t* s, node_t* x)
{
    node_t* l = LDNODE(x, l); /* AKA l,y */
    node_t* lr = LDNODE(l, r);
    STF(x, l, lr);
    if (lr != NULL) {
        STF(lr, p, x);
    }
    node_t* xp = LDNODE(x, p);
    STF(l, p, xp);
    if (xp == NULL) {
        STF(s, root, l);
    } else if (LDNODE(xp, r) == x) {
        STF(xp, r, l);
    } else {
        STF(xp, l, l);
    }
    STF(l, r, x);
    STF(x, p, l);
}
#define ROTATE_RIGHT(set, node)  rotateRight(set, node)


/* =============================================================================
 * HTMrotateRight
 * =============================================================================
 */
static void
HTMrotateRight (rbtree_t* s, node_t* x)
{
    node_t* l = HTX_LDNODE(x, l); /* AKA l,y */
    node_t* lr = HTX_LDNODE(l, r);
    HTX_STF_P(x, l, lr);
    if (lr != NULL) {
        HTX_STF_P(lr, p, x);
    }
    node_t* xp = HTX_LDNODE(x, p);
    HTX_STF_P(l, p, xp);
    if (xp == NULL) {
        HTX_STF_P(s, root, l);
    } else if (HTX_LDNODE(xp, r) == x) {
        HTX_STF_P(xp, r, l);
    } else {
        HTX_STF_P(xp, l, l);
    }
    HTX_STF_P(l, r, x);
    HTX_STF_P(x, p, l);
}
#define HTX_ROTATE_RIGHT(set, node)  HTMrotateRight(set, node)


/* =============================================================================
 * TMrotateRight
 * =============================================================================
 */
TM_CALLABLE
static void
TMrotateRight (TM_ARGDECL  rbtree_t* s, node_t* x)
{
    node_t* l = TX_LDNODE(x, l); /* AKA l,y */
    node_t* lr = TX_LDNODE(l, r);
    TX_STF_P(x, l, lr);
    if (lr != NULL) {
        TX_STF_P(lr, p, x);
    }
    node_t* xp = TX_LDNODE(x, p);
    TX_STF_P(l, p, xp);
    if (xp == NULL) {
        TX_STF_P(s, root, l);
    } else if (TX_LDNODE(xp, r) == x) {
        TX_STF_P(xp, r, l);
    } else {
        TX_STF_P(xp, l, l);
    }
    TX_STF_P(l, r, x);
    TX_STF_P(x, p, l);
}
#define TX_ROTATE_RIGHT(set, node)  TMrotateRight(TM_ARG  set, node)


/* =============================================================================
 * parentOf
 * =============================================================================
 */
static inline node_t*
parentOf (node_t* n)
{
   return (n ? LDNODE(n,p) : NULL);
}
#define PARENT_OF(n) parentOf(n)


/* =============================================================================
 * HTMparentOf
 * =============================================================================
 */
static inline node_t*
HTMparentOf (node_t* n)
{
   return (n ? HTX_LDNODE(n,p) : NULL);
}
#define HTX_PARENT_OF(n)  HTMparentOf(TM_ARG  n)


/* =============================================================================
 * TMparentOf
 * =============================================================================
 */
TM_CALLABLE
static inline node_t*
TMparentOf (TM_ARGDECL  node_t* n)
{
   return (n ? TX_LDNODE(n,p) : NULL);
}
#define TX_PARENT_OF(n)  TMparentOf(TM_ARG  n)


/* =============================================================================
 * leftOf
 * =============================================================================
 */
static inline node_t*
leftOf (node_t* n)
{
   return (n ? LDNODE(n, l) : NULL);
}
#define LEFT_OF(n)  leftOf(n)


/* =============================================================================
 * HTMleftOf
 * =============================================================================
 */
static inline node_t*
HTMleftOf (node_t* n)
{
   return (n ? HTX_LDNODE(n, l) : NULL);
}
#define HTX_LEFT_OF(n)  HTMleftOf(n)

/* =============================================================================
 * TMleftOf
 * =============================================================================
 */
TM_CALLABLE
static inline node_t*
TMleftOf (TM_ARGDECL  node_t* n)
{
   return (n ? TX_LDNODE(n, l) : NULL);
}
#define TX_LEFT_OF(n)  TMleftOf(TM_ARG  n)


/* =============================================================================
 * rightOf
 * =============================================================================
 */
static inline node_t*
rightOf (node_t* n)
{
    return (n ? LDNODE(n, r) : NULL);
}
#define RIGHT_OF(n)  rightOf(n)


/* =============================================================================
 * HTMrightOf
 * =============================================================================
 */
static inline node_t*
HTMrightOf (node_t* n)
{
    return (n ? HTX_LDNODE(n, r) : NULL);
}
#define HTX_RIGHT_OF(n)  HTMrightOf(n)


/* =============================================================================
 * TMrightOf
 * =============================================================================
 */
TM_CALLABLE
static inline node_t*
TMrightOf (TM_ARGDECL  node_t* n)
{
    return (n ? TX_LDNODE(n, r) : NULL);
}
#define TX_RIGHT_OF(n)  TMrightOf(TM_ARG  n)


/* =============================================================================
 * colorOf
 * =============================================================================
 */
static inline long
colorOf (node_t* n)
{
    return (n ? (long)LDNODE(n, c) : BLACK);
}
#define COLOR_OF(n)  colorOf(n)


/* =============================================================================
 * HTMcolorOf
 * =============================================================================
 */
static inline long
HTMcolorOf (node_t* n)
{
    return (n ? (long)HTX_LDF(n, c) : BLACK);
}
#define HTX_COLOR_OF(n)  HTMcolorOf(n)


/* =============================================================================
 * TMcolorOf
 * =============================================================================
 */
TM_CALLABLE
static inline long
TMcolorOf (TM_ARGDECL  node_t* n)
{
    return (n ? (long)TX_LDF(n, c) : BLACK);
}
#define TX_COLOR_OF(n)  TMcolorOf(TM_ARG  n)


/* =============================================================================
 * setColor
 * =============================================================================
 */
static inline void
setColor (node_t* n, long c)
{
    if (n != NULL) {
        STF(n, c, c);
    }
}
#define SET_COLOR(n, c)  setColor(n, c)


/* =============================================================================
 * HTMsetColor
 * =============================================================================
 */
static inline void
HTMsetColor (node_t* n, long c)
{
    if (n != NULL) {
        HTX_STF(n, c, c);
    }
}
#define HTX_SET_COLOR(n, c)  HTMsetColor(n, c)


/* =============================================================================
 * TMsetColor
 * =============================================================================
 */
TM_CALLABLE
static inline void
TMsetColor (TM_ARGDECL  node_t* n, long c)
{
    if (n != NULL) {
        TX_STF(n, c, c);
    }
}
#define TX_SET_COLOR(n, c)  TMsetColor(TM_ARG  n, c)


/* =============================================================================
 * fixAfterInsertion
 * =============================================================================
 */
static void
fixAfterInsertion (rbtree_t* s, node_t* x)
{
    STF(x, c, RED);
    while (x != NULL && x != LDNODE(s, root)) {
        node_t* xp = PARENT_OF(x);
        if (LDF(xp, c) != RED) {
            break;
        }
        node_t *g = PARENT_OF(xp);
        if (xp == LEFT_OF(g)) {
            node_t* y = RIGHT_OF(g);
            if (COLOR_OF(y) == RED) {
                SET_COLOR(xp, BLACK);
                SET_COLOR(y, BLACK);
                SET_COLOR(g, RED);
                x = g;
            } else {
                if (x == RIGHT_OF(xp)) {
                    x = xp;
                    ROTATE_LEFT(s, x);
                    xp = PARENT_OF(x);
                }
                SET_COLOR(xp, BLACK);
                SET_COLOR(g, RED);
                if (g != NULL) {
                    ROTATE_RIGHT(s, g);
                }
            }
        } else {
            node_t* y = LEFT_OF(g);
            if (COLOR_OF(y) == RED) {
                SET_COLOR(xp, BLACK);
                SET_COLOR(y, BLACK);
                SET_COLOR(g, RED);
                x = g;
            } else {
                if (x == LEFT_OF(xp)) {
                    x = xp;
                    ROTATE_RIGHT(s, x);
                    xp = PARENT_OF(x);
                }
                SET_COLOR(xp, BLACK);
                SET_COLOR(g, RED);
                if (g != NULL) {
                    ROTATE_LEFT(s, g);
                }
            }
        }
    }
    node_t* ro = LDNODE(s, root);
    if (LDF(ro, c) != BLACK) {
        STF(ro, c, BLACK);
    }
}
#define FIX_AFTER_INSERTION(s, x)  fixAfterInsertion(s, x)


/* =============================================================================
 * HTMfixAfterInsertion
 * =============================================================================
 */
static void
HTMfixAfterInsertion (rbtree_t* s, node_t* x)
{
    HTX_STF(x, c, RED);
    while (x != NULL && x != HTX_LDNODE(s, root)) {
        node_t* xp = HTX_PARENT_OF(x);
        if (HTX_LDF(xp, c) != RED) {
            break;
        }
        node_t *g = HTX_PARENT_OF(xp);
        if (xp == HTX_LEFT_OF(g)) {
            node_t* y = HTX_RIGHT_OF(g);
            if (HTX_COLOR_OF(y) == RED) {
                HTX_SET_COLOR(xp, BLACK);
                HTX_SET_COLOR(y, BLACK);
                HTX_SET_COLOR(g, RED);
                x = g;
            } else {
                if (x == HTX_RIGHT_OF(xp)) {
                    x = xp;
                    HTX_ROTATE_LEFT(s, x);
                    xp = HTX_PARENT_OF(x);
                }
                HTX_SET_COLOR(xp, BLACK);
                HTX_SET_COLOR(g, RED);
                if (g != NULL) {
                    HTX_ROTATE_RIGHT(s, g);
                }
            }
        } else {
            node_t* y = HTX_LEFT_OF(g);
            if (HTX_COLOR_OF(y) == RED) {
                HTX_SET_COLOR(xp, BLACK);
                HTX_SET_COLOR(y, BLACK);
                HTX_SET_COLOR(g, RED);
                x = g;
            } else {
                if (x == HTX_LEFT_OF(xp)) {
                    x = xp;
                    HTX_ROTATE_RIGHT(s, x);
                    xp = HTX_PARENT_OF(x);
                }
                HTX_SET_COLOR(xp, BLACK);
                HTX_SET_COLOR(g, RED);
                if (g != NULL) {
                    HTX_ROTATE_LEFT(s, g);
                }
            }
        }
    }
    node_t* ro = HTX_LDNODE(s, root);
    if (HTX_LDF(ro, c) != BLACK) {
        HTX_STF(ro, c, BLACK);
    }
}
#define HTX_FIX_AFTER_INSERTION(s, x)  HTMfixAfterInsertion(s, x)


/* =============================================================================
 * TMfixAfterInsertion
 * =============================================================================
 */
TM_CALLABLE
static void
TMfixAfterInsertion (TM_ARGDECL  rbtree_t* s, node_t* x)
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(RBTREE_FIX_INS, NULL, s, x);
#endif /* ORIGINAL */

    TX_STF(x, c, RED);
    while (x != NULL && x != TX_LDNODE(s, root)) {
        node_t* xp = TX_PARENT_OF(x);
        if (TX_LDF(xp, c) != RED) {
            break;
        }
        node_t *g = TX_PARENT_OF(xp);
        if (xp == TX_LEFT_OF(g)) {
            node_t* y = TX_RIGHT_OF(g);
            if (TX_COLOR_OF(y) == RED) {
                TX_SET_COLOR(xp, BLACK);
                TX_SET_COLOR(y, BLACK);
                TX_SET_COLOR(g, RED);
                x = g;
            } else {
                if (x == TX_RIGHT_OF(xp)) {
                    x = xp;
                    TX_ROTATE_LEFT(s, x);
                    xp = TX_PARENT_OF(x);
                }
                TX_SET_COLOR(xp, BLACK);
                TX_SET_COLOR(g, RED);
                if (g != NULL) {
                    TX_ROTATE_RIGHT(s, g);
                }
            }
        } else {
            node_t* y = TX_LEFT_OF(g);
            if (TX_COLOR_OF(y) == RED) {
                TX_SET_COLOR(xp, BLACK);
                TX_SET_COLOR(y, BLACK);
                TX_SET_COLOR(g, RED);
                x = g;
            } else {
                if (x == TX_LEFT_OF(xp)) {
                    x = xp;
                    TX_ROTATE_RIGHT(s, x);
                    xp = TX_PARENT_OF(x);
                }
                TX_SET_COLOR(xp, BLACK);
                TX_SET_COLOR(g, RED);
                if (g != NULL) {
                    TX_ROTATE_LEFT(s, g);
                }
            }
        }
    }
    node_t* ro = TX_LDNODE(s, root);
    if (TX_LDF(ro, c) != BLACK) {
        TX_STF(ro, c, BLACK);
    }

#ifndef ORIGINAL
    TM_LOG_END(RBTREE_FIX_INS, NULL);
#endif /* ORIGINAL */
}
#define TX_FIX_AFTER_INSERTION(s, x)  TMfixAfterInsertion(TM_ARG  s, x)


/* =============================================================================
 * insert
 * =============================================================================
 */
static node_t*
insert (rbtree_t* s, void* k, void* v, node_t* n)
{
    node_t* t  = LDNODE(s, root);
    if (t == NULL) {
        if (n == NULL) {
            return NULL;
        }
        /* Note: the following STs don't really need to be transactional */
        STF(n, l, NULL);
        STF(n, r, NULL);
        STF(n, p, NULL);
        STF(n, k, k);
        STF(n, v, v);
        STF(n, c, BLACK);
        STF(s, root, n);
        return NULL;
    }

    for (;;) {
        long cmp = s->compare(k, LDF(t, k));
        if (cmp == 0) {
            return t;
        } else if (cmp < 0) {
            node_t* tl = LDNODE(t, l);
            if (tl != NULL) {
                t = tl;
            } else {
                STF(n, l, NULL);
                STF(n, r, NULL);
                STF(n, k, k);
                STF(n, v, v);
                STF(n, p, t);
                STF(t, l, n);
                FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        } else { /* cmp > 0 */
            node_t* tr = LDNODE(t, r);
            if (tr != NULL) {
                t = tr;
            } else {
                STF(n, l, NULL);
                STF(n, r, NULL);
                STF(n, k, k);
                STF(n, v, v);
                STF(n, p, t);
                STF(t, r, n);
                FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        }
    }
}
#define INSERT(s, k, v, n)  insert(s, k, v, n)


/* =============================================================================
 * TMinsert
 * =============================================================================
 */
static node_t*
HTMinsert (rbtree_t* s, void* k, void* v, node_t* n)
{
    node_t* t  = HTX_LDNODE(s, root);
    if (t == NULL) {
        if (n == NULL) {
            return NULL;
        }
        n->l = NULL;
        n->r = NULL;
        n->p = NULL;
        n->k = k;
        n->v = v;
        n->c = BLACK;
        HTX_STF_P(s, root, n);
        return NULL;
    }

    for (;;) {
        long cmp = s->compare(k, HTX_LDF_P(t, k));
        if (cmp == 0) {
            return t;
        } else if (cmp < 0) {
            node_t* tl = HTX_LDNODE(t, l);
            if (tl != NULL) {
                t = tl;
            } else {
                n->l = NULL;
                n->r = NULL;
                n->p = t;
                n->k = k;
                n->v = v;
                HTX_STF_P(t, l, n);
                HTX_FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        } else { /* cmp > 0 */
            node_t* tr = HTX_LDNODE(t, r);
            if (tr != NULL) {
                t = tr;
            } else {
                n->l = NULL;
                n->r = NULL;
                n->p = t;
                n->k = k;
                n->v = v;
                HTX_STF_P(t, r, n);
                HTX_FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        }
    }
}
#define HTX_INSERT(s, k, v, n) HTMinsert(s, k, v, n)


/* =============================================================================
 * TMinsert
 * =============================================================================
 */
TM_CALLABLE
static node_t*
TMinsert (TM_ARGDECL  rbtree_t* s, void* k, void* v, node_t* n)
{
    node_t* t  = TX_LDNODE(s, root);
    if (t == NULL) {
        if (n == NULL) {
            return NULL;
        }
        n->l = NULL;
        n->r = NULL;
        n->p = NULL;
        n->k = k;
        n->v = v;
        n->c = BLACK;
        TX_STF_P(s, root, n);
        return NULL;
    }

    for (;;) {
        long cmp = s->compare(k, TX_LDF_P(t, k));
        if (cmp == 0) {
            return t;
        } else if (cmp < 0) {
            node_t* tl = TX_LDNODE(t, l);
            if (tl != NULL) {
                t = tl;
            } else {
                n->l = NULL;
                n->r = NULL;
                n->p = t;
                n->k = k;
                n->v = v;
                TX_STF_P(t, l, n);
                TX_FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        } else { /* cmp > 0 */
            node_t* tr = TX_LDNODE(t, r);
            if (tr != NULL) {
                t = tr;
            } else {
                n->l = NULL;
                n->r = NULL;
                n->p = t;
                n->k = k;
                n->v = v;
                TX_STF_P(t, r, n);
                TX_FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        }
    }
}
#define TX_INSERT(s, k, v, n)  TMinsert(TM_ARG  s, k, v, n)


/*
 * Return the given node's successor node---the node which has the
 * next key in the the left to right ordering. If the node has
 * no successor, a null pointer is returned rather than a pointer to
 * the nil node
 */


/* =============================================================================
 * successor
 * =============================================================================
 */
static node_t*
successor (node_t* t)
{
    if (t == NULL) {
        return NULL;
    } else if (LDNODE(t, r) != NULL) {
        node_t* p = LDNODE(t, r);
        while (LDNODE(p, l) != NULL) {
            p = LDNODE(p, l);
        }
        return p;
    } else {
        node_t* p = LDNODE(t, p);
        node_t* ch = t;
        while (p != NULL && ch == LDNODE(p, r)) {
            ch = p;
            p = LDNODE(p, p);
        }
        return p;
    }
}
#define SUCCESSOR(n)  successor(n)


/* =============================================================================
 * TMsuccessor
 * =============================================================================
 */
TM_CALLABLE
static node_t*
TMsuccessor  (TM_ARGDECL  node_t* t)
{
    if (t == NULL) {
        return NULL;
    } else if (TX_LDNODE(t, r) != NULL) {
        node_t* p = TX_LDNODE(t,r);
        while (TX_LDNODE(p, l) != NULL) {
            p = TX_LDNODE(p, l);
        }
        return p;
    } else {
        node_t* p = TX_LDNODE(t, p);
        node_t* ch = t;
        while (p != NULL && ch == TX_LDNODE(p, r)) {
            ch = p;
            p = TX_LDNODE(p, p);
        }
        return p;
    }
}
#define TX_SUCCESSOR(n)  TMsuccessor(TM_ARG  n)


/* =============================================================================
 * fixAfterDeletion
 * =============================================================================
 */
static void
fixAfterDeletion (rbtree_t* s, node_t* x)
{
    while (x != LDNODE(s,root) && COLOR_OF(x) == BLACK) {
        if (x == LEFT_OF(PARENT_OF(x))) {
            node_t* sib = RIGHT_OF(PARENT_OF(x));
            if (COLOR_OF(sib) == RED) {
                SET_COLOR(sib, BLACK);
                SET_COLOR(PARENT_OF(x), RED);
                ROTATE_LEFT(s, PARENT_OF(x));
                sib = RIGHT_OF(PARENT_OF(x));
            }
            if (COLOR_OF(LEFT_OF(sib)) == BLACK &&
                COLOR_OF(RIGHT_OF(sib)) == BLACK) {
                SET_COLOR(sib, RED);
                x = PARENT_OF(x);
            } else {
                if (COLOR_OF(RIGHT_OF(sib)) == BLACK) {
                    SET_COLOR(LEFT_OF(sib), BLACK);
                    SET_COLOR(sib, RED);
                    ROTATE_RIGHT(s, sib);
                    sib = RIGHT_OF(PARENT_OF(x));
                }
                SET_COLOR(sib, COLOR_OF(PARENT_OF(x)));
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(RIGHT_OF(sib), BLACK);
                ROTATE_LEFT(s, PARENT_OF(x));
                /* TODO: consider break ... */
                x = LDNODE(s,root);
            }
        } else { /* symmetric */
            node_t* sib = LEFT_OF(PARENT_OF(x));
            if (COLOR_OF(sib) == RED) {
                SET_COLOR(sib, BLACK);
                SET_COLOR(PARENT_OF(x), RED);
                ROTATE_RIGHT(s, PARENT_OF(x));
                sib = LEFT_OF(PARENT_OF(x));
            }
            if (COLOR_OF(RIGHT_OF(sib)) == BLACK &&
                COLOR_OF(LEFT_OF(sib)) == BLACK) {
                SET_COLOR(sib,  RED);
                x = PARENT_OF(x);
            } else {
                if (COLOR_OF(LEFT_OF(sib)) == BLACK) {
                    SET_COLOR(RIGHT_OF(sib), BLACK);
                    SET_COLOR(sib, RED);
                    ROTATE_LEFT(s, sib);
                    sib = LEFT_OF(PARENT_OF(x));
                }
                SET_COLOR(sib, COLOR_OF(PARENT_OF(x)));
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(LEFT_OF(sib), BLACK);
                ROTATE_RIGHT(s, PARENT_OF(x));
                /* TODO: consider break ... */
                x = LDNODE(s, root);
            }
        }
    }

    if (x != NULL && LDF(x,c) != BLACK) {
       STF(x, c, BLACK);
    }
}
#define FIX_AFTER_DELETION(s, n)  fixAfterDeletion(s, n)


/* =============================================================================
 * HTMfixAfterDeletion
 * =============================================================================
 */
static void
HTMfixAfterDeletion  (rbtree_t* s, node_t* x)
{
    while (x != HTX_LDNODE(s,root) && HTX_COLOR_OF(x) == BLACK) {
        if (x == HTX_LEFT_OF(HTX_PARENT_OF(x))) {
            node_t* sib = HTX_RIGHT_OF(HTX_PARENT_OF(x));
            if (HTX_COLOR_OF(sib) == RED) {
                HTX_SET_COLOR(sib, BLACK);
                HTX_SET_COLOR(TX_PARENT_OF(x), RED);
                HTX_ROTATE_LEFT(s, HTX_PARENT_OF(x));
                sib = HTX_RIGHT_OF(HTX_PARENT_OF(x));
            }
            if (HTX_COLOR_OF(HTX_LEFT_OF(sib)) == BLACK &&
                HTX_COLOR_OF(HTX_RIGHT_OF(sib)) == BLACK) {
                HTX_SET_COLOR(sib, RED);
                x = HTX_PARENT_OF(x);
            } else {
                if (HTX_COLOR_OF(HTX_RIGHT_OF(sib)) == BLACK) {
                    HTX_SET_COLOR(HTX_LEFT_OF(sib), BLACK);
                    HTX_SET_COLOR(sib, RED);
                    HTX_ROTATE_RIGHT(s, sib);
                    sib = HTX_RIGHT_OF(HTX_PARENT_OF(x));
                }
                HTX_SET_COLOR(sib, HTX_COLOR_OF(HTX_PARENT_OF(x)));
                HTX_SET_COLOR(HTX_PARENT_OF(x), BLACK);
                HTX_SET_COLOR(HTX_RIGHT_OF(sib), BLACK);
                HTX_ROTATE_LEFT(s, HTX_PARENT_OF(x));
                /* TODO: consider break ... */
                x = HTX_LDNODE(s,root);
            }
        } else { /* symmetric */
            node_t* sib = HTX_LEFT_OF(HTX_PARENT_OF(x));

            if (HTX_COLOR_OF(sib) == RED) {
                HTX_SET_COLOR(sib, BLACK);
                HTX_SET_COLOR(HTX_PARENT_OF(x), RED);
                HTX_ROTATE_RIGHT(s, HTX_PARENT_OF(x));
                sib = HTX_LEFT_OF(HTX_PARENT_OF(x));
            }
            if (HTX_COLOR_OF(HTX_RIGHT_OF(sib)) == BLACK &&
                HTX_COLOR_OF(HTX_LEFT_OF(sib)) == BLACK) {
                HTX_SET_COLOR(sib,  RED);
                x = HTX_PARENT_OF(x);
            } else {
                if (HTX_COLOR_OF(HTX_LEFT_OF(sib)) == BLACK) {
                    HTX_SET_COLOR(HTX_RIGHT_OF(sib), BLACK);
                    HTX_SET_COLOR(sib, RED);
                    HTX_ROTATE_LEFT(s, sib);
                    sib = HTX_LEFT_OF(HTX_PARENT_OF(x));
                }
                HTX_SET_COLOR(sib, HTX_COLOR_OF(HTX_PARENT_OF(x)));
                HTX_SET_COLOR(HTX_PARENT_OF(x), BLACK);
                HTX_SET_COLOR(HTX_LEFT_OF(sib), BLACK);
                HTX_ROTATE_RIGHT(s, TX_PARENT_OF(x));
                /* TODO: consider break ... */
                x = HTX_LDNODE(s, root);
            }
        }
    }

    if (x != NULL && HTX_LDF(x,c) != BLACK) {
       HTX_STF(x, c, BLACK);
    }
}
#define HTX_FIX_AFTER_DELETION(s, n)  HTMfixAfterDeletion(TM_ARG  s, n )


/* =============================================================================
 * TMfixAfterDeletion
 * =============================================================================
 */
TM_CALLABLE
static void
TMfixAfterDeletion  (TM_ARGDECL  rbtree_t* s, node_t* x)
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(RBTREE_FIX_DEL, NULL, s, x);
#endif /* ORIGINAL */

    while (x != TX_LDNODE(s,root) && TX_COLOR_OF(x) == BLACK) {
        if (x == TX_LEFT_OF(TX_PARENT_OF(x))) {
            node_t* sib = TX_RIGHT_OF(TX_PARENT_OF(x));
            if (TX_COLOR_OF(sib) == RED) {
                TX_SET_COLOR(sib, BLACK);
                TX_SET_COLOR(TX_PARENT_OF(x), RED);
                TX_ROTATE_LEFT(s, TX_PARENT_OF(x));
                sib = TX_RIGHT_OF(TX_PARENT_OF(x));
            }
            if (TX_COLOR_OF(TX_LEFT_OF(sib)) == BLACK &&
                TX_COLOR_OF(TX_RIGHT_OF(sib)) == BLACK) {
                TX_SET_COLOR(sib, RED);
                x = TX_PARENT_OF(x);
            } else {
                if (TX_COLOR_OF(TX_RIGHT_OF(sib)) == BLACK) {
                    TX_SET_COLOR(TX_LEFT_OF(sib), BLACK);
                    TX_SET_COLOR(sib, RED);
                    TX_ROTATE_RIGHT(s, sib);
                    sib = TX_RIGHT_OF(TX_PARENT_OF(x));
                }
                TX_SET_COLOR(sib, TX_COLOR_OF(TX_PARENT_OF(x)));
                TX_SET_COLOR(TX_PARENT_OF(x), BLACK);
                TX_SET_COLOR(TX_RIGHT_OF(sib), BLACK);
                TX_ROTATE_LEFT(s, TX_PARENT_OF(x));
                /* TODO: consider break ... */
                x = TX_LDNODE(s,root);
            }
        } else { /* symmetric */
            node_t* sib = TX_LEFT_OF(TX_PARENT_OF(x));

            if (TX_COLOR_OF(sib) == RED) {
                TX_SET_COLOR(sib, BLACK);
                TX_SET_COLOR(TX_PARENT_OF(x), RED);
                TX_ROTATE_RIGHT(s, TX_PARENT_OF(x));
                sib = TX_LEFT_OF(TX_PARENT_OF(x));
            }
            if (TX_COLOR_OF(TX_RIGHT_OF(sib)) == BLACK &&
                TX_COLOR_OF(TX_LEFT_OF(sib)) == BLACK) {
                TX_SET_COLOR(sib,  RED);
                x = TX_PARENT_OF(x);
            } else {
                if (TX_COLOR_OF(TX_LEFT_OF(sib)) == BLACK) {
                    TX_SET_COLOR(TX_RIGHT_OF(sib), BLACK);
                    TX_SET_COLOR(sib, RED);
                    TX_ROTATE_LEFT(s, sib);
                    sib = TX_LEFT_OF(TX_PARENT_OF(x));
                }
                TX_SET_COLOR(sib, TX_COLOR_OF(TX_PARENT_OF(x)));
                TX_SET_COLOR(TX_PARENT_OF(x), BLACK);
                TX_SET_COLOR(TX_LEFT_OF(sib), BLACK);
                TX_ROTATE_RIGHT(s, TX_PARENT_OF(x));
                /* TODO: consider break ... */
                x = TX_LDNODE(s, root);
            }
        }
    }

    if (x != NULL && TX_LDF(x,c) != BLACK) {
       TX_STF(x, c, BLACK);
    }

#ifndef ORIGINAL
    TM_LOG_END(RBTREE_FIX_DEL, NULL);
#endif /* ORIGINAL */
}
#define TX_FIX_AFTER_DELETION(s, n)  TMfixAfterDeletion(TM_ARG  s, n )


/* =============================================================================
 * delete_node
 * =============================================================================
 */
static node_t*
delete_node (rbtree_t* s, node_t* p)
{
    /*
     * If strictly internal, copy successor's element to p and then make p
     * point to successor
     */
    if (LDNODE(p, l) != NULL && LDNODE(p, r) != NULL) {
        node_t* s = SUCCESSOR(p);
        STF(p, k, LDNODE(s, k));
        STF(p, v, LDNODE(s, v));
        p = s;
    } /* p has 2 children */

    /* Start fixup at replacement node, if it exists */
    node_t* replacement =
        ((LDNODE(p, l) != NULL) ? LDNODE(p, l) : LDNODE(p, r));
    node_t* pp = LDNODE(p, p);

    if (replacement != NULL) {
        /* Link replacement to parent */
        STF (replacement, p, pp);
        if (pp == NULL) {
            STF(s, root, replacement);
        } else if (p == LDNODE(pp, l)) {
            STF(pp, l, replacement);
        } else {
            STF(pp, r, replacement);
        }

        /* Null out links so they are OK to use by fixAfterDeletion */
        STF(p, l, NULL);
        STF(p, r, NULL);
        STF(p, p, NULL);

        /* Fix replacement */
        if (LDF(p, c) == BLACK) {
            FIX_AFTER_DELETION(s, replacement);
        }
    } else if (pp == NULL) { /* return if we are the only node */
        STF(s, root, NULL);
    } else { /* No children. Use self as phantom replacement and unlink */
        if (LDF(p, c) == BLACK) {
            FIX_AFTER_DELETION(s, p);
        }
        if (pp != NULL) {
            if (p == LDNODE(pp, l)) {
                STF(pp, l, NULL);
            } else if (p == LDNODE(pp, r)) {
                STF(pp, r, NULL);
            }
            STF(p, p, NULL);
        }
    }
    return p;
}
#define DELETE(s, n)  delete_node(s, n)


/* =============================================================================
 * HTMdelete
 * =============================================================================
 */
static node_t*
HTMdelete (rbtree_t* s, node_t* p)
{
    /*
     * If strictly internal, copy successor's element to p and then make p
     * point to successor
     */
    if (HTX_LDNODE(p, l) != NULL && HTX_LDNODE(p, r) != NULL) {
        node_t* s = TX_SUCCESSOR(p);
        HTX_STF_P(p,k, HTX_LDF_P(s, k));
        HTX_STF_P(p,v, HTX_LDF_P(s, v));
        p = s;
    } /* p has 2 children */

    /* Start fixup at replacement node, if it exists */
    node_t* replacement =
        ((HTX_LDNODE(p, l) != NULL) ? HTX_LDNODE(p, l) : HTX_LDNODE(p, r));
    node_t* pp = HTX_LDNODE(p, p);

    if (replacement != NULL) {
        /* Link replacement to parent */
        HTX_STF_P(replacement, p, pp);
        if (pp == NULL) {
            HTX_STF_P(s, root, replacement);
        } else if (p == HTX_LDNODE(pp, l)) {
            HTX_STF_P(pp, l, replacement);
        } else {
            HTX_STF_P(pp, r, replacement);
        }

        /* Null out links so they are OK to use by fixAfterDeletion */
        HTX_STF_P(p, l, (node_t*)NULL);
        HTX_STF_P(p, r, (node_t*)NULL);
        HTX_STF_P(p, p, (node_t*)NULL);

        /* Fix replacement */
        if (HTX_LDF(p, c) == BLACK) {
            HTX_FIX_AFTER_DELETION(s, replacement);
        }
    } else if (pp == NULL) { /* return if we are the only node */
        HTX_STF_P(s, root, (node_t*)NULL);
    } else { /* No children. Use self as phantom replacement and unlink */
        if (HTX_LDF(p, c) == BLACK) {
            HTX_FIX_AFTER_DELETION(s, p);
        }
        if (pp != NULL) {
            if (p == HTX_LDNODE(pp, l)) {
                HTX_STF_P(pp, l, (node_t*)NULL);
            } else if (p == HTX_LDNODE(pp, r)) {
                HTX_STF_P(pp, r, (node_t*)NULL);
            }
            HTX_STF_P(p, p, (node_t*)NULL);
        }
    }
    return p;
}
#define HTX_DELETE(s, n)  HTMdelete(TM_ARG  s, n)


/* =============================================================================
 * TMdelete
 * =============================================================================
 */
TM_CALLABLE
static node_t*
TMdelete (TM_ARGDECL  rbtree_t* s, node_t* p)
{
    /*
     * If strictly internal, copy successor's element to p and then make p
     * point to successor
     */
    if (TX_LDNODE(p, l) != NULL && TX_LDNODE(p, r) != NULL) {
        node_t* s = TX_SUCCESSOR(p);
        TX_STF_P(p,k, TX_LDF_P(s, k));
        TX_STF_P(p,v, TX_LDF_P(s, v));
        p = s;
    } /* p has 2 children */

    /* Start fixup at replacement node, if it exists */
    node_t* replacement =
        ((TX_LDNODE(p, l) != NULL) ? TX_LDNODE(p, l) : TX_LDNODE(p, r));
    node_t* pp = TX_LDNODE(p, p);

    if (replacement != NULL) {
        /* Link replacement to parent */
        TX_STF_P(replacement, p, pp);
        if (pp == NULL) {
            TX_STF_P(s, root, replacement);
        } else if (p == TX_LDNODE(pp, l)) {
            TX_STF_P(pp, l, replacement);
        } else {
            TX_STF_P(pp, r, replacement);
        }

        /* Null out links so they are OK to use by fixAfterDeletion */
        TX_STF_P(p, l, (node_t*)NULL);
        TX_STF_P(p, r, (node_t*)NULL);
        TX_STF_P(p, p, (node_t*)NULL);

        /* Fix replacement */
        if (TX_LDF(p, c) == BLACK) {
            TX_FIX_AFTER_DELETION(s, replacement);
        }
    } else if (pp == NULL) { /* return if we are the only node */
        TX_STF_P(s, root, (node_t*)NULL);
    } else { /* No children. Use self as phantom replacement and unlink */
        if (TX_LDF(p, c) == BLACK) {
            TX_FIX_AFTER_DELETION(s, p);
        }
        if (pp != NULL) {
            if (p == TX_LDNODE(pp, l)) {
                TX_STF_P(pp, l, (node_t*)NULL);
            } else if (p == TX_LDNODE(pp, r)) {
                TX_STF_P(pp, r, (node_t*)NULL);
            }
            TX_STF_P(p, p, (node_t*)NULL);
        }
    }
    return p;
}
#define TX_DELETE(s, n)  TMdelete(TM_ARG  s, n)


/*
 * Diagnostic section
 */


/* =============================================================================
 * firstEntry
 * =============================================================================
 */
static node_t*
firstEntry (rbtree_t* s)
{
    node_t* p = s->root;
    if (p != NULL) {
        while (p->l != NULL) {
            p = p->l;
        }
    }
    return p;
}


#if 0
/* =============================================================================
 * predecessor
 * =============================================================================
 */
static node_t*
predecessor (node_t* t)
{
    if (t == NULL)
        return NULL;
    else if (t->l != NULL) {
        node_t* p = t->l;
        while (p->r != NULL) {
            p = p->r;
        }
        return p;
    } else {
        node_t* p = t->p;
        node_t* ch = t;
        while (p != NULL && ch == p->l) {
            ch = p;
            p = p->p;
        }
        return p;
    }
}
#endif


/*
 * Compute the BH (BlackHeight) and validate the tree.
 *
 * This function recursively verifies that the given binary subtree satisfies
 * three of the red black properties. It checks that every red node has only
 * black children. It makes sure that each node is either red or black. And it
 * checks that every path has the same count of black nodes from root to leaf.
 * It returns the blackheight of the given subtree; this allows blackheights to
 * be computed recursively and compared for left and right siblings for
 * mismatches. It does not check for every nil node being black, because there
 * is only one sentinel nil node. The return value of this function is the
 * black height of the subtree rooted at the node ``root'', or zero if the
 * subtree is not red-black.
 *
 */


/* =============================================================================
 * verifyRedBlack
 * =============================================================================
 */
static long
verifyRedBlack (node_t* root, long depth)
{
    long height_left;
    long height_right;

    if (root == NULL) {
        return 1;
    }

    height_left  = verifyRedBlack(root->l, depth+1);
    height_right = verifyRedBlack(root->r, depth+1);
    if (height_left == 0 || height_right == 0) {
        return 0;
    }
    if (height_left != height_right) {
        printf(" Imbalance @depth=%ld : %ld %ld\n", depth, height_left, height_right);
    }

    if (root->l != NULL && root->l->p != root) {
       printf(" lineage\n");
    }
    if (root->r != NULL && root->r->p != root) {
       printf(" lineage\n");
    }

    /* Red-Black alternation */
    if (root->c == RED) {
        if (root->l != NULL && root->l->c != BLACK) {
          printf("VERIFY %d\n", __LINE__);
          return 0;
        }
        if (root->r != NULL && root->r->c != BLACK) {
          printf("VERIFY %d\n", __LINE__);
          return 0;
        }
        return height_left;
    }
    if (root->c != BLACK) {
        printf("VERIFY %d\n", __LINE__);
        return 0;
    }

    return (height_left + 1);
}


/* =============================================================================
 * rbtree_verify
 * =============================================================================
 */
long
rbtree_verify (rbtree_t* s, long verbose)
{
    node_t* root = s->root;
    if (root == NULL) {
        return 1;
    }
    if (verbose) {
       printf("Integrity check: ");
    }

    if (root->p != NULL) {
        printf("  (WARNING) root %lX parent=%lX\n",
               (unsigned long)root, (unsigned long)root->p);
        return -1;
    }
    if (root->c != BLACK) {
        printf("  (WARNING) root %lX color=%lX\n",
               (unsigned long)root, (unsigned long)root->c);
    }

    /* Weak check of binary-tree property */
    long ctr = 0;
    node_t* its = firstEntry(s);
    while (its != NULL) {
        ctr++;
        node_t* child = its->l;
        if (child != NULL && child->p != its) {
            printf("Bad parent\n");
        }
        child = its->r;
        if (child != NULL && child->p != its) {
            printf("Bad parent\n");
        }
        node_t* nxt = successor(its);
        if (nxt == NULL) {
            break;
        }
        if (s->compare(its->k, nxt->k) >= 0) {
            printf("Key order %lX (%ld %ld) %lX (%ld %ld)\n",
                   (unsigned long)its, (long)its->k, (long)its->v,
                   (unsigned long)nxt, (long)nxt->k, (long)nxt->v);
            return -3;
        }
        its = nxt;
    }

    long vfy = verifyRedBlack(root, 0);
    if (verbose) {
        printf(" Nodes=%ld Depth=%ld\n", ctr, vfy);
    }

    return vfy;
}


/* =============================================================================
 * rbtree_compare
 * =============================================================================
 */
long
rbtree_compare (const void* a, const void* b)
{
    return ((long)a - (long)b);
}


/* =============================================================================
 * rbtree_alloc
 * =============================================================================
 */
rbtree_t*
rbtree_alloc (long (*compare)(const void*, const void*))
{
    rbtree_t* n = (rbtree_t* )malloc(sizeof(*n));
    if (n) {
        rbtree_setCompare(n, compare ? compare : rbtree_compare);
        n->root = NULL;
    }
    return n;
}


/* =============================================================================
 * HTMrbtree_alloc
 * =============================================================================
 */
rbtree_t*
HTMrbtree_alloc (long (*compare)(const void*, const void*))
{
    rbtree_t* n = (rbtree_t* )HTM_MALLOC(sizeof(*n));
    if (n){
        rbtree_setCompare(n, compare ? compare : rbtree_compare);
        n->root = NULL;
    }

    return n;
}


/* =============================================================================
 * TMrbtree_alloc
 * =============================================================================
 */
TM_CALLABLE
rbtree_t*
TMrbtree_alloc (TM_ARGDECL  long (*compare)(const void*, const void*))
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(RBTREE_ALLOC, NULL, compare);
#endif /* ORIGINAL */

    rbtree_t* n = (rbtree_t* )TM_MALLOC(sizeof(*n));
    if (n){
        rbtree_setCompare(n, compare ? compare : rbtree_compare);
        n->root = NULL;
    }

#ifndef ORIGINAL
    TM_LOG_END(RBTREE_ALLOC, &n);
#endif /* ORIGINAL */
    return n;
}


/* =============================================================================
 * releaseNode
 * =============================================================================
 */
static void
releaseNode (node_t* n)
{
#ifndef SIMULATOR
    free(n);
#endif
}


/* =============================================================================
 * HTMreleaseNode
 * =============================================================================
 */
static void
HTMreleaseNode  (node_t* n)
{
    HTM_FREE(n);
}


/* =============================================================================
 * TMreleaseNode
 * =============================================================================
 */
TM_CALLABLE
static void
TMreleaseNode  (TM_ARGDECL  node_t* n)
{
#if !defined(ORIGINAL) && defined(MERGE_RBTREE)
    TM_SHARED_WRITE_P(n->k, TMRBTREE_BAD_ADDR);
    TM_SHARED_WRITE_P(n->v, TMRBTREE_BAD_ADDR);
    TM_SHARED_WRITE_P(n->l, TMRBTREE_BAD_ADDR);
    TM_SHARED_WRITE_P(n->r, TMRBTREE_BAD_ADDR);
#endif /* !ORIGINAL && MERGE_RBTREE */
    TM_FREE(n);
}


#if !defined(ORIGINAL) && defined(MERGE_RBTREE)
/* =============================================================================
 * TMreleaseNodeUndo
 * =============================================================================
 */
TM_CALLABLE
static stm_merge_t
TMreleaseNodeUndo (TM_ARGDECL  node_t* n)
{
    void *val;
    stm_write_t w;
    stm_free_t f;

    w = TM_SHARED_DID_WRITE(n->k);
    if (STM_VALID_WRITE(w)) {
        ASSERT(TM_SHARED_WRITE_VALUE_P(w, n->k, val) && val == TMRBTREE_BAD_ADDR);
        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
    }

    w = TM_SHARED_DID_WRITE(n->v);
    if (STM_VALID_WRITE(w)) {
        ASSERT(TM_SHARED_WRITE_VALUE_P(w, n->v, val) && val == TMRBTREE_BAD_ADDR);
        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
    }

    w = TM_SHARED_DID_WRITE(n->l);
    if (STM_VALID_WRITE(w)) {
        ASSERT(TM_SHARED_WRITE_VALUE_P(w, n->l, val) && val == TMRBTREE_BAD_ADDR);
        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
    }

    w = TM_SHARED_DID_WRITE(n->r);
    if (STM_VALID_WRITE(w)) {
        ASSERT(TM_SHARED_WRITE_VALUE_P(w, n->r, val) && val == TMRBTREE_BAD_ADDR);
        ASSERT_FAIL(TM_SHARED_UNDO_WRITE(w));
    }

    f = TM_DID_FREE(n);
    if (STM_VALID_FREE(f))
        ASSERT_FAIL(TM_UNDO_FREE(f));

    return STM_MERGE_OK;
}
#endif /* !ORIGINAL && MERGE_RBTREE */


/* =============================================================================
 * freeNode
 * =============================================================================
 */
static void
freeNode (node_t* n, void (*freeData)(void *, void *))
{
    if (n) {
        freeNode(n->l, freeData);
        freeNode(n->r, freeData);
        if (freeData)
            freeData(n->k, n->v);
        releaseNode(n);
    }
}


/* =============================================================================
 * HTMfreeNode
 * =============================================================================
 */
static void
HTMfreeNode (node_t* n, void (*HTMfreeData)(void *, void *))
{
    if (n) {
        HTMfreeNode(n->l, HTMfreeData);
        HTMfreeNode(n->r, HTMfreeData);
        if (HTMfreeData)
            HTMfreeData(n->k, n-> v);
        HTMreleaseNode(n);
    }
}


/* =============================================================================
 * TMfreeNode
 * =============================================================================
 */
TM_CALLABLE
static void
TMfreeNode (TM_ARGDECL  node_t* n, TM_CALLABLE void (*TMfreeData)(void *, void *))
{
    if (n) {
        TMfreeNode(TM_ARG  n->l, TMfreeData);
        TMfreeNode(TM_ARG  n->r, TMfreeData);
        if (TMfreeData)
            TMfreeData(TM_ARG  n->k, n-> v);
        TMreleaseNode(TM_ARG  n);
    }
}


/* =============================================================================
 * rbtree_free
 * =============================================================================
 */
void
rbtree_free (rbtree_t* r, void (*freeData)(void *, void *))
{
    freeNode(r->root, freeData);
    free(r);
}


/* =============================================================================
 * HTMrbtree_free
 * =============================================================================
 */
void
HTMrbtree_free (rbtree_t* r, void (*HTMfreeData)(void *, void *))
{
    HTMfreeNode(TM_ARG  r->root, HTMfreeData);
    HTM_FREE(r);
}


/* =============================================================================
 * TMrbtree_free
 * =============================================================================
 */
TM_CALLABLE
void
TMrbtree_free (TM_ARGDECL  rbtree_t* r, TM_CALLABLE void (*TMfreeData)(void *, void *))
{
#ifndef ORIGINAL
    TM_LOG_BEGIN(RBTREE_FREE, NULL, r);
#endif /* ORIGINAL */
    TMfreeNode(TM_ARG  r->root, TMfreeData);
#if !defined(ORIGINAL) && defined(MERGE_RBTREE)
    TM_SHARED_WRITE_P(r->root, TMRBTREE_BAD_ADDR);
    TM_SHARED_WRITE_P(r->compare, TMRBTREE_BAD_ADDR);
#endif /* !ORIGINAL && MERGE_RBTREE */
    TM_FREE(r);
#ifndef ORIGINAL
    TM_LOG_END(RBTREE_FREE, NULL);
#endif /* ORIGINAL */
}


/* =============================================================================
 * getNode
 * =============================================================================
 */
static node_t*
getNode ()
{
    node_t* n = (node_t*)malloc(sizeof(*n));
    return n;
}


/* =============================================================================
 * HTMgetNode
 * =============================================================================
 */
static node_t*
HTMgetNode ()
{
    node_t* n = (node_t*)HTM_MALLOC(sizeof(*n));
    return n;
}


/* =============================================================================
 * TMgetNode
 * =============================================================================
 */
TM_CALLABLE
static node_t*
TMgetNode (TM_ARGDECL_ALONE)
{
    node_t* n = (node_t*)TM_MALLOC(sizeof(*n));
    return n;
}


/* =============================================================================
 * rbtree_insert
 * -- Returns TRUE on success
 * =============================================================================
 */
bool_t
rbtree_insert (rbtree_t* r, void* key, void* val)
{
    node_t* node = getNode();
    node_t* ex = INSERT(r, key, val, node);
    if (ex != NULL) {
        releaseNode(node);
    }
    return ((ex == NULL) ? TRUE : FALSE);
}


/* =============================================================================
 * HTMrbtree_insert
 * -- Returns TRUE on success
 * =============================================================================
 */
bool_t
HTMrbtree_insert (rbtree_t* r, void* key, void* val)
{
    bool_t rv;

    node_t* node = HTMgetNode();
    node_t* ex = HTX_INSERT(r, key, val, node);
    if (ex != NULL) {
        HTMreleaseNode(TM_ARG  node);
    }
    rv = (ex == NULL) ? TRUE : FALSE;
    return rv;
}


/* =============================================================================
 * TMrbtree_insert
 * -- Returns TRUE on success
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMrbtree_insert (TM_ARGDECL  rbtree_t* r, void* key, void* val)
{
#if !defined(ORIGINAL) && defined(MERGE_RBTREE)
    __label__ replay;
#endif /* !ORIGINAL && MERGE_RBTREE */

    bool_t rv;

#ifndef ORIGINAL
# ifdef MERGE_RBTREE
    stm_merge_t merge(stm_merge_context_t *params) {
        /* Conflict occurred directly inside RBTREE_INSERT */
        ASSERT(params->leaf == 1);
        ASSERT(STM_SAME_OPID(stm_get_op_opcode(params->current), RBTREE_INSERT));

# ifdef TM_DEBUG
        printf("RBTREE_INSERT_JIT r:%p key:%p val:%p rv:%ld\n", r, key, val, params->rv.sint);
# endif
        rv = params->rv.sint;

        TM_FINISH_MERGE();
        goto replay;
    }
# endif /* MERGE_RBTREE */
    TM_LOG_BEGIN(RBTREE_INSERT,
# ifdef MERGE_RBTREE
        merge
# else
        NULL
# endif /* MERGE_RBTREE */
        , r, key, val);
#endif /* ORIGINAL */
    node_t* node = TMgetNode(TM_ARG_ALONE);
    node_t* ex = TX_INSERT(r, key, val, node);
    if (ex != NULL) {
        TMreleaseNode(TM_ARG  node);
    }
    rv = (ex == NULL) ? TRUE : FALSE;
#ifndef ORIGINAL
    TM_LOG_END(RBTREE_INSERT, &rv);
replay:
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * rbtree_delete
 * -- Returns TRUE if key exists
 * =============================================================================
 */
bool_t
rbtree_delete (rbtree_t* r, void* key)
{
    node_t* node = NULL;
    node = LOOKUP(r, key);
    if (node != NULL) {
        node = DELETE(r, node);
    }
    if (node != NULL) {
        releaseNode(node);
    }
    return ((node != NULL) ? TRUE : FALSE);
}


/* =============================================================================
 * HTMrbtree_delete
 * -- Returns TRUE if key exists
 * =============================================================================
 */
bool_t
HTMrbtree_delete (rbtree_t* r, void* key)
{
    bool_t rv;
    node_t* node = NULL;
    node = HTX_LOOKUP(r, key);
    if (node != NULL) {
        node = HTX_DELETE(r, node);
    }
    if (node != NULL) {
        HTMreleaseNode(TM_ARG  node);
    }
    rv = (node != NULL) ? TRUE : FALSE;
    return rv;
}


/* =============================================================================
 * TMrbtree_delete
 * -- Returns TRUE if key exists
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMrbtree_delete (TM_ARGDECL  rbtree_t* r, void* key)
{
    bool_t rv;
#ifndef ORIGINAL
    TM_LOG_BEGIN(RBTREE_DELETE, NULL, r, key);
#endif /* ORIGINAL */
    node_t* node = NULL;
    node = TX_LOOKUP(r, key);
    if (node != NULL) {
        node = TX_DELETE(r, node);
    }
    if (node != NULL) {
        TMreleaseNode(TM_ARG  node);
    }
    rv = (node != NULL) ? TRUE : FALSE;
#ifndef ORIGINAL
    TM_LOG_END(RBTREE_DELETE, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * rbtree_update
 * -- Return FALSE if had to insert node first
 * =============================================================================
 */
bool_t
rbtree_update (rbtree_t* r, void* key, void* val)
{
    node_t* nn = getNode();
    node_t* ex = INSERT(r, key, val, nn);
    if (ex != NULL) {
        STF(ex, v, val);
        releaseNode(nn);
        return TRUE;
    }
    return FALSE;
}


/* =============================================================================
 * TMrbtree_update
 * -- Return FALSE if had to insert node first
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMrbtree_update (TM_ARGDECL  rbtree_t* r, void* key, void* val)
{
    bool_t rv;
#ifndef ORIGINAL
    TM_LOG_BEGIN(RBTREE_UPDATE, NULL, r, key, val);
#endif /* ORIGINAL */
    node_t* nn = TMgetNode(TM_ARG_ALONE);
    node_t* ex = TX_INSERT(r, key, val, nn);
    if (ex != NULL) {
        TX_STF(ex, v, val);
        TMreleaseNode(TM_ARG  nn);
        rv = TRUE;
        goto out;
    }
    rv = FALSE;
out:
#ifndef ORIGINAL
    TM_LOG_END(RBTREE_UPDATE, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * rbtree_get
 * =============================================================================
 */
void*
rbtree_get (rbtree_t* r, void* key) {
    node_t* n = LOOKUP(r, key);
    if (n != NULL) {
        void* val = LDF(n, v);
        return val;
    }
    return NULL;
}


/* =============================================================================
 * HTMrbtree_get
 * =============================================================================
 */
void*
HTMrbtree_get (rbtree_t* r, void* key) {
    void *val = NULL;
    node_t* n = HTX_LOOKUP(r, key);
    if (n != NULL) {
        val = HTX_LDF_P(n, v);
        goto out;
    }
out:
    return val;
}


/* =============================================================================
 * TMrbtree_get
 * =============================================================================
 */
TM_CALLABLE
void*
TMrbtree_get (TM_ARGDECL  rbtree_t* r, void* key) {
    void *val = NULL;
#ifndef ORIGINAL
    TM_LOG_BEGIN(RBTREE_GET, NULL, r, key);
#endif /* ORIGINAL */
    node_t* n = TX_LOOKUP(r, key);
    if (n != NULL) {
        val = TX_LDF_P(n, v);
        goto out;
    }
out:
#ifndef ORIGINAL
    TM_LOG_END(RBTREE_GET, &val);
#endif /* ORIGINAL */
    return val;
}


/* =============================================================================
 * rbtree_contains
 * =============================================================================
 */
long
rbtree_contains (rbtree_t* r, void* key)
{
    node_t* n = LOOKUP(r, key);
    return (n != NULL);
}


/* =============================================================================
 * HTMrbtree_contains
 * =============================================================================
 */
long
HTMrbtree_contains (rbtree_t* r, void* key)
{
    node_t* n = HTX_LOOKUP(r, key);
    return (n != NULL);
}


/* =============================================================================
 * TMrbtree_contains
 * =============================================================================
 */
TM_CALLABLE
long
TMrbtree_contains (TM_ARGDECL  rbtree_t* r, void* key)
{
    long rv;
#ifndef ORIGINAL
    TM_LOG_BEGIN(RBTREE_CONTAINS, NULL, r, key);
#endif /* ORIGINAL */
    node_t* n = TX_LOOKUP(r, key);
    rv = (n != NULL);
#ifndef ORIGINAL
    TM_LOG_END(RBTREE_CONTAINS, &rv);
#endif /* ORIGINAL */
    return rv;
}


/* =============================================================================
 * rbtree_setCompare
 * =============================================================================
 */
void
rbtree_setCompare (rbtree_t* r, long (*compare)(const void*, const void*))
{
    STF(r, compare, compare);
}


/* =============================================================================
 * TMrbtree_compare
 * =============================================================================
 */
TM_CALLABLE
TM_CALLABLE
void
TMrbtree_setCompare (TM_ARGDECL  rbtree_t* r, long (*compare)(const void*, const void*))
{
    TX_STF_P(r, compare, compare);
}


/* =============================================================================
 * rbtree_iterate
 * =============================================================================
 */
static
void rbtree_node(node_t *n, void (*cb)(void *, void *))
{
    if (n) {
        rbtree_node(LDF(n, l), cb);
        rbtree_node(LDF(n, r), cb);
        if (cb)
            cb(LDF(n, k), LDF(n, v));
    }
}


/* =============================================================================
 * rbtree_iterate
 * =============================================================================
 */
void
rbtree_iterate (rbtree_t* r, void (*cb)(void *, void *))
{
    node_t* n = LDNODE(r, root);
    rbtree_node(n, cb);
}


/* /////////////////////////////////////////////////////////////////////////////
 * TEST_RBTREE
 * /////////////////////////////////////////////////////////////////////////////
 */
#ifdef TEST_RBTREE


#include <assert.h>
#include <stdio.h>


static long
compare (const void* a, const void* b)
{
    return (*((const long*)a) - *((const long*)b));
}


static void
insertInt (rbtree_t* rbtreePtr, long* data)
{
    printf("Inserting: %li\n", *data);
    rbtree_insert(rbtreePtr, (void*)data, (void*)data);
    assert(*(long*)rbtree_get(rbtreePtr, (void*)data) == *data);
    assert(rbtree_verify(rbtreePtr, 0) > 0);
}


static void
removeInt (rbtree_t* rbtreePtr, long* data)
{
    printf("Removing: %li\n", *data);
    rbtree_delete(rbtreePtr, (void*)data);
    assert(rbtree_get(rbtreePtr, (void*)data) == NULL);
    assert(rbtree_verify(rbtreePtr, 0) > 0);
}


int
main ()
{
    long data[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7};
    long numData = sizeof(data) / sizeof(data[0]);
    long i;

    puts("Starting...");

    rbtree_t* rbtreePtr = rbtree_alloc(&compare);
    assert(rbtreePtr);

    for (i = 0; i < numData; i++) {
        insertInt(rbtreePtr, &data[i]);
    }

    for (i = 0; i < numData; i++) {
        removeInt(rbtreePtr, &data[i]);
    }

    rbtree_free(rbtreePtr);

    puts("Done.");

    return 0;
}


#endif /* TEST_RBTREE */


/* =============================================================================
 *
 * End of rbtree.c
 *
 * =============================================================================
 */

#ifndef ORIGINAL
# ifdef MERGE_RBTREE
stm_merge_t TMrbtree_merge(stm_merge_context_t *params) {
    const stm_op_id_t op = stm_get_op_opcode(params->current);

    /* Delayed merge only */
    ASSERT(!STM_SAME_OP(stm_get_current_op(), params->current));

    const stm_union_t *args;
    const ssize_t nargs = stm_get_op_args(params->current, &args);

    if (STM_SAME_OPID(op, RBTREE_FIX_DEL) || STM_SAME_OPID(op, RBTREE_FIX_INS)) {
        ASSERT(nargs == 2);
        const rbtree_t *s = args[0].ptr;
        ASSERT(s);
        const node_t *x = args[1].ptr;
        ASSERT(x);
        ASSERT(!STM_VALID_OP(params->previous));
        ASSERT(params->leaf == 1);

        if (STM_SAME_OPID(op, RBTREE_FIX_DEL)) {
# ifdef TM_DEBUG
            printf("\nRBTREE_FIX_DEL addr:%p treePtr:%p node:%p\n", params->addr, s, x);
# endif
        } else {
# ifdef TM_DEBUG
            printf("\nRBTREE_FIX_INS addr:%p treePtr:%p node:%p\n", params->addr, s, x);
# endif
        }
    } else if (STM_SAME_OPID(op, RBTREE_LOOKUP) || STM_SAME_OPID(op, RBTREE_DELETE) || STM_SAME_OPID(op, RBTREE_GET) || STM_SAME_OPID(op, RBTREE_CONTAINS)) {
        ASSERT(nargs == 2);
        const rbtree_t *s = args[0].ptr;
        ASSERT(s);
        const void *key = args[1].ptr;

        if (STM_SAME_OPID(op, RBTREE_LOOKUP)) {
            ASSERT(params->leaf == 1);
            ASSERT(!STM_VALID_OP(params->previous));

            ASSERT(ENTRY_VALID(params->conflict.entries->e1));
            stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
            ASSERT(STM_VALID_READ(r));
# ifdef TM_DEBUG
            printf("\nRBTREE_LOOKUP addr:%p treePtr:%p key:%p\n", params->addr, s, key);
# endif

            const node_t *old, *new;
            /* Recover tree node pointer from address */
            const node_t *base = (node_t *)TM_SHARED_GET_TAG(r);
            ASSERT(base);
            if (params->addr == &s->root) {
                /* Read the old and new values */
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, s->root, old));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, s->root, new));
# ifdef TM_DEBUG
                printf("RBTREE_LOOKUP treePtr->root:%p (old):%p (new):%p\n", &s->root, old, new);
# endif
                if (old == new)
                    return STM_MERGE_OK;

                /* Check if the current node has been freed */
                if (new == TMRBTREE_BAD_ADDR) {
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    return STM_MERGE_OK;
                }
            } else if (params->addr == &base->k) {
                ASSERT(STM_SAME_READ(r, TM_SHARED_DID_READ(base->k)));
                /* Read the old and new values */
                const void *oldKey, *newKey;
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, base->k, oldKey));
                ASSERT(oldKey);
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, base->k, newKey));
                ASSERT(newKey);

                if (oldKey == newKey)
                    return STM_MERGE_OK;
# ifdef TM_DEBUG
                printf("RBTREE_LOOKUP p:%p p->k:%p (old):%p (new):%p\n", base, &base->k, oldKey, newKey);
# endif

                /* Check if the current node has been freed */
                if (newKey == TMRBTREE_BAD_ADDR) {
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    return STM_MERGE_OK;
                }
                const long oldCmp = s->compare(key, oldKey);
                const long newCmp = s->compare(key, newKey);
                if ((!oldCmp && !newCmp) || (oldCmp && newCmp && (oldCmp < 0) == (newCmp < 0)))
                    return STM_MERGE_OK;
                else if (!newCmp) {
                    new = base;
                    goto out;
                } else {
                    ASSERT(!STM_VALID_READ(newCmp < 0 ? TM_SHARED_DID_READ(base->l) : TM_SHARED_DID_READ(base->r)));
                    new = ((newCmp < 0) ? TX_LDNODE(base, l) : TX_LDNODE(base, r));
                }
                old = base;
            } else if (params->addr == &base->l) {
                ASSERT(STM_SAME_READ(r, TM_SHARED_DID_READ(base->l)));
                /* Read the old and new values */
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, base->l, old));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, base->l, new));

                if (old == new)
                    return STM_MERGE_OK;
l:
# ifdef TM_DEBUG
                printf("RBTREE_LOOKUP p:%p p->l:%p (old):%p (new):%p\n", base, &base->l, old, new);
# endif

                /* Check if the current node has been freed */
                if (new == TMRBTREE_BAD_ADDR) {
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    new = params->rv.ptr;
                    goto out;
                }
            } else if (params->addr == &base->r) {
                ASSERT(STM_SAME_READ(r, TM_SHARED_DID_READ(base->r)));
                /* Read the old and new values */
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, base->r, old));
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, base->r, new));

                if (old == new)
                    return STM_MERGE_OK;
r:
# ifdef TM_DEBUG
                printf("RBTREE_LOOKUP p:%p p->r:%p (old):%p (new):%p\n", base, &base->r, old, new);
# endif

                /* Check if the current node has been freed */
                if (new == TMRBTREE_BAD_ADDR) {
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(r));
                    new = params->rv.ptr;
                    goto out;
                }
            } else {
                /* FIXME: Conflict unsupported */
                ASSERT(0);
# ifdef TM_DEBUG
                printf("RBTREE_LOOKUP UNSUPPORTED addr:%p\n", params->addr);
# endif
                return STM_MERGE_ABORT;
            }

            ASSERT(old != new);
            while (new) {
                /* We should never reach a node that has been freed. */
                if (__builtin_expect(new == TMRBTREE_BAD_ADDR, 0))
                    return STM_MERGE_RETRY;

                r = TM_SHARED_DID_READ(new->k);
                /* Early termination when previously visited nodes are encountered */
                if (STM_VALID_READ(r)) {
                    new = params->rv.ptr;
                    break;
                }

                void *k = TX_LDF_P(new, k);
# ifdef TM_DEBUG
                printf("RBTREE_LOOKUP p:%p p.k:%p\n", new, k);
# endif
                ASSERT(k);
                /* We should never reach a node that has been freed. */
                if (__builtin_expect(k == TMRBTREE_BAD_ADDR, 0))
                    return STM_MERGE_RETRY;

                const int cmp = s->compare(key, k);
                if (!cmp)
                    break;

                old = new;

                r = (cmp < 0) ? TM_SHARED_DID_READ(new->l) : TM_SHARED_DID_READ(new->r);
                if (!STM_VALID_READ(r)) {
                    new = (cmp < 0) ? TX_LDNODE(new, l) : TX_LDNODE(new, r);
                } else {
                    ASSERT_FAIL((cmp < 0) ? TM_SHARED_READ_VALUE_P(r, new->l, old) : TM_SHARED_READ_VALUE_P(r, new->r, old));
                    ASSERT_FAIL((cmp < 0) ? TM_SHARED_READ_UPDATE_P(r, new->l, new) : TM_SHARED_READ_UPDATE_P(r, new->r, new));
                    if (old != new) {
                        if (cmp < 0)
                            goto l;
                        else
                            goto r;
                    }
                }
# ifdef TM_DEBUG
                printf("RBTREE_LOOKUP new:%p new.l/r(new):%p\n", old, new);
# endif
            }

out:
# ifdef TM_DEBUG
            printf("RBTREE_LOOKUP rv(old):%p rv(new):%p\n", params->rv.ptr, new);
# endif

            /* Pass the new return value */
            params->rv.ptr = (node_t *)new;
            return STM_MERGE_OK;
        } else if (STM_SAME_OPID(op, RBTREE_DELETE)) {
            ASSERT(params->rv.sint == FALSE || params->rv.sint == TRUE);

            if (params->leaf == 1) {
                ASSERT(!STM_VALID_OP(params->previous));
# ifdef TM_DEBUG
                printf("\nRBTREE_DELETE addr:%p treePtr:%p key:%p\n", params->addr, s, key);
# endif
            } else if (STM_SAME_OPID(stm_get_op_opcode(params->previous), RBTREE_LOOKUP)) {
                ASSERT(STM_VALID_OP(params->previous));
# ifdef TM_DEBUG
                printf("\nRBTREE_DELETE <- RBTREE_LOOKUP %p\n", params->conflict.result.ptr);
# endif
                if (!params->conflict.result.ptr) {
                    ASSERT_FAIL(stm_clear_op(params->current, 1, 1, 1));
# ifdef TM_DEBUG
                    printf("RBTREE_DELETE rv(old):%p rv(new):%p\n", params->rv.ptr, NULL);
# endif
                    params->rv.ptr = NULL;
                    return STM_MERGE_OK;
                }
            }
        } else if (STM_SAME_OPID(op, RBTREE_GET)) {
# ifdef TM_DEBUG
            void *old;
# endif
            void *new;
            if (params->leaf == 1) {
                ASSERT(!STM_VALID_OP(params->previous));
# ifdef TM_DEBUG
                printf("\nRBTREE_GET addr:%p treePtr:%p key:%p\n", params->addr, s, key);
# endif
                ASSERT(ENTRY_VALID(params->conflict.entries->e1));
                stm_read_t r = ENTRY_GET_READ(params->conflict.entries->e1);
                ASSERT(STM_VALID_READ(r));

                /* Recover tree node pointer from address */
                const node_t *base = (node_t *)TM_SHARED_GET_TAG(r);
                ASSERT(base && params->addr == &base->v);

                /* Read the old and new values */
# ifdef TM_DEBUG
                ASSERT_FAIL(TM_SHARED_READ_VALUE_P(r, base->v, old));
# endif
                ASSERT_FAIL(TM_SHARED_READ_UPDATE_P(r, base->v, new));
# ifdef TM_DEBUG
                printf("RBTREE_GET n->v:%p (old):%p (new):%p\n", &base->v, old, new);
# endif
            } else if (STM_SAME_OPID(stm_get_op_opcode(params->previous), RBTREE_LOOKUP)) {
                ASSERT(STM_VALID_OP(params->previous));
# ifdef TM_DEBUG
                printf("\nRBTREE_GET <- RBTREE_LOOKUP %p\n", params->conflict.result.ptr);
# endif
                const node_t *prev_node = (node_t *)params->conflict.previous_result.ptr;
                ASSERT(prev_node != params->conflict.result.ptr);

                if (prev_node) {
                    stm_read_t rprev = TM_SHARED_DID_READ(prev_node->v);
                    ASSERT(STM_VALID_READ(rprev));

                    /* Explicitly set the read position */
                    params->read = rprev;
# ifdef TM_DEBUG
                    ASSERT_FAIL(TM_SHARED_READ_VALUE_P(rprev, prev_node->v, old));
# endif
                    ASSERT_FAIL(TM_SHARED_UNDO_READ(rprev));
                } else {
# ifdef TM_DEBUG
                    old = NULL;
# endif
                }

                const node_t *ret = params->conflict.result.ptr;
                ASSERT(!ret || !STM_VALID_READ(TM_SHARED_DID_READ(ret->v)));
                new = ret ? TX_LDF_P(ret, v) : NULL;
            }

# ifdef TM_DEBUG
            printf("RBTREE_GET rv(old):%p rv(new):%p\n", old, new);
# endif
            params->rv.ptr = new;
            return STM_MERGE_OK;
        } else if (STM_SAME_OPID(stm_get_op_opcode(params->previous), RBTREE_LOOKUP)) {
            ASSERT(params->rv.sint == FALSE || params->rv.sint == TRUE);

            ASSERT(!params->leaf);
            ASSERT(STM_VALID_OP(params->previous));
            const long rv = params->conflict.result.ptr ? TRUE : FALSE;
# ifdef TM_DEBUG
            printf("\nRBTREE_CONTAINS <- RBTREE_LOOKUP %p\n", params->conflict.result.ptr);
            printf("RBTREE_CONTAINS rv(old):%li rv(new):%li\n", params->rv.sint, rv);
# endif
            params->rv.sint = rv;
            return STM_MERGE_OK;
        }
    } else if (STM_SAME_OPID(op, RBTREE_INSERT) || STM_SAME_OPID(op, RBTREE_UPDATE)) {
        ASSERT(nargs == 3);
        const rbtree_t *s = args[0].ptr;
        ASSERT(s);
        const void *key = args[1].ptr;
        const void *val = args[2].ptr;
        ASSERT(!STM_VALID_OP(params->previous));
        ASSERT(params->leaf == 1);

        if (STM_SAME_OPID(op, RBTREE_INSERT)) {
            ASSERT(params->rv.sint == FALSE || params->rv.sint == TRUE);
# ifdef TM_DEBUG
            printf("\nRBTREE_INSERT addr:%p treePtr:%p key:%p val:%p\n", params->addr, s, key, val);
# endif
        } else {
            ASSERT(params->rv.sint == FALSE || params->rv.sint == TRUE);
# ifdef TM_DEBUG
            printf("\nRBTREE_UPDATE addr:%p treePtr:%p key:%p val:%p\n", params->addr, s, key, val);
# endif
        }
    }

# ifdef TM_DEBUG
    printf("\nRBTREE_MERGE UNSUPPORTED addr:%p\n", params->addr);
# endif
    return STM_MERGE_UNSUPPORTED;
}
# define TMRBTREE_MERGE TMrbtree_merge
#else
# define TMRBTREE_MERGE NULL
#endif /* MERGE_RBTREE */

__attribute__((constructor)) void rbtree_init() {
    TM_LOG_FFI_DECLARE;
    TM_LOG_TYPE_DECLARE_INIT(*ppp[], {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer});
    TM_LOG_TYPE_DECLARE_INIT(*pp[], {&ffi_type_pointer, &ffi_type_pointer});
    TM_LOG_TYPE_DECLARE_INIT(*p[], {&ffi_type_pointer});
    # define TM_LOG_OP TM_LOG_OP_INIT
    # include "rbtree.inc"
    # undef TM_LOG_OP
}
#endif /* ORIGINAL */
