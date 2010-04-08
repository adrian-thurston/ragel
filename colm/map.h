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

typedef struct _MapEl MapEl;
typedef struct _GenericInfo GenericInfo;

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

#if defined(__cplusplus)
extern "C" {
#endif

void mapListAbandon( Map *map );

#if defined(__cplusplus)
}
#endif

