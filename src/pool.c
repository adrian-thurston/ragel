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

#include <colm/pool.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <colm/pdarun.h>
#include <colm/debug.h>

void init_pool_alloc( struct pool_alloc *pool_alloc, int sizeofT )
{
	pool_alloc->head = 0;
	pool_alloc->nextel = FRESH_BLOCK;
	pool_alloc->pool = 0;
	pool_alloc->sizeofT = sizeofT;
}

static void *pool_alloc_allocate( struct pool_alloc *pool_alloc )
{
	//debug( REALM_POOL, "pool allocation\n" );

#ifdef POOL_MALLOC
	void *res = malloc( pool_alloc->sizeofT );
	memset( res, 0, pool_alloc->sizeofT );
	return res;
#else

	void *new_el = 0;
	if ( pool_alloc->pool == 0 ) {
		if ( pool_alloc->nextel == FRESH_BLOCK ) {
			struct pool_block *new_block = (struct pool_block*)malloc( sizeof(struct pool_block) );
			new_block->data = malloc( pool_alloc->sizeofT * FRESH_BLOCK );
			new_block->next = pool_alloc->head;
			pool_alloc->head = new_block;
			pool_alloc->nextel = 0;
		}

		new_el = (char*)pool_alloc->head->data + pool_alloc->sizeofT * pool_alloc->nextel++;
	}
	else {
		new_el = pool_alloc->pool;
		pool_alloc->pool = pool_alloc->pool->next;
	}
	memset( new_el, 0, pool_alloc->sizeofT );
	return new_el;
#endif
}

void pool_alloc_free( struct pool_alloc *pool_alloc, void *el )
{
	#if 0
	/* Some sanity checking. Best not to normally run with this on. */
	char *p = (char*)el + sizeof(struct pool_item*);
	char *pe = (char*)el + sizeof(T);
	for ( ; p < pe; p++ )
		assert( *p != 0xcc );
	memset( el, 0xcc, sizeof(T) );
	#endif

#ifdef POOL_MALLOC
	free( el );
#else
	struct pool_item *pi = (struct pool_item*) el;
	pi->next = pool_alloc->pool;
	pool_alloc->pool = pi;
#endif
}

void pool_alloc_clear( struct pool_alloc *pool_alloc )
{
	struct pool_block *block = pool_alloc->head;
	while ( block != 0 ) {
		struct pool_block *next = block->next;
		free( block->data );
		free( block );
		block = next;
	}

	pool_alloc->head = 0;
	pool_alloc->nextel = 0;
	pool_alloc->pool = 0;
}

long pool_alloc_num_lost( struct pool_alloc *pool_alloc )
{
	/* Count the number of items allocated. */
	long lost = 0;
	struct pool_block *block = pool_alloc->head;
	if ( block != 0 ) {
		lost = pool_alloc->nextel;
		block = block->next;
		while ( block != 0 ) {
			lost += FRESH_BLOCK;
			block = block->next;
		}
	}

	/* Subtract. Items that are on the free list. */
	struct pool_item *pi = pool_alloc->pool;
	while ( pi != 0 ) {
		lost -= 1;
		pi = pi->next;
	}

	return lost;
}

/* 
 * kid_t
 */

kid_t *kid_allocate( program_t *prg )
{
	return (kid_t*) pool_alloc_allocate( &prg->kid_pool );
}

void kid_free( program_t *prg, kid_t *el )
{
	pool_alloc_free( &prg->kid_pool, el );
}

void kid_clear( program_t *prg )
{
	pool_alloc_clear( &prg->kid_pool );
}

long kid_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->kid_pool );
}

/* 
 * tree_t
 */

tree_t *tree_allocate( program_t *prg )
{
	return (tree_t*) pool_alloc_allocate( &prg->tree_pool );
}

void tree_free( program_t *prg, tree_t *el )
{
	pool_alloc_free( &prg->tree_pool, el );
}

void tree_clear( program_t *prg )
{
	pool_alloc_clear( &prg->tree_pool );
}

long tree_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->tree_pool );
}

/* 
 * parse_tree_t
 */

parse_tree_t *parse_tree_allocate( struct pda_run *pda_run )
{
	return (parse_tree_t*) pool_alloc_allocate( pda_run->parse_tree_pool );
}

void parse_tree_free( struct pda_run *pda_run, parse_tree_t *el )
{
	pool_alloc_free( pda_run->parse_tree_pool, el );
}

void parse_tree_clear( struct pool_alloc *pool_alloc )
{
	pool_alloc_clear( pool_alloc );
}

long parse_tree_num_lost( struct pool_alloc *pool_alloc )
{
	return pool_alloc_num_lost( pool_alloc );
}

/* 
 * head_t
 */

head_t *head_allocate( program_t *prg )
{
	return (head_t*) pool_alloc_allocate( &prg->head_pool );
}

void head_free( program_t *prg, head_t *el )
{
	pool_alloc_free( &prg->head_pool, el );
}

void head_clear( program_t *prg )
{
	pool_alloc_clear( &prg->head_pool );
}

long head_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->head_pool );
}

/* 
 * location_t
 */

location_t *location_allocate( program_t *prg )
{
	return (location_t*) pool_alloc_allocate( &prg->location_pool );
}

void location_free( program_t *prg, location_t *el )
{
	pool_alloc_free( &prg->location_pool, el );
}

void location_clear( program_t *prg )
{
	pool_alloc_clear( &prg->location_pool );
}

long location_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->location_pool );
}
