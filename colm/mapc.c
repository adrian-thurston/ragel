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



