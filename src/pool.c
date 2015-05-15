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

void initPoolAlloc( struct pool_alloc *poolAlloc, int sizeofT )
{
	poolAlloc->head = 0;
	poolAlloc->nextel = FRESH_BLOCK;
	poolAlloc->pool = 0;
	poolAlloc->sizeofT = sizeofT;
}

void *poolAllocAllocate( struct pool_alloc *poolAlloc )
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

void poolAllocFree( struct pool_alloc *poolAlloc, void *el )
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

void poolAllocClear( struct pool_alloc *poolAlloc )
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

long poolAllocNumLost( struct pool_alloc *poolAlloc )
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

kid_t *kidAllocate( program_t *prg )
{
	return (kid_t*) poolAllocAllocate( &prg->kidPool );
}

void kidFree( program_t *prg, kid_t *el )
{
	poolAllocFree( &prg->kidPool, el );
}

void kidClear( program_t *prg )
{
	poolAllocClear( &prg->kidPool );
}

long kidNumLost( program_t *prg )
{
	return poolAllocNumLost( &prg->kidPool );
}

/* 
 * tree_t
 */

tree_t *treeAllocate( program_t *prg )
{
	return (tree_t*) poolAllocAllocate( &prg->treePool );
}

void treeFree( program_t *prg, tree_t *el )
{
	poolAllocFree( &prg->treePool, el );
}

void treeClear( program_t *prg )
{
	poolAllocClear( &prg->treePool );
}

long treeNumLost( program_t *prg )
{
	return poolAllocNumLost( &prg->treePool );
}

/* 
 * parse_tree_t
 */

parse_tree_t *parseTreeAllocate( program_t *prg )
{
	return (parse_tree_t*) poolAllocAllocate( &prg->parseTreePool );
}

void parseTreeFree( program_t *prg, parse_tree_t *el )
{
	poolAllocFree( &prg->parseTreePool, el );
}

void parseTreeClear( program_t *prg )
{
	poolAllocClear( &prg->parseTreePool );
}

long parseTreeNumLost( program_t *prg )
{
	return poolAllocNumLost( &prg->parseTreePool );
}

/* 
 * list_el_t
 */

#if 0
list_el_t *listElAllocate( program_t *prg )
{
	return (list_el_t*) poolAllocAllocate( &prg->listElPool );
}

void listElFree( program_t *prg, list_el_t *el )
{
	//poolAllocFree( &prg->listElPool, el );
}

void listElClear( program_t *prg )
{
	poolAllocClear( &prg->listElPool );
}

long listElNumLost( program_t *prg )
{
	return poolAllocNumLost( &prg->listElPool );
}
#endif

/* 
 * map_el_t
 */

#if 0
map_el_t *mapElAllocate( program_t *prg )
{
	return (map_el_t*) poolAllocAllocate( &prg->mapElPool );
}

void mapElFree( program_t *prg, map_el_t *el )
{
	poolAllocFree( &prg->mapElPool, el );
}

void mapElClear( program_t *prg )
{
	poolAllocClear( &prg->mapElPool );
}

long mapElNumLost( program_t *prg )
{
	return poolAllocNumLost( &prg->mapElPool );
}
#endif

/* 
 * head_t
 */

head_t *headAllocate( program_t *prg )
{
	return (head_t*) poolAllocAllocate( &prg->headPool );
}

void headFree( program_t *prg, head_t *el )
{
	poolAllocFree( &prg->headPool, el );
}

void headClear( program_t *prg )
{
	poolAllocClear( &prg->headPool );
}

long headNumLost( program_t *prg )
{
	return poolAllocNumLost( &prg->headPool );
}

/* 
 * location_t
 */

location_t *locationAllocate( program_t *prg )
{
	return (location_t*) poolAllocAllocate( &prg->locationPool );
}

void locationFree( program_t *prg, location_t *el )
{
	poolAllocFree( &prg->locationPool, el );
}

void locationClear( program_t *prg )
{
	poolAllocClear( &prg->locationPool );
}

long locationNumLost( program_t *prg )
{
	return poolAllocNumLost( &prg->locationPool );
}

/*
 * stream_t
 */

#if 0
stream_t *streamAllocate( program_t *prg )
{
	return (stream_t*)mapElAllocate( prg );
}

void streamFree( program_t *prg, stream_t *stream )
{
	mapElFree( prg, (map_el_t*)stream );
}
#endif
