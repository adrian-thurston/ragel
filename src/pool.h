/*
 * Copyright 2010-2012 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _COLM_POOL_H
#define _COLM_POOL_H

/* Allocation, number of items. */
#define FRESH_BLOCK 8128                    

#include <colm/pdarun.h>
#include <colm/map.h>
#include <colm/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_pool_alloc( struct pool_alloc *pool_alloc, int sizeofT );

kid_t *kid_allocate( program_t *prg );
void kid_free( program_t *prg, kid_t *el );
void kid_clear( program_t *prg );
long kid_num_lost( program_t *prg );

tree_t *tree_allocate( program_t *prg );
void tree_free( program_t *prg, tree_t *el );
void tree_clear( program_t *prg );
long tree_num_lost( program_t *prg );

/* Parse tree allocators go into pda_run structs. */
parse_tree_t *parse_tree_allocate( struct pda_run *pda_run );
void parse_tree_free( struct pda_run *pda_run, parse_tree_t *el );
void parse_tree_clear( struct pool_alloc *pool_alloc );
long parse_tree_num_lost( struct pool_alloc *pool_alloc );

head_t *head_allocate( program_t *prg );
void head_free( program_t *prg, head_t *el );
void head_clear( program_t *prg );
long head_num_lost( program_t *prg );

location_t *location_allocate( program_t *prg );
void location_free( program_t *prg, location_t *el );
void location_clear( program_t *prg );
long location_num_lost( program_t *prg );

void pool_alloc_clear( struct pool_alloc *pool_alloc );
long pool_alloc_num_lost( struct pool_alloc *pool_alloc );

#ifdef __cplusplus
}
#endif

#endif /* _COLM_POOL_H */

