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

void initPoolAlloc( PoolAlloc *poolAlloc, int sizeofT )
{
	poolAlloc->head = 0;
	poolAlloc->nextel = FRESH_BLOCK;
	poolAlloc->pool = 0;
	poolAlloc->sizeofT = sizeofT;
}

void *poolAllocAllocate( PoolAlloc *poolAlloc )
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
			PoolBlock *newBlock = (PoolBlock*)malloc( sizeof(PoolBlock) );
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

void poolAllocFree( PoolAlloc *poolAlloc, void *el )
{
	#if 0
	/* Some sanity checking. Best not to normally run with this on. */
	char *p = (char*)el + sizeof(PoolItem*);
	char *pe = (char*)el + sizeof(T);
	for ( ; p < pe; p++ )
		assert( *p != 0xcc );
	memset( el, 0xcc, sizeof(T) );
	#endif

#ifdef POOL_MALLOC
	free( el );
#else
	PoolItem *pi = (PoolItem*) el;
	pi->next = poolAlloc->pool;
	poolAlloc->pool = pi;
#endif
}

void poolAllocClear( PoolAlloc *poolAlloc )
{
	PoolBlock *block = poolAlloc->head;
	while ( block != 0 ) {
		PoolBlock *next = block->next;
		free( block->data );
		free( block );
		block = next;
	}

	poolAlloc->head = 0;
	poolAlloc->nextel = 0;
	poolAlloc->pool = 0;
}

long poolAllocNumLost( PoolAlloc *poolAlloc )
{
	/* Count the number of items allocated. */
	long lost = 0;
	PoolBlock *block = poolAlloc->head;
	if ( block != 0 ) {
		lost = poolAlloc->nextel;
		block = block->next;
		while ( block != 0 ) {
			lost += FRESH_BLOCK;
			block = block->next;
		}
	}

	/* Subtract. Items that are on the free list. */
	PoolItem *pi = poolAlloc->pool;
	while ( pi != 0 ) {
		lost -= 1;
		pi = pi->next;
	}

	return lost;
}

/* 
 * Kid
 */

Kid *kidAllocate( Program *prg )
{
	return (Kid*) poolAllocAllocate( &prg->kidPool );
}

void kidFree( Program *prg, Kid *el )
{
	poolAllocFree( &prg->kidPool, el );
}

void kidClear( Program *prg )
{
	poolAllocClear( &prg->kidPool );
}

long kidNumLost( Program *prg )
{
	return poolAllocNumLost( &prg->kidPool );
}

/* 
 * Tree
 */

Tree *treeAllocate( Program *prg )
{
	return (Tree*) poolAllocAllocate( &prg->treePool );
}

void treeFree( Program *prg, Tree *el )
{
	poolAllocFree( &prg->treePool, el );
}

void treeClear( Program *prg )
{
	poolAllocClear( &prg->treePool );
}

long treeNumLost( Program *prg )
{
	return poolAllocNumLost( &prg->treePool );
}

/* 
 * ParseTree
 */

ParseTree *parseTreeAllocate( Program *prg )
{
	return (ParseTree*) poolAllocAllocate( &prg->parseTreePool );
}

void parseTreeFree( Program *prg, ParseTree *el )
{
	poolAllocFree( &prg->parseTreePool, el );
}

void parseTreeClear( Program *prg )
{
	poolAllocClear( &prg->parseTreePool );
}

long parseTreeNumLost( Program *prg )
{
	return poolAllocNumLost( &prg->parseTreePool );
}

/* 
 * ListEl
 */

ListEl *listElAllocate( Program *prg )
{
	return (ListEl*) poolAllocAllocate( &prg->listElPool );
}

void listElFree( Program *prg, ListEl *el )
{
	poolAllocFree( &prg->listElPool, el );
}

void listElClear( Program *prg )
{
	poolAllocClear( &prg->listElPool );
}

long listElNumLost( Program *prg )
{
	return poolAllocNumLost( &prg->listElPool );
}

/* 
 * MapEl
 */

MapEl *mapElAllocate( Program *prg )
{
	return (MapEl*) poolAllocAllocate( &prg->mapElPool );
}

void mapElFree( Program *prg, MapEl *el )
{
	poolAllocFree( &prg->mapElPool, el );
}

void mapElClear( Program *prg )
{
	poolAllocClear( &prg->mapElPool );
}

long mapElNumLost( Program *prg )
{
	return poolAllocNumLost( &prg->mapElPool );
}

/* 
 * Head
 */

Head *headAllocate( Program *prg )
{
	return (Head*) poolAllocAllocate( &prg->headPool );
}

void headFree( Program *prg, Head *el )
{
	poolAllocFree( &prg->headPool, el );
}

void headClear( Program *prg )
{
	poolAllocClear( &prg->headPool );
}

long headNumLost( Program *prg )
{
	return poolAllocNumLost( &prg->headPool );
}

/* 
 * Location
 */

Location *locationAllocate( Program *prg )
{
	return (Location*) poolAllocAllocate( &prg->locationPool );
}

void locationFree( Program *prg, Location *el )
{
	poolAllocFree( &prg->locationPool, el );
}

void locationClear( Program *prg )
{
	poolAllocClear( &prg->locationPool );
}

long locationNumLost( Program *prg )
{
	return poolAllocNumLost( &prg->locationPool );
}

/*
 * Stream
 */

Stream *streamAllocate( Program *prg )
{
	return (Stream*)mapElAllocate( prg );
}

void streamFree( Program *prg, Stream *stream )
{
	mapElFree( prg, (MapEl*)stream );
}
