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

#ifndef _COLM_MAP_H
#define _COLM_MAP_H

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

#endif /* _COLM_MAP_H */

