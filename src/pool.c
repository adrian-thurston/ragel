/*
 *  Copyright 2010-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <string.h>
#include <stdlib.h>

#include <colm/pdarun.h>
#include <colm/pool.h>
#include <colm/debug.h>

void init_pool_alloc( struct pool_alloc *poolAlloc, int sizeofT )
{
	poolAlloc->head = 0;
	poolAlloc->nextel = FRESH_BLOCK;
	poolAlloc->pool = 0;
	poolAlloc->sizeofT = sizeofT;
}

static void *pool_alloc_allocate( struct pool_alloc *poolAlloc )
{
	//debug( REALM_POOL, "pool allocation\n" );

#ifdef POOL_MALLOC
	void *res = malloc( poolAlloc->sizeofT );
	memset( res, 0, poolAlloc->sizeofT );
	return res;
#else

	void *newEl = 0;
	if ( poolAlloc->pool == 0 ) {
		if ( poolAlloc->nextel == FRESH_BLOCK ) {
			struct pool_block *newBlock = (struct pool_block*)malloc( sizeof(struct pool_block) );
			newBlock->data = malloc( poolAlloc->sizeofT * FRESH_BLOCK );
			newBlock->next = poolAlloc->head;
			poolAlloc->head = newBlock;
			poolAlloc->nextel = 0;
		}

		newEl = (char*)poolAlloc->head->data + poolAlloc->sizeofT * poolAlloc->nextel++;
	}
	else {
		newEl = poolAlloc->pool;
		poolAlloc->pool = poolAlloc->pool->next;
	}
	memset( newEl, 0, poolAlloc->sizeofT );
	return newEl;
#endif
}

void pool_alloc_free( struct pool_alloc *poolAlloc, void *el )
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
	pi->next = poolAlloc->pool;
	poolAlloc->pool = pi;
#endif
}

void pool_alloc_clear( struct pool_alloc *poolAlloc )
{
	struct pool_block *block = poolAlloc->head;
	while ( block != 0 ) {
		struct pool_block *next = block->next;
		free( block->data );
		free( block );
		block = next;
	}

	poolAlloc->head = 0;
	poolAlloc->nextel = 0;
	poolAlloc->pool = 0;
}

long pool_alloc_num_lost( struct pool_alloc *poolAlloc )
{
	/* Count the number of items allocated. */
	long lost = 0;
	struct pool_block *block = poolAlloc->head;
	if ( block != 0 ) {
		lost = poolAlloc->nextel;
		block = block->next;
		while ( block != 0 ) {
			lost += FRESH_BLOCK;
			block = block->next;
		}
	}

	/* Subtract. Items that are on the free list. */
	struct pool_item *pi = poolAlloc->pool;
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
	return (kid_t*) pool_alloc_allocate( &prg->kidPool );
}

void kid_free( program_t *prg, kid_t *el )
{
	pool_alloc_free( &prg->kidPool, el );
}

void kid_clear( program_t *prg )
{
	pool_alloc_clear( &prg->kidPool );
}

long kid_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->kidPool );
}

/* 
 * tree_t
 */

tree_t *tree_allocate( program_t *prg )
{
	return (tree_t*) pool_alloc_allocate( &prg->treePool );
}

void tree_free( program_t *prg, tree_t *el )
{
	pool_alloc_free( &prg->treePool, el );
}

void tree_clear( program_t *prg )
{
	pool_alloc_clear( &prg->treePool );
}

long tree_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->treePool );
}

/* 
 * parse_tree_t
 */

parse_tree_t *parse_tree_allocate( program_t *prg )
{
	return (parse_tree_t*) pool_alloc_allocate( &prg->parseTreePool );
}

void parse_tree_free( program_t *prg, parse_tree_t *el )
{
	pool_alloc_free( &prg->parseTreePool, el );
}

void parse_tree_clear( program_t *prg )
{
	pool_alloc_clear( &prg->parseTreePool );
}

long parse_tree_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->parseTreePool );
}

/* 
 * head_t
 */

head_t *head_allocate( program_t *prg )
{
	return (head_t*) pool_alloc_allocate( &prg->headPool );
}

void head_free( program_t *prg, head_t *el )
{
	pool_alloc_free( &prg->headPool, el );
}

void head_clear( program_t *prg )
{
	pool_alloc_clear( &prg->headPool );
}

long head_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->headPool );
}

/* 
 * location_t
 */

location_t *location_allocate( program_t *prg )
{
	return (location_t*) pool_alloc_allocate( &prg->locationPool );
}

void location_free( program_t *prg, location_t *el )
{
	pool_alloc_free( &prg->locationPool, el );
}

void location_clear( program_t *prg )
{
	pool_alloc_clear( &prg->locationPool );
}

long location_num_lost( program_t *prg )
{
	return pool_alloc_num_lost( &prg->locationPool );
}
