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

void map_list_abandon( map_t *map );

void map_list_add_before( map_t *map, map_el_t *next_el, map_el_t *new_el );
void map_list_add_after( map_t *map, map_el_t *prev_el, map_el_t *new_el );
map_el_t *map_list_detach( map_t *map, map_el_t *el );
void map_attach_rebal( map_t *map, map_el_t *element, map_el_t *parent_el, map_el_t *last_less );
void map_delete_children_of( map_t *map, map_el_t *element );
void map_empty( map_t *map );
map_el_t *map_rebalance( map_t *map, map_el_t *n );
void map_recalc_heights( map_t *map, map_el_t *element );
map_el_t *mapFindFirstUnbalGP( map_t *map, map_el_t *element );
map_el_t *map_find_first_unbal_el( map_t *map, map_el_t *element );
void map_remove_el( map_t *map, map_el_t *element, map_el_t *filler );
void map_replace_el( map_t *map, map_el_t *element, map_el_t *replacement );
map_el_t *map_insert_el( program_t *prg, map_t *map, map_el_t *element, map_el_t **last_found );
map_el_t *map_insert_key( program_t *prg, map_t *map, tree_t *key, map_el_t **last_found );
map_el_t *map_impl_find( program_t *prg, map_t *map, tree_t *key );
map_el_t *map_detach_by_key( program_t *prg, map_t *map, tree_t *key );
map_el_t *map_detach( program_t *prg, map_t *map, map_el_t *element );
map_el_t *map_copy_branch( program_t *prg, map_t *map, map_el_t *el,
		kid_t *old_next_down, kid_t **new_next_down );

struct tree_pair map_remove( program_t *prg, map_t *map, tree_t *key );

long cmp_tree( program_t *prg, const tree_t *tree1, const tree_t *tree2 );

void map_impl_remove_el( program_t *prg, map_t *map, map_el_t *element );
int map_impl_remove_key( program_t *prg, map_t *map, tree_t *key );

tree_t *map_find( program_t *prg, map_t *map, tree_t *key );
long map_length( map_t *map );
tree_t *map_unstore( program_t *prg, map_t *map, tree_t *key, tree_t *existing );
int map_insert( program_t *prg, map_t *map, tree_t *key, tree_t *element );
void map_unremove( program_t *prg, map_t *map, tree_t *key, tree_t *element );
tree_t *map_uninsert( program_t *prg, map_t *map, tree_t *key );
tree_t *map_store( program_t *prg, map_t *map, tree_t *key, tree_t *element );

map_el_t *colm_map_insert( program_t *prg, map_t *map, map_el_t *map_el );
void colm_map_detach( program_t *prg, map_t *map, map_el_t *map_el );
map_el_t *colm_map_find( program_t *prg, map_t *map, tree_t *key );

map_el_t *colm_vmap_insert( program_t *prg, map_t *map, struct_t *key, struct_t *value );
map_el_t *colm_vmap_remove( program_t *prg, map_t *map, tree_t *key );
tree_t *colm_map_iter_advance( program_t *prg, tree_t ***psp, generic_iter_t *iter );
tree_t *colm_vmap_find( program_t *prg, map_t *map, tree_t *key );

#if defined(__cplusplus)
}
#endif

#endif

