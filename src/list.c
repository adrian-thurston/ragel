/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
 */

#include <colm/pdarun.h>
#include <colm/program.h>
#include <colm/struct.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static void colm_list_add_after( List *list, ListEl *prev_el, ListEl *new_el );
static void colm_list_add_before( List *list, ListEl *next_el, ListEl *new_el);

void colm_list_prepend( List *list, ListEl *newEl )
{
	colm_list_add_before( list, list->head, newEl );
}

void colm_list_append( List *list, ListEl *newEl )
{
	colm_list_add_after( list, list->tail, newEl );
}

ListEl *colm_list_detach( List *list, ListEl *el );

ListEl *colm_list_detach_head( List *list )
{
	return colm_list_detach( list, list->head );
}

ListEl *colm_list_detach_tail( List *list )
{
	return colm_list_detach( list, list->tail );
}

long colm_list_length( List *list )
{
	return list->listLen;
}

static void colm_list_add_after( List *list, ListEl *prev_el, ListEl *new_el )
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

static void colm_list_add_before( List *list, ListEl *next_el, ListEl *new_el)
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

ListEl *colm_list_detach( List *list, ListEl *el )
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

void colm_list_destroy( struct colm_program *prg, Tree **sp, struct colm_struct *s )
{
}

List *colm_list_new( struct colm_program *prg )
{
	size_t memsize = sizeof(struct colm_list);
	struct colm_list *list = (struct colm_list*) malloc( memsize );
	memset( list, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)list );
	list->id = STRUCT_INBUILT_ID;
	list->destructor = &colm_list_destroy;
	return list;
}

struct colm_struct *colm_list_get( struct colm_program *prg,
		List *list, Word genId, Word field )
{
	GenericInfo *gi = &prg->rtd->genericInfo[genId];
	ListEl *result = 0;
	switch ( field ) {
		case 0: 
			result = list->head;
			break;
		case 1: 
			result = list->tail;
			break;
		default:
			assert( 0 );
			break;
	}

	struct colm_struct *s = result != 0 ?
			colm_struct_container( result, gi->elOffset ) : 0;
	return s;
}

struct colm_struct *colm_list_el_get( struct colm_program *prg,
		ListEl *listEl, Word genId, Word field )
{
	GenericInfo *gi = &prg->rtd->genericInfo[genId];
	ListEl *result = 0;
	switch ( field ) {
		case 0: 
			result = listEl->list_prev;
			break;
		case 1: 
			result = listEl->list_next;
			break;
		default:
			assert( 0 );
			break;
	}

	struct colm_struct *s = result != 0 ?
			colm_struct_container( result, gi->elOffset ) : 0;
	return s;
}
