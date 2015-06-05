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

#include <assert.h>
#include <colm/pdarun.h>
#include <colm/map.h>
#include <colm/pool.h>
#include <colm/bytecode.h>

#define true 1
#define false 0

struct colm_struct *colm_map_el_get( struct colm_program *prg,
		map_el_t *map_el, word_t gen_id, word_t field )
{
	struct generic_info *gi = &prg->rtd->generic_info[gen_id];
	map_el_t *result = 0;
	switch ( field ) {
		case 0: 
			result = map_el->prev;
			break;
		case 1: 
			result = map_el->next;
			break;
		default:
			assert( 0 );
			break;
	}

	struct colm_struct *s = result != 0 ?
			colm_struct_container( result, gi->el_offset ) : 0;
	return s;
}

struct colm_struct *colm_map_get( struct colm_program *prg,
		map_t *map, word_t gen_id, word_t field )
{
	struct generic_info *gi = &prg->rtd->generic_info[gen_id];
	map_el_t *result = 0;
	switch ( field ) {
		case 0: 
			result = map->head;
			break;
		case 1: 
			result = map->tail;
			break;
		default:
			assert( 0 );
			break;
	}

	struct colm_struct *s = result != 0 ?
			colm_struct_container( result, gi->el_offset ) : 0;
	return s;
}

void map_list_abandon( map_t *map )
{
	map->head = map->tail = 0;
}

void map_list_add_before( map_t *map, map_el_t *next_el, map_el_t *new_el )
{
	/* Set the next pointer of the new element to next_el. We do
	 * this regardless of the state of the list. */
	new_el->next = next_el; 

	/* Set reverse pointers. */
	if ( next_el == 0 ) {
		/* There is no next elememnt. We are inserting at the tail. */
		new_el->prev = map->tail;
		map->tail = new_el;
	} 
	else {
		/* There is a next element and we can access next's previous. */
		new_el->prev = next_el->prev;
		next_el->prev = new_el;
	} 

	/* Set forward pointers. */
	if ( new_el->prev == 0 ) {
		/* There is no previous element. Set the head pointer.*/
		map->head = new_el;
	}
	else {
		/* There is a previous element, set it's next pointer to new_el. */
		new_el->prev->next = new_el;
	}
}

void map_list_add_after( map_t *map, map_el_t *prev_el, map_el_t *new_el )
{
	/* Set the previous pointer of new_el to prev_el. We do
	 * this regardless of the state of the list. */
	new_el->prev = prev_el; 

	/* Set forward pointers. */
	if (prev_el == 0) {
		/* There was no prev_el, we are inserting at the head. */
		new_el->next = map->head;
		map->head = new_el;
	} 
	else {
		/* There was a prev_el, we can access previous next. */
		new_el->next = prev_el->next;
		prev_el->next = new_el;
	} 

	/* Set reverse pointers. */
	if (new_el->next == 0) {
		/* There is no next element. Set the tail pointer. */
		map->tail = new_el;
	}
	else {
		/* There is a next element. Set it's prev pointer. */
		new_el->next->prev = new_el;
	}
}


map_el_t *map_list_detach( map_t *map, map_el_t *el )
{
	/* Set forward pointers to skip over el. */
	if ( el->prev == 0 ) 
		map->head = el->next; 
	else
		el->prev->next = el->next; 

	/* Set reverse pointers to skip over el. */
	if ( el->next == 0 ) 
		map->tail = el->prev; 
	else
		el->next->prev = el->prev; 

	/* Update List length and return element we detached. */
	return el;
}


/* Once an insertion position is found, attach a element to the tree. */
void map_attach_rebal( map_t *map, map_el_t *element, map_el_t *parent_el, map_el_t *last_less )
{
	/* Increment the number of element in the tree. */
	map->tree_size += 1;

	/* Set element's parent. */
	element->parent = parent_el;

	/* New element always starts as a leaf with height 1. */
	element->left = 0;
	element->right = 0;
	element->height = 1;

	/* Are we inserting in the tree somewhere? */
	if ( parent_el != 0 ) {
		/* We have a parent so we are somewhere in the tree. If the parent
		 * equals lastLess, then the last traversal in the insertion went
		 * left, otherwise it went right. */
		if ( last_less == parent_el ) {
			parent_el->left = element;

			map_list_add_before( map, parent_el, element );
		}
		else {
			parent_el->right = element;

			map_list_add_after( map, parent_el, element );
		}
	}
	else {
		/* No parent element so we are inserting the root. */
		map->root = element;

		map_list_add_after( map, map->tail, element );
	}

	/* Recalculate the heights. */
	map_recalc_heights( map, parent_el );

	/* Find the first unbalance. */
	map_el_t *ub = mapFindFirstUnbalGP( map, element );

	/* rebalance. */
	if ( ub != 0 )
	{
		/* We assert that after this single rotation the 
		 * tree is now properly balanced. */
		map_rebalance( map, ub );
	}
}

#if 0
/* Recursively delete all the children of a element. */
void map_delete_children_of( map_t *map, map_el_t *element )
{
	/* Recurse left. */
	if ( element->left ) {
		map_delete_children_of( map, element->left );

		/* Delete left element. */
		delete element->left;
		element->left = 0;
	}

	/* Recurse right. */
	if ( element->right ) {
		map_delete_children_of( map, element->right );

		/* Delete right element. */
		delete element->right;
		element->left = 0;
	}
}

void map_empty( map_t *map )
{
	if ( map->root ) {
		/* Recursively delete from the tree structure. */
		map_delete_children_of( map, map->root );
		delete map->root;
		map->root = 0;
		map->tree_size = 0;

		map_list_abandon( map );
	}
}
#endif

/* rebalance from a element whose gradparent is unbalanced. Only
 * call on a element that has a grandparent. */
map_el_t *map_rebalance( map_t *map, map_el_t *n )
{
	long lheight, rheight;
	map_el_t *a, *b, *c;
	map_el_t *t1, *t2, *t3, *t4;

	map_el_t *p = n->parent; /* parent (Non-NUL). L*/
	map_el_t *gp = p->parent; /* Grand-parent (Non-NULL). */
	map_el_t *ggp = gp->parent; /* Great grand-parent (may be NULL). */

	if (gp->right == p)
	{
		/*  gp
		 *   		 *    p
		 p
		 */
		if (p->right == n)
		{
			/*  gp
			 *   			 *    p
			 p
			 *     			 *      n
			 n
			 */
			a = gp;
			b = p;
			c = n;
			t1 = gp->left;
			t2 = p->left;
			t3 = n->left;
			t4 = n->right;
		}
		else
		{
			/*  gp
			 *     			 *       p
			 p
			 *      /
			 *     n
			 */
			a = gp;
			b = n;
			c = p;
			t1 = gp->left;
			t2 = n->left;
			t3 = n->right;
			t4 = p->right;
		}
	}
	else
	{
		/*    gp
		 *   /
		 *  p
		 */
		if (p->right == n)
		{
			/*      gp
			 *    /
			 *  p
			 *   			 *    n
			 n
			 */
			a = p;
			b = n;
			c = gp;
			t1 = p->left;
			t2 = n->left;
			t3 = n->right;
			t4 = gp->right;
		}
		else
		{
			/*      gp
			 *     /
			 *    p
			 *   /
			 *  n
			 */
			a = n;
			b = p;
			c = gp;
			t1 = n->left;
			t2 = n->right;
			t3 = p->right;
			t4 = gp->right;
		}
	}

	/* Perform rotation.
	*/

	/* Tie b to the great grandparent. */
	if ( ggp == 0 )
		map->root = b;
	else if ( ggp->left == gp )
		ggp->left = b;
	else
		ggp->right = b;
	b->parent = ggp;

	/* Tie a as a leftchild of b. */
	b->left = a;
	a->parent = b;

	/* Tie c as a rightchild of b. */
	b->right = c;
	c->parent = b;

	/* Tie t1 as a leftchild of a. */
	a->left = t1;
	if ( t1 != 0 ) t1->parent = a;

	/* Tie t2 as a rightchild of a. */
	a->right = t2;
	if ( t2 != 0 ) t2->parent = a;

	/* Tie t3 as a leftchild of c. */
	c->left = t3;
	if ( t3 != 0 ) t3->parent = c;

	/* Tie t4 as a rightchild of c. */
	c->right = t4;
	if ( t4 != 0 ) t4->parent = c;

	/* The heights are all recalculated manualy and the great
	 * grand-parent is passed to recalcHeights() to ensure
	 * the heights are correct up the tree.
	 *
	 * Note that recalcHeights() cuts out when it comes across
	 * a height that hasn't changed.
	 */

	/* Fix height of a. */
	lheight = a->left ? a->left->height : 0;
	rheight = a->right ? a->right->height : 0;
	a->height = (lheight > rheight ? lheight : rheight) + 1;

	/* Fix height of c. */
	lheight = c->left ? c->left->height : 0;
	rheight = c->right ? c->right->height : 0;
	c->height = (lheight > rheight ? lheight : rheight) + 1;

	/* Fix height of b. */
	lheight = a->height;
	rheight = c->height;
	b->height = (lheight > rheight ? lheight : rheight) + 1;

	/* Fix height of b's parents. */
	map_recalc_heights( map, ggp );
	return ggp;
}

/* Recalculates the heights of all the ancestors of element. */
void map_recalc_heights( map_t *map, map_el_t *element )
{
	while ( element != 0 )
	{
		long lheight = element->left ? element->left->height : 0;
		long rheight = element->right ? element->right->height : 0;

		long new_height = (lheight > rheight ? lheight : rheight) + 1;

		/* If there is no chage in the height, then there will be no
		 * change in any of the ancestor's height. We can stop going up.
		 * If there was a change, continue upward. */
		if (new_height == element->height)
			return;
		else
			element->height = new_height;

		element = element->parent;
	}
}

/* Finds the first element whose grandparent is unbalanced. */
map_el_t *mapFindFirstUnbalGP( map_t *map, map_el_t *element )
{
	long lheight, rheight, balance_prop;
	map_el_t *gp;

	if ( element == 0 || element->parent == 0 ||
			element->parent->parent == 0 )
		return 0;

	/* Don't do anything if we we have no grandparent. */
	gp = element->parent->parent;
	while ( gp != 0 )
	{
		lheight = gp->left ? gp->left->height : 0;
		rheight = gp->right ? gp->right->height : 0;
		balance_prop = lheight - rheight;

		if ( balance_prop < -1 || balance_prop > 1 )
			return element;

		element = element->parent;
		gp = gp->parent;
	}
	return 0;
}



/* Finds the first element that is unbalanced. */
map_el_t *map_find_first_unbal_el( map_t *map, map_el_t *element )
{
	if ( element == 0 )
		return 0;

	while ( element != 0 )
	{
		long lheight = element->left ?
			element->left->height : 0;
		long rheight = element->right ?
			element->right->height : 0;
		long balance_prop = lheight - rheight;

		if ( balance_prop < -1 || balance_prop > 1 )
			return element;

		element = element->parent;
	}
	return 0;
}

/* Replace a element in the tree with another element not in the tree. */
void map_replace_el( map_t *map, map_el_t *element, map_el_t *replacement )
{
	map_el_t *parent = element->parent,
			*left = element->left,
			*right = element->right;

	replacement->left = left;
	if (left)
		left->parent = replacement;
	replacement->right = right;
	if (right)
		right->parent = replacement;

	replacement->parent = parent;
	if (parent)
	{
		if (parent->left == element)
			parent->left = replacement;
		else
			parent->right = replacement;
	}
	else {
		map->root = replacement;
	}

	replacement->height = element->height;
}


/* Removes a element from a tree and puts filler in it's place.
 * Filler should be null or a child of element. */
void map_remove_el( map_t *map, map_el_t *element, map_el_t *filler )
{
	map_el_t *parent = element->parent;

	if ( parent )
	{
		if ( parent->left == element )
			parent->left = filler;
		else
			parent->right = filler;
	}
	else {
		map->root = filler;
	}

	if ( filler )
		filler->parent = parent;

	return;
}

#if 0
/* Recursive worker for tree copying. */
map_el_t *map_copy_branch( program_t *prg, map_t *map, map_el_t *el, kid_t *old_next_down, kid_t **new_next_down )
{
	/* Duplicate element. Either the base element's copy constructor or defaul
	 * constructor will get called. Both will suffice for initting the
	 * pointers to null when they need to be. */
	map_el_t *new_el = map_el_allocate( prg );

	if ( (kid_t*)el == old_next_down )
		*new_next_down = (kid_t*)new_el;

	/* If the left tree is there, copy it. */
	if ( new_el->left ) {
		new_el->left = map_copy_branch( prg, map, new_el->left, old_next_down, new_next_down );
		new_el->left->parent = new_el;
	}

	map_list_add_after( map, map->tail, new_el );

	/* If the right tree is there, copy it. */
	if ( new_el->right ) {
		new_el->right = map_copy_branch( prg, map, new_el->right, old_next_down, new_next_down );
		new_el->right->parent = new_el;
	}

	return new_el;
}
#endif

static long map_cmp( program_t *prg, map_t *map, const tree_t *tree1, const tree_t *tree2 )
{
	if ( map->generic_info->key_type == TYPE_TREE ) {
		return colm_cmp_tree( prg, tree1, tree2 );
	}
	else {
		if ( (long)tree1 < (long)tree2 )
			return -1;
		else if ( (long)tree1 > (long)tree2)
			return 1;
		return 0;
	}
}

map_el_t *map_insert_el( program_t *prg, map_t *map, map_el_t *element, map_el_t **last_found )
{
	long key_relation;
	map_el_t *cur_el = map->root, *parent_el = 0;
	map_el_t *last_less = 0;

	while ( true ) {
		if ( cur_el == 0 ) {
			/* We are at an external element and did not find the key we were
			 * looking for. Attach underneath the leaf and rebalance. */
			map_attach_rebal( map, element, parent_el, last_less );

			if ( last_found != 0 )
				*last_found = element;
			return element;
		}

		key_relation = map_cmp( prg, map,
			element->key, cur_el->key );

		/* Do we go left? */
		if ( key_relation < 0 ) {
			parent_el = last_less = cur_el;
			cur_el = cur_el->left;
		}
		/* Do we go right? */
		else if ( key_relation > 0 ) {
			parent_el = cur_el;
			cur_el = cur_el->right;
		}
		/* We have hit the target. */
		else {
			if ( last_found != 0 )
				*last_found = cur_el;
			return 0;
		}
	}
}

#if 0
map_el_t *map_insert_key( program_t *prg, map_t *map, tree_t *key, map_el_t **last_found )
{
	long key_relation;
	map_el_t *cur_el = map->root, *parent_el = 0;
	map_el_t *last_less = 0;

	while ( true ) {
		if ( cur_el == 0 ) {
			/* We are at an external element and did not find the key we were
			 * looking for. Create the new element, attach it underneath the leaf
			 * and rebalance. */
			map_el_t *element = map_el_allocate( prg );
			element->key = key;
			map_attach_rebal( map, element, parent_el, last_less );

			if ( last_found != 0 )
				*last_found = element;
			return element;
		}

		key_relation = map_cmp( prg, map, key, cur_el->key );

		/* Do we go left? */
		if ( key_relation < 0 ) {
			parent_el = last_less = cur_el;
			cur_el = cur_el->left;
		}
		/* Do we go right? */
		else if ( key_relation > 0 ) {
			parent_el = cur_el;
			cur_el = cur_el->right;
		}
		/* We have hit the target. */
		else {
			if ( last_found != 0 )
				*last_found = cur_el;
			return 0;
		}
	}
}
#endif

map_el_t *colm_map_insert( program_t *prg, map_t *map, map_el_t *map_el )
{
	return map_insert_el( prg, map, map_el, 0 );
}

map_el_t *colm_vmap_insert( program_t *prg, map_t *map, struct_t *key, struct_t *value )
{
	struct colm_struct *s = colm_struct_new( prg, map->generic_info->el_struct_id );

	colm_struct_set_field( s, struct_t*, map->generic_info->el_offset, key );
	colm_struct_set_field( s, struct_t*, 0, value );

	map_el_t *map_el = colm_struct_get_addr( s, map_el_t*, map->generic_info->el_offset );

	return colm_map_insert( prg, map, map_el );
}

map_el_t *colm_vmap_remove( program_t *prg, map_t *map, tree_t *key )
{
	map_el_t *map_el = colm_map_find( prg, map, key );
	if ( map_el != 0 )
		colm_map_detach( prg, map, map_el );
	return 0;
}

tree_t *colm_vmap_find( program_t *prg, map_t *map, tree_t *key )
{
	map_el_t *map_el = colm_map_find( prg, map, key );
	if ( map_el != 0 ) {
		struct_t *s = colm_generic_el_container( prg, map_el,
				map->generic_info - prg->rtd->generic_info );
		tree_t *val = colm_struct_get_field( s, tree_t*, 0 );

		if ( map->generic_info->value_type == TYPE_TREE )
			colm_tree_upref( val );

		return val;
	}
	return 0;
}

void colm_map_detach( program_t *prg, map_t *map, map_el_t *map_el )
{
	map_detach( prg, map, map_el );
}

map_el_t *colm_map_find( program_t *prg, map_t *map, tree_t *key )
{
	return map_impl_find( prg, map, key );
}

/**
 * \brief Find a element in the tree with the given key.
 *
 * \returns The element if key exists, null if the key does not exist.
 */
map_el_t *map_impl_find( program_t *prg, map_t *map, tree_t *key )
{
	map_el_t *cur_el = map->root;
	long key_relation;

	while ( cur_el != 0 ) {
		key_relation = map_cmp( prg, map, key, cur_el->key );

		/* Do we go left? */
		if ( key_relation < 0 )
			cur_el = cur_el->left;
		/* Do we go right? */
		else if ( key_relation > 0 )
			cur_el = cur_el->right;
		/* We have hit the target. */
		else {
			return cur_el;
		}
	}
	return 0;
}


/**
 * \brief Find a element, then detach it from the tree. 
 * 
 * The element is not deleted.
 *
 * \returns The element detached if the key is found, othewise returns null.
 */
map_el_t *map_detach_by_key( program_t *prg, map_t *map, tree_t *key )
{
	map_el_t *element = map_impl_find( prg, map, key );
	if ( element )
		map_detach( prg, map, element );

	return element;
}

/**
 * \brief Detach a element from the tree. 
 *
 * If the element is not in the tree then undefined behaviour results.
 * 
 * \returns The element given.
 */
map_el_t *map_detach( program_t *prg, map_t *map, map_el_t *element )
{
	map_el_t *replacement, *fixfrom;
	long lheight, rheight;

	/* Remove the element from the ordered list. */
	map_list_detach( map, element );

	/* Update treeSize. */
	map->tree_size--;

	/* Find a replacement element. */
	if (element->right)
	{
		/* Find the leftmost element of the right subtree. */
		replacement = element->right;
		while (replacement->left)
			replacement = replacement->left;

		/* If replacing the element the with its child then we need to start
		 * fixing at the replacement, otherwise we start fixing at the
		 * parent of the replacement. */
		if (replacement->parent == element)
			fixfrom = replacement;
		else
			fixfrom = replacement->parent;

		map_remove_el( map, replacement, replacement->right );
		map_replace_el( map, element, replacement );
	}
	else if (element->left)
	{
		/* Find the rightmost element of the left subtree. */
		replacement = element->left;
		while (replacement->right)
			replacement = replacement->right;

		/* If replacing the element the with its child then we need to start
		 * fixing at the replacement, otherwise we start fixing at the
		 * parent of the replacement. */
		if (replacement->parent == element)
			fixfrom = replacement;
		else
			fixfrom = replacement->parent;

		map_remove_el( map, replacement, replacement->left );
		map_replace_el( map, element, replacement );
	}
	else
	{
		/* We need to start fixing at the parent of the element. */
		fixfrom = element->parent;

		/* The element we are deleting is a leaf element. */
		map_remove_el( map, element, 0 );
	}

	/* If fixfrom is null it means we just deleted
	 * the root of the tree. */
	if ( fixfrom == 0 )
		return element;

	/* Fix the heights after the deletion. */
	map_recalc_heights( map, fixfrom );

	/* Fix every unbalanced element going up in the tree. */
	map_el_t *ub = map_find_first_unbal_el( map, fixfrom );
	while ( ub )
	{
		/* Find the element to rebalance by moving down from the first unbalanced
		 * element 2 levels in the direction of the greatest heights. On the
		 * second move down, the heights may be equal ( but not on the first ).
		 * In which case go in the direction of the first move. */
		lheight = ub->left ? ub->left->height : 0;
		rheight = ub->right ? ub->right->height : 0;
		assert( lheight != rheight );
		if (rheight > lheight)
		{
			ub = ub->right;
			lheight = ub->left ?
				ub->left->height : 0;
			rheight = ub->right ?
				ub->right->height : 0;
			if (rheight > lheight)
				ub = ub->right;
			else if (rheight < lheight)
				ub = ub->left;
			else
				ub = ub->right;
		}
		else
		{
			ub = ub->left;
			lheight = ub->left ?
				ub->left->height : 0;
			rheight = ub->right ?
				ub->right->height : 0;
			if (rheight > lheight)
				ub = ub->right;
			else if (rheight < lheight)
				ub = ub->left;
			else
				ub = ub->left;
		}


		/* rebalance returns the grandparant of the subtree formed
		 * by the element that were rebalanced.
		 * We must continue upward from there rebalancing. */
		fixfrom = map_rebalance( map, ub );

		/* Find the next unbalaced element. */
		ub = map_find_first_unbal_el( map, fixfrom );
	}

	return element;
}



