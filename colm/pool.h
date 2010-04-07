/*
 *  Copyright 2010 Adrian Thurston <thurston@complang.org>
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

#ifndef _POOL_H
#define _POOL_H

#include <iostream>

using std::cerr;
using std::endl;
using std::ostream;

/* Allocation, number of items. */
#define FRESH_BLOCK 8128                    

struct PoolItem
{
	PoolItem *next;
};

template <class T> struct PoolBlock
{
	T data[FRESH_BLOCK];
	PoolBlock<T> *next;
};

template <class T> struct PoolAlloc
{
	PoolAlloc() : 
		head(0), nextel(FRESH_BLOCK), pool(0)
	{}

	T *_allocate();
	void free( T *el );
	void clear();
	long numLost();

private:

	PoolBlock<T> *head;
	long nextel;
	PoolItem *pool;
};

template <class T> T *PoolAlloc<T>::_allocate()
{
	//#ifdef COLM_LOG_BYTECODE
	//cerr << "allocating in: " << __PRETTY_FUNCTION__ << endl;
	//#endif
	T *newEl = 0;
	if ( pool == 0 ) {
		if ( nextel == FRESH_BLOCK ) {
			#ifdef COLM_LOG_BYTECODE
			if ( colm_log_bytecode )
				cerr << "allocating " << FRESH_BLOCK << " Elements of type T" << endl;
			#endif

			PoolBlock<T> *newBlock = new PoolBlock<T>;
			newBlock->next = head;
			head = newBlock;
			nextel = 0;
		}
		newEl = &head->data[nextel++];
	}
	else {
		newEl = (T*)pool;
		pool = pool->next;
	}
	memset( newEl, 0, sizeof(T) );
	return newEl;
}

template <class T> void PoolAlloc<T>::free( T *el )
{
	#if 0
	/* Some sanity checking. Best not to normally run with this on. */
	char *p = (char*)el + sizeof(PoolItem*);
	char *pe = (char*)el + sizeof(T);
	for ( ; p < pe; p++ )
		assert( *p != 0xcc );
	memset( el, 0xcc, sizeof(T) );
	#endif

	PoolItem *pi = (PoolItem*) el;
	pi->next = pool;
	pool = pi;
}

template <class T> void PoolAlloc<T>::clear()
{
	PoolBlock<T> *block = head;
	while ( block != 0 ) {
		PoolBlock<T> *next = block->next;
		delete block;
		block = next;
	}

	head = 0;
	nextel = 0;
	pool = 0;
}

template <class T> long PoolAlloc<T>::numLost()
{
	/* Count the number of items allocated. */
	long lost = 0;
	PoolBlock<T> *block = head;
	if ( block != 0 ) {
		lost = nextel;
		block = block->next;
		while ( block != 0 ) {
			lost += FRESH_BLOCK;
			block = block->next;
		}
	}

	/* Subtract. Items that are on the free list. */
	PoolItem *pi = pool;
	while ( pi != 0 ) {
		lost -= 1;
		pi = pi->next;
	}

	return lost;
}

struct Program;
struct Kid;
struct Tree;
struct ParseTree;
struct ListEl;
struct MapEl;
struct Head;
struct Location;

Kid *kidAllocate( Program *prg );
void kidFree( Program *prg, Kid *el );
void kidClear( Program *prg );
long kidNumlost( Program *prg );

Tree *treeAllocate( Program *prg );
void treeFree( Program *prg, Tree *el );
void treeClear( Program *prg );
long treeNumlost( Program *prg );

ParseTree *parseTreeAllocate( Program *prg );
void parseTreeFree( Program *prg, ParseTree *el );
void parseTreeClear( Program *prg );
long parseTreeNumlost( Program *prg );

ListEl *listElAllocate( Program *prg );
void listElFree( Program *prg, ListEl *el );
void listElClear( Program *prg );
long listElNumlost( Program *prg );

MapEl *mapElAllocate( Program *prg );
void mapElFree( Program *prg, MapEl *el );
void mapElClear( Program *prg );
long mapElNumlost( Program *prg );

Head *headAllocate( Program *prg );
void headFree( Program *prg, Head *el );
void headClear( Program *prg );
long headNumlost( Program *prg );

Location *locationAllocate( Program *prg );
void locationFree( Program *prg, Location *el );
void locationClear( Program *prg );
long locationNumlost( Program *prg );

#endif
