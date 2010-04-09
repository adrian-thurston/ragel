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

/* Nothing Yet */

#include "map.h"

void mapListAbandon( Map *map )
{
	map->head = map->tail = 0;
}



#if 0
/* Recursively delete all the children of a element. */
void mapDeleteChildrenOf( Map *map, MapEl *element )
{
	/* Recurse left. */
	if ( element->left ) {
		mapDeleteChildrenOf( map, element->left );

		/* Delete left element. */
		delete element->left;
		element->left = 0;
	}

	/* Recurse right. */
	if ( element->right ) {
		mapDeleteChildrenOf( map, element->right );

		/* Delete right element. */
		delete element->right;
		element->left = 0;
	}
}

void mapEmpty( Map *map )
{
	if ( map->root ) {
		/* Recursively delete from the tree structure. */
		mapDeleteChildrenOf( map, map->root );
		delete map->root;
		map->root = 0;
		map->treeSize = 0;

		mapListAbandon( map );
	}
}
#endif

/* rebalance from a element whose gradparent is unbalanced. Only
 * call on a element that has a grandparent. */
MapEl *mapRebalance( Map *map, MapEl *n )
{
	long lheight, rheight;
	MapEl *a, *b, *c;
	MapEl *t1, *t2, *t3, *t4;

	MapEl *p = n->parent; /* parent (Non-NUL). L*/
	MapEl *gp = p->parent; /* Grand-parent (Non-NULL). */
	MapEl *ggp = gp->parent; /* Great grand-parent (may be NULL). */

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
	mapRecalcHeights( map, ggp );
	return ggp;
}

/* Recalculates the heights of all the ancestors of element. */
void mapRecalcHeights( Map *map, MapEl *element )
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
MapEl *mapFindFirstUnbalGP( Map *map, MapEl *element )
{
	long lheight, rheight, balanceProp;
	MapEl *gp;

	if ( element == 0 || element->parent == 0 ||
			element->parent->parent == 0 )
		return 0;

	/* Don't do anything if we we have no grandparent. */
	gp = element->parent->parent;
	while ( gp != 0 )
	{
		lheight = gp->left ? gp->left->height : 0;
		rheight = gp->right ? gp->right->height : 0;
		balanceProp = lheight - rheight;

		if ( balanceProp < -1 || balanceProp > 1 )
			return element;

		element = element->parent;
		gp = gp->parent;
	}
	return 0;
}



/* Finds the first element that is unbalanced. */
MapEl *mapFindFirstUnbalEl( Map *map, MapEl *element )
{
	if ( element == 0 )
		return 0;

	while ( element != 0 )
	{
		long lheight = element->left ?
			element->left->height : 0;
		long rheight = element->right ?
			element->right->height : 0;
		long balanceProp = lheight - rheight;

		if ( balanceProp < -1 || balanceProp > 1 )
			return element;

		element = element->parent;
	}
	return 0;
}

/* Replace a element in the tree with another element not in the tree. */
void mapReplaceEl( Map *map, MapEl *element, MapEl *replacement )
{
	MapEl *parent = element->parent,
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
void mapRemoveEl( Map *map, MapEl *element, MapEl *filler )
{
	MapEl *parent = element->parent;

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



