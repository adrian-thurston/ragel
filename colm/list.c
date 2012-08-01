/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
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

#include <colm/pdarun.h>

void listAddAfter( List *list, ListEl *prev_el, ListEl *new_el )
{
	/* Set the previous pointer of new_el to prev_el. We do
	 * this regardless of the state of the list. */
	new_el->prev = prev_el; 

	/* Set forward pointers. */
	if (prev_el == 0) {
		/* There was no prev_el, we are inserting at the head. */
		new_el->next = list->head;
		list->head = new_el;
	} 
	else {
		/* There was a prev_el, we can access previous next. */
		new_el->next = prev_el->next;
		prev_el->next = new_el;
	} 

	/* Set reverse pointers. */
	if (new_el->next == 0) {
		/* There is no next element. Set the tail pointer. */
		list->tail = new_el;
	}
	else {
		/* There is a next element. Set it's prev pointer. */
		new_el->next->prev = new_el;
	}

	/* Update list length. */
	list->listLen++;
}

void listAddBefore( List *list, ListEl *next_el, ListEl *new_el)
{
	/* Set the next pointer of the new element to next_el. We do
	 * this regardless of the state of the list. */
	new_el->next = next_el; 

	/* Set reverse pointers. */
	if (next_el == 0) {
		/* There is no next elememnt. We are inserting at the tail. */
		new_el->prev = list->tail;
		list->tail = new_el;
	} 
	else {
		/* There is a next element and we can access next's previous. */
		new_el->prev = next_el->prev;
		next_el->prev = new_el;
	} 

	/* Set forward pointers. */
	if (new_el->prev == 0) {
		/* There is no previous element. Set the head pointer.*/
		list->head = new_el;
	}
	else {
		/* There is a previous element, set it's next pointer to new_el. */
		new_el->prev->next = new_el;
	}

	list->listLen++;
}

ListEl *listDetach( List *list, ListEl *el )
{
	/* Set forward pointers to skip over el. */
	if (el->prev == 0) 
		list->head = el->next; 
	else
		el->prev->next = el->next; 

	/* Set reverse pointers to skip over el. */
	if (el->next == 0) 
		list->tail = el->prev; 
	else
		el->next->prev = el->prev; 

	/* Update List length and return element we detached. */
	list->listLen--;
	return el;
}

