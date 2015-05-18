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
#include <colm/struct.h>
#include "internal.h"

void mapListAbandon( map_t *map );

void mapListAddBefore( map_t *map, map_el_t *next_el, map_el_t *new_el );
void mapListAddAfter( map_t *map, map_el_t *prev_el, map_el_t *new_el );
map_el_t *mapListDetach( map_t *map, map_el_t *el );
void mapAttachRebal( map_t *map, map_el_t *element, map_el_t *parentEl, map_el_t *lastLess );
void mapDeleteChildrenOf( map_t *map, map_el_t *element );
void mapEmpty( map_t *map );
map_el_t *mapRebalance( map_t *map, map_el_t *n );
void mapRecalcHeights( map_t *map, map_el_t *element );
map_el_t *mapFindFirstUnbalGP( map_t *map, map_el_t *element );
map_el_t *mapFindFirstUnbalEl( map_t *map, map_el_t *element );
void mapRemoveEl( map_t *map, map_el_t *element, map_el_t *filler );
void mapReplaceEl( map_t *map, map_el_t *element, map_el_t *replacement );
map_el_t *mapInsertEl( program_t *prg, map_t *map, map_el_t *element, map_el_t **lastFound );
map_el_t *mapInsertKey( program_t *prg, map_t *map, tree_t *key, map_el_t **lastFound );
map_el_t *mapImplFind( program_t *prg, map_t *map, tree_t *key );
map_el_t *mapDetachByKey( program_t *prg, map_t *map, tree_t *key );
map_el_t *mapDetach( program_t *prg, map_t *map, map_el_t *element );
map_el_t *mapCopyBranch( program_t *prg, map_t *map, map_el_t *el,
		kid_t *oldNextDown, kid_t **newNextDown );

struct tree_pair mapRemove( program_t *prg, map_t *map, tree_t *key );

long cmpTree( program_t *prg, const tree_t *tree1, const tree_t *tree2 );

void mapImplRemoveEl( program_t *prg, map_t *map, map_el_t *element );
int mapImplRemoveKey( program_t *prg, map_t *map, tree_t *key );

tree_t *mapFind( program_t *prg, map_t *map, tree_t *key );
long mapLength( map_t *map );
tree_t *mapUnstore( program_t *prg, map_t *map, tree_t *key, tree_t *existing );
int mapInsert( program_t *prg, map_t *map, tree_t *key, tree_t *element );
void mapUnremove( program_t *prg, map_t *map, tree_t *key, tree_t *element );
tree_t *mapUninsert( program_t *prg, map_t *map, tree_t *key );
tree_t *mapStore( program_t *prg, map_t *map, tree_t *key, tree_t *element );

map_el_t *colm_map_insert( program_t *prg, map_t *map, map_el_t *mapEl );
void colm_map_detach( program_t *prg, map_t *map, map_el_t *mapEl );
map_el_t *colm_map_find( program_t *prg, map_t *map, tree_t *key );

map_el_t *colm_vmap_insert( program_t *prg, map_t *map, struct_t *key, struct_t *value );
map_el_t *colm_vmap_remove( program_t *prg, map_t *map, tree_t *key );
tree_t *colm_map_iter_advance( program_t *prg, tree_t ***psp, generic_iter_t *iter );
tree_t *colm_vmap_find( program_t *prg, map_t *map, tree_t *key );

#if defined(__cplusplus)
}
#endif

#endif

