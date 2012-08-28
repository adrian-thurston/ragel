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

#ifndef _MAP_H
#define _MAP_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <colm/program.h>

typedef struct _MapEl
{
	/* Must overlay Kid. */
	Tree *tree;
	struct _MapEl *next;
	struct _MapEl *prev;

	struct _MapEl *left, *right, *parent;
	long height;
	Tree *key;
} MapEl;

typedef struct _Map
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	MapEl *head;

	MapEl *tail;
	MapEl *root;
	long treeSize;
	GenericInfo *genericInfo;
} Map;

void mapListAbandon( Map *map );

void mapListAddBefore( Map *map, MapEl *next_el, MapEl *new_el );
void mapListAddAfter( Map *map, MapEl *prev_el, MapEl *new_el );
MapEl *mapListDetach( Map *map, MapEl *el );
void mapAttachRebal( Map *map, MapEl *element, MapEl *parentEl, MapEl *lastLess );
void mapDeleteChildrenOf( Map *map, MapEl *element );
void mapEmpty( Map *map );
MapEl *mapRebalance( Map *map, MapEl *n );
void mapRecalcHeights( Map *map, MapEl *element );
MapEl *mapFindFirstUnbalGP( Map *map, MapEl *element );
MapEl *mapFindFirstUnbalEl( Map *map, MapEl *element );
void mapRemoveEl( Map *map, MapEl *element, MapEl *filler );
void mapReplaceEl( Map *map, MapEl *element, MapEl *replacement );
MapEl *mapInsertEl( Program *prg, Map *map, MapEl *element, MapEl **lastFound );
MapEl *mapInsertKey( Program *prg, Map *map, Tree *key, MapEl **lastFound );
MapEl *mapImplFind( Program *prg, Map *map, Tree *key );
MapEl *mapDetachByKey( Program *prg, Map *map, Tree *key );
MapEl *mapDetach( Program *prg, Map *map, MapEl *element );
MapEl *mapCopyBranch( Program *prg, Map *map, MapEl *el, Kid *oldNextDown, Kid **newNextDown );

long cmpTree( Program *prg, const Tree *tree1, const Tree *tree2 );

void mapImplRemoveEl( Program *prg, Map *map, MapEl *element );
int mapImplRemoveKey( Program *prg, Map *map, Tree *key );

Tree *mapFind( Program *prg, Map *map, Tree *key );
long mapLength( Map *map );
Tree *mapUnstore( Program *prg, Map *map, Tree *key, Tree *existing );
int mapInsert( Program *prg, Map *map, Tree *key, Tree *element );
void mapUnremove( Program *prg, Map *map, Tree *key, Tree *element );
Tree *mapUninsert( Program *prg, Map *map, Tree *key );
Tree *mapStore( Program *prg, Map *map, Tree *key, Tree *element );


#if defined(__cplusplus)
}
#endif

#endif

