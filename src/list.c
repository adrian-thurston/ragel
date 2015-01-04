/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
 */

#include <colm/pdarun.h>
#include <colm/struct.h>

void listAddAfter( List *list, ListEl *prev_el, ListEl *new_el )
{
	/* Set the previous pointer of new_el to prev_el. We do
	 * this regardless of the state of the list. */
	new_el->list_prev = prev_el; 

	/* Set forward pointers. */
	if (prev_el == 0) {
		/* There was no prev_el, we are inserting at the head. */
		new_el->list_next = list->head;
		list->head = new_el;
	} 
	else {
		/* There was a prev_el, we can access previous next. */
		new_el->list_next = prev_el->list_next;
		prev_el->list_next = new_el;
	} 

	/* Set reverse pointers. */
	if (new_el->list_next == 0) {
		/* There is no next element. Set the tail pointer. */
		list->tail = new_el;
	}
	else {
		/* There is a next element. Set it's prev pointer. */
		new_el->list_next->list_prev = new_el;
	}

	/* Update list length. */
	list->listLen++;
}

void listAddBefore( List *list, ListEl *next_el, ListEl *new_el)
{
	/* Set the next pointer of the new element to next_el. We do
	 * this regardless of the state of the list. */
	new_el->list_next = next_el; 

	/* Set reverse pointers. */
	if (next_el == 0) {
		/* There is no next elememnt. We are inserting at the tail. */
		new_el->list_prev = list->tail;
		list->tail = new_el;
	} 
	else {
		/* There is a next element and we can access next's previous. */
		new_el->list_prev = next_el->list_prev;
		next_el->list_prev = new_el;
	} 

	/* Set forward pointers. */
	if (new_el->list_prev == 0) {
		/* There is no previous element. Set the head pointer.*/
		list->head = new_el;
	}
	else {
		/* There is a previous element, set it's next pointer to new_el. */
		new_el->list_prev->list_next = new_el;
	}

	list->listLen++;
}

ListEl *listDetach( List *list, ListEl *el )
{
	/* Set forward pointers to skip over el. */
	if (el->list_prev == 0) 
		list->head = el->list_next; 
	else
		el->list_prev->list_next = el->list_next; 

	/* Set reverse pointers to skip over el. */
	if (el->list_next == 0) 
		list->tail = el->list_prev; 
	else
		el->list_next->list_prev = el->list_prev; 

	/* Update List length and return element we detached. */
	list->listLen--;
	return el;
}

