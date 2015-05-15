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

#ifndef _POOL_H
#define _POOL_H

/* Allocation, number of items. */
#define FRESH_BLOCK 8128                    

#include <colm/pdarun.h>
#include <colm/map.h>
#include <colm/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

void initPoolAlloc( struct pool_alloc *poolAlloc, int sizeofT );

kid_t *kidAllocate( program_t *prg );
void kidFree( program_t *prg, kid_t *el );
void kidClear( program_t *prg );
long kidNumLost( program_t *prg );

tree_t *treeAllocate( program_t *prg );
void treeFree( program_t *prg, tree_t *el );
void treeClear( program_t *prg );
long treeNumLost( program_t *prg );

parse_tree_t *parseTreeAllocate( program_t *prg );
void parseTreeFree( program_t *prg, parse_tree_t *el );
void parseTreeClear( program_t *prg );
long parseTreeNumLost( program_t *prg );

list_el_t *listElAllocate( program_t *prg );
void listElFree( program_t *prg, list_el_t *el );
void listElClear( program_t *prg );
long listElNumLost( program_t *prg );

map_el_t *mapElAllocate( program_t *prg );
void mapElFree( program_t *prg, map_el_t *el );
void mapElClear( program_t *prg );
long mapElNumLost( program_t *prg );

head_t *headAllocate( program_t *prg );
void headFree( program_t *prg, head_t *el );
void headClear( program_t *prg );
long headNumLost( program_t *prg );

location_t *locationAllocate( program_t *prg );
void locationFree( program_t *prg, location_t *el );
void locationClear( program_t *prg );
long locationNumLost( program_t *prg );

stream_t *streamAllocate( program_t *prg );
void streamFree( program_t *prg, stream_t *stream );

/* Wrong place. */
struct tree_pair mapRemove( program_t *prg, map_t *map, tree_t *key );

#ifdef __cplusplus
}
#endif

#endif
