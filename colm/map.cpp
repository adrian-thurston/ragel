/*
 *  Copyright 2008 Adrian Thurston <thurston@complang.org>
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

#include "pdarun.h"

MapEl *mapRebalance( Map *map, MapEl *n );

void mapListAddBefore( Map *map, MapEl *next_el, MapEl *new_el )
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

void mapListAddAfter( Map *map, MapEl *prev_el, MapEl *new_el )
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

MapEl *mapListDetach( Map *map, MapEl *el )
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


/* Recursive worker for tree copying. */
MapEl *mapCopyBranch( Program *p, Map *map, MapEl *el, Kid *oldNextDown, Kid *&newNextDown )
{
	/* Duplicate element. Either the base element's copy constructor or defaul
	 * constructor will get called. Both will suffice for initting the
	 * pointers to null when they need to be. */
	MapEl *newEl = mapElAllocate( p );

	if ( (Kid*)el == oldNextDown )
		newNextDown = (Kid*)newEl;

	/* If the left tree is there, copy it. */
	if ( newEl->left ) {
		newEl->left = mapCopyBranch( p, map, newEl->left, oldNextDown, newNextDown );
		newEl->left->parent = newEl;
	}

	mapListAddAfter( map, map->tail, newEl );

	/* If the right tree is there, copy it. */
	if ( newEl->right ) {
		newEl->right = mapCopyBranch( p, map, newEl->right, oldNextDown, newNextDown );
		newEl->right->parent = newEl;
	}

	return newEl;
}

/* Once an insertion position is found, attach a element to the tree. */
void mapAttachRebal( Map *map, MapEl *element, MapEl *parentEl, MapEl *lastLess )
{
	/* Increment the number of element in the tree. */
	map->treeSize += 1;

	/* Set element's parent. */
	element->parent = parentEl;

	/* New element always starts as a leaf with height 1. */
	element->left = 0;
	element->right = 0;
	element->height = 1;

	/* Are we inserting in the tree somewhere? */
	if ( parentEl != 0 ) {
		/* We have a parent so we are somewhere in the tree. If the parent
		 * equals lastLess, then the last traversal in the insertion went
		 * left, otherwise it went right. */
		if ( lastLess == parentEl ) {
			parentEl->left = element;

			mapListAddBefore( map, parentEl, element );
		}
		else {
			parentEl->right = element;

			mapListAddAfter( map, parentEl, element );
		}
	}
	else {
		/* No parent element so we are inserting the root. */
		map->root = element;

		mapListAddAfter( map, map->tail, element );
	}

	/* Recalculate the heights. */
	mapRecalcHeights( map, parentEl );

	/* Find the first unbalance. */
	MapEl *ub = mapFindFirstUnbalGP( map, element );

	/* rebalance. */
	if ( ub != 0 )
	{
		/* We assert that after this single rotation the 
		 * tree is now properly balanced. */
		mapRebalance( map, ub );
	}
}

/**
 * \brief Insert an existing element into the tree. 
 *
 * If the insert succeeds and lastFound is given then it is set to the element
 * inserted. If the insert fails then lastFound is set to the existing element in
 * the tree that has the same key as element. If the element's avl pointers are
 * already in use then undefined behaviour results.
 * 
 * \returns The element inserted upon success, null upon failure.
 */
MapEl *mapInsert( Program *prg, Map *map, MapEl *element, MapEl **lastFound )
{
	long keyRelation;
	MapEl *curEl = map->root, *parentEl = 0;
	MapEl *lastLess = 0;

	while (true) {
		if ( curEl == 0 ) {
			/* We are at an external element and did not find the key we were
			 * looking for. Attach underneath the leaf and rebalance. */
			mapAttachRebal( map, element, parentEl, lastLess );

			if ( lastFound != 0 )
				*lastFound = element;
			return element;
		}

		keyRelation = cmpTree( prg,
			element->key, curEl->key );

		/* Do we go left? */
		if ( keyRelation < 0 ) {
			parentEl = lastLess = curEl;
			curEl = curEl->left;
		}
		/* Do we go right? */
		else if ( keyRelation > 0 ) {
			parentEl = curEl;
			curEl = curEl->right;
		}
		/* We have hit the target. */
		else {
			if ( lastFound != 0 )
				*lastFound = curEl;
			return 0;
		}
	}
}

/**
 * \brief Insert a new element into the tree with given key.
 *
 * If the key is not already in the tree then a new element is made using the
 * MapEl(const Key &key) constructor and the insert succeeds. If lastFound is
 * given then it is set to the element inserted. If the insert fails then
 * lastFound is set to the existing element in the tree that has the same key as
 * element.
 * 
 * \returns The new element upon success, null upon failure.
 */
MapEl *mapInsert( Program *prg, Map *map, Tree *key, MapEl **lastFound )
{
	long keyRelation;
	MapEl *curEl = map->root, *parentEl = 0;
	MapEl *lastLess = 0;

	while (true) {
		if ( curEl == 0 ) {
			/* We are at an external element and did not find the key we were
			 * looking for. Create the new element, attach it underneath the leaf
			 * and rebalance. */
			MapEl *element = mapElAllocate( prg );
			element->key = key;
			element->tree = 0;
			mapAttachRebal( map, element, parentEl, lastLess );

			if ( lastFound != 0 )
				*lastFound = element;
			return element;
		}

		keyRelation = cmpTree( prg, key, curEl->key );

		/* Do we go left? */
		if ( keyRelation < 0 ) {
			parentEl = lastLess = curEl;
			curEl = curEl->left;
		}
		/* Do we go right? */
		else if ( keyRelation > 0 ) {
			parentEl = curEl;
			curEl = curEl->right;
		}
		/* We have hit the target. */
		else {
			if ( lastFound != 0 )
				*lastFound = curEl;
			return 0;
		}
	}
}

/**
 * \brief Find a element in the tree with the given key.
 *
 * \returns The element if key exists, null if the key does not exist.
 */
MapEl *mapImplFind( Program *prg, Map *map, Tree *key )
{
	MapEl *curEl = map->root;
	long keyRelation;

	while ( curEl != 0 ) {
		keyRelation = cmpTree( prg, key, curEl->key );

		/* Do we go left? */
		if ( keyRelation < 0 )
			curEl = curEl->left;
		/* Do we go right? */
		else if ( keyRelation > 0 )
			curEl = curEl->right;
		/* We have hit the target. */
		else {
			return curEl;
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
MapEl *mapDetachByKey( Program *prg, Map *map, Tree *key )
{
	MapEl *element = mapImplFind( prg, map, key );
	if ( element )
		mapDetach( prg, map, element );

	return element;
}

/**
 * \brief Find, detach and delete a element from the tree. 
 *
 * \returns True if the element was found and deleted, false otherwise.
 */
bool mapImplRemove( Program *prg, Map *map, Tree *key )
{
	/* Assume not found. */
	bool retVal = false;

	/* Look for the key. */
	MapEl *element = mapImplFind( prg, map, key );
	if ( element != 0 ) {
		/* If found, detach the element and delete. */
		mapDetach( prg, map, element );
		delete element;
		retVal = true;
	}

	return retVal;
}

/**
 * \brief Detach and delete a element from the tree. 
 *
 * If the element is not in the tree then undefined behaviour results.
 */
void mapImplRemove( Program *prg, Map *map, MapEl *element )
{
	/* Detach and delete. */
	mapDetach( prg, map, element );
	delete element;
}

/**
 * \brief Detach a element from the tree. 
 *
 * If the element is not in the tree then undefined behaviour results.
 * 
 * \returns The element given.
 */
MapEl *mapDetach( Program *prg, Map *map, MapEl *element )
{
	MapEl *replacement, *fixfrom;
	long lheight, rheight;

	/* Remove the element from the ordered list. */
	mapListDetach( map, element );

	/* Update treeSize. */
	map->treeSize--;

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

		mapRemoveEl( map, replacement, replacement->right );
		mapReplaceEl( map, element, replacement );
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

		mapRemoveEl( map, replacement, replacement->left );
		mapReplaceEl( map, element, replacement );
	}
	else
	{
		/* We need to start fixing at the parent of the element. */
		fixfrom = element->parent;

		/* The element we are deleting is a leaf element. */
		mapRemoveEl( map, element, 0 );
	}

	/* If fixfrom is null it means we just deleted
	 * the root of the tree. */
	if ( fixfrom == 0 )
		return element;

	/* Fix the heights after the deletion. */
	mapRecalcHeights( map, fixfrom );

	/* Fix every unbalanced element going up in the tree. */
	MapEl *ub = mapFindFirstUnbalEl( map, fixfrom );
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
		fixfrom = mapRebalance( map, ub );

		/* Find the next unbalaced element. */
		ub = mapFindFirstUnbalEl( map, fixfrom );
	}

	return element;
}

