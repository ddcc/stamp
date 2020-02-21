/* =============================================================================
 *
 * mesh.h
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


#ifndef MESH_H
#define MESH_H 1


#include "element.h"
#include "map.h"
#include "random.h"
#include "tm.h"
#include "vector.h"


typedef struct mesh  mesh_t;


/* =============================================================================
 * mesh_alloc
 * =============================================================================
 */
mesh_t*
mesh_alloc ();


/* =============================================================================
 * mesh_free
 * =============================================================================
 */
void
mesh_free (mesh_t* meshPtr);


/* =============================================================================
 * mesh_insert
 * =============================================================================
 */
void
mesh_insert (mesh_t* meshPtr, element_t* elementPtr, MAP_T* edgeMapPtr);


/* =============================================================================
 * HTMmesh_insert
 * =============================================================================
 */
void
HTMmesh_insert (mesh_t* meshPtr, element_t* elementPtr, MAP_T* edgeMapPtr);


/* =============================================================================
 * TMmesh_insert
 * =============================================================================
 */
TM_CANCELLABLE
void
TMmesh_insert (TM_ARGDECL
               mesh_t* meshPtr, element_t* elementPtr, MAP_T* edgeMapPtr);


/* =============================================================================
 * mesh_remove
 * =============================================================================
 */
void
mesh_remove (mesh_t* meshPtr, element_t* elementPtr);


/* =============================================================================
 * HTMmesh_remove
 * =============================================================================
 */
void
HTMmesh_remove (mesh_t* meshPtr, element_t* elementPtr);


/* =============================================================================
 * TMmesh_remove
 * =============================================================================
 */
TM_CALLABLE
void
TMmesh_remove (TM_ARGDECL  mesh_t* meshPtr, element_t* elementPtr);


/* =============================================================================
 * mesh_insertBoundary
 * =============================================================================
 */
bool_t
mesh_insertBoundary (mesh_t* meshPtr, edge_t* boundaryPtr);


/* =============================================================================
 * HTMmesh_insertBoundary
 * =============================================================================
 */
bool_t
HTMmesh_insertBoundary (mesh_t* meshPtr, edge_t* boundaryPtr);


/* =============================================================================
 * TMmesh_insertBoundary
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMmesh_insertBoundary (TM_ARGDECL  mesh_t* meshPtr, edge_t* boundaryPtr);


/* =============================================================================
 * mesh_removeBoundary
 * =============================================================================
 */
bool_t
mesh_removeBoundary (mesh_t* meshPtr, edge_t* boundaryPtr);


/* =============================================================================
 * HTMmesh_removeBoundary
 * =============================================================================
 */
bool_t
HTMmesh_removeBoundary (mesh_t* meshPtr, edge_t* boundaryPtr);


/* =============================================================================
 * TMmesh_removeBoundary
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMmesh_removeBoundary (TM_ARGDECL  mesh_t* meshPtr, edge_t* boundaryPtr);


/* =============================================================================
 * mesh_read
 *
 * Returns number of elements read from file.
 *
 * Refer to http://www.cs.cmu.edu/~quake/triangle.html for file formats.
 * =============================================================================
 */
long
mesh_read (mesh_t* meshPtr, char* fileNamePrefix);


/* =============================================================================
 * mesh_getBad
 * -- Returns NULL if none
 * =============================================================================
 */
element_t*
mesh_getBad (mesh_t* meshPtr);


/* =============================================================================
 * mesh_shuffleBad
 * =============================================================================
 */
void
mesh_shuffleBad (mesh_t* meshPtr, random_t* randomPtr);


/* =============================================================================
 * mesh_check
 * =============================================================================
 */
bool_t
mesh_check (mesh_t* meshPtr, long expectedNumElement);


#define HTMMESH_INSERT(m, e, em)        HTMmesh_insert(m, e, em)
#define HTMMESH_REMOVE(m, e)            HTMmesh_remove(m, e)
#define HTMMESH_INSERTBOUNDARY(m, b)    HTMmesh_insertBoundary(m, b)
#define HTMMESH_REMOVEBOUNDARY(m, b)    HTMmesh_removeBoundary(m, b)

#define TMMESH_INSERT(m, e, em)         TMmesh_insert(TM_ARG  m, e, em)
#define TMMESH_REMOVE(m, e)             TMmesh_remove(TM_ARG  m, e)
#define TMMESH_INSERTBOUNDARY(m, b)     TMmesh_insertBoundary(TM_ARG  m, b)
#define TMMESH_REMOVEBOUNDARY(m, b)     TMmesh_removeBoundary(TM_ARG  m, b)


#endif /* MESH_H */


/* =============================================================================
 *
 * End of mesh.h
 *
 * =============================================================================
 */
