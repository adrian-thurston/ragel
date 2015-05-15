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

#include <colm/pool.h>
#include <colm/pdarun.h>
#include <colm/bytecode.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

str_t *string_prefix( program_t *prg, str_t *str, long len )
{
	head_t *head = stringAllocFull( prg, str->value->data, len );
	return (str_t*)constructString( prg, head );
}

str_t *string_suffix( program_t *prg, str_t *str, long pos )
{
	long len = str->value->length - pos;
	head_t *head = stringAllocFull( prg, str->value->data + pos, len );
	return (str_t*)constructString( prg, head );
}

tree_t *constructString( program_t *prg, head_t *s )
{
	str_t *str = (str_t*) treeAllocate( prg );
	str->id = LEL_ID_STR;
	str->value = s;

	return (tree_t*)str;
}


/* 
 * In this system strings are not null terminated. Often strings come from a
 * parse, in which case the string is just a pointer into the the data string.
 * A block in a parsed stream can house many tokens and there is no room for
 * nulls.
 */

head_t *stringCopy( program_t *prg, head_t *head )
{
	head_t *result = 0;
	if ( head != 0 ) {
		if ( (char*)(head+1) == head->data )
			result = stringAllocFull( prg, head->data, head->length );
		else
			result = colm_string_alloc_pointer( prg, head->data, head->length );

		if ( head->location != 0 ) {
			result->location = locationAllocate( prg );
			result->location->name = head->location->name;
			result->location->line = head->location->line;
			result->location->column = head->location->column;
			result->location->byte = head->location->byte;
		}
	}
	return result;
}

void stringFree( program_t *prg, head_t *head )
{
	if ( head != 0 ) {
		if ( head->location != 0 )
			locationFree( prg, head->location );

		if ( (char*)(head+1) == head->data ) {
			/* Full string allocation. */
			free( head );
		}
		else {
			/* Just a string head. */
			headFree( prg, head );
		}
	}
}

const char *stringData( head_t *head )
{
	if ( head == 0 )
		return 0;
	return head->data;
}

long stringLength( head_t *head )
{
	if ( head == 0 )
		return 0;
	return head->length;
}

void stringShorten( head_t *head, long newlen )
{
	assert( newlen <= head->length );
	head->length = newlen;
}

head_t *initStrSpace( long length )
{
	/* Find the length and allocate the space for the shared string. */
	head_t *head = (head_t*) malloc( sizeof(head_t) + length );

	/* Init the header. */
	head->data = (char*)(head+1);
	head->length = length;
	head->location = 0;

	/* Save the pointer to the data. */
	return head;
}

/* Create from a c-style string. */
head_t *stringAllocFull( program_t *prg, const char *data, long length )
{
	/* Init space for the data. */
	head_t *head = initStrSpace( length );

	/* Copy in the data. */
	memcpy( (head+1), data, length );

	return head;
}

/* Create from a c-style string. */
head_t *colm_string_alloc_pointer( program_t *prg, const char *data, long length )
{
	/* Find the length and allocate the space for the shared string. */
	head_t *head = headAllocate( prg );

	/* Init the header. */
	head->data = data;
	head->length = length;

	return head;
}

head_t *concatStr( head_t *s1, head_t *s2 )
{
	long s1Len = s1->length;
	long s2Len = s2->length;

	/* Init space for the data. */
	head_t *head = initStrSpace( s1Len + s2Len );

	/* Copy in the data. */
	memcpy( (head+1), s1->data, s1Len );
	memcpy( (char*)(head+1) + s1Len, s2->data, s2Len );

	return head;
}

head_t *stringToUpper( head_t *s )
{
	/* Init space for the data. */
	long len = s->length;
	head_t *head = initStrSpace( len );

	/* Copy in the data. */
	const char *src = s->data;
	char *dst = (char*)(head+1);
	int i;
	for ( i = 0; i < len; i++ )
		*dst++ = toupper( *src++ );
		
	return head;
}

head_t *stringToLower( head_t *s )
{
	/* Init space for the data. */
	long len = s->length;
	head_t *head = initStrSpace( len );

	/* Copy in the data. */
	const char *src = s->data;
	char *dst = (char*)(head+1);
	int i;
	for ( i = 0; i < len; i++ )
		*dst++ = tolower( *src++ );
		
	return head;
}


/* Compare two strings. If identical returns 1, otherwise 0. */
word_t cmpString( head_t *s1, head_t *s2 )
{
	if ( s1->length < s2->length )
		return -1;
	else if ( s1->length > s2->length )
		return 1;
	else {
		char *d1 = (char*)(s1->data);
		char *d2 = (char*)(s2->data);
		return memcmp( d1, d2, s1->length );
	}
}

word_t strAtoi( head_t *str )
{
	/* FIXME: need to implement this by hand. There is no null terminator. */
	char *nulled = (char*)malloc( str->length + 1 );
	memcpy( nulled, str->data, str->length );
	nulled[str->length] = 0;
	int res = atoi( nulled );
	free( nulled );
	return res;
}

word_t strAtoo( head_t *str )
{
	/* FIXME: need to implement this by hand. There is no null terminator. */
	char *nulled = (char*)malloc( str->length + 1 );
	memcpy( nulled, str->data, str->length );
	nulled[str->length] = 0;
	int res = strtol( nulled, 0, 8 );
	free( nulled );
	return res;
}

head_t *intToStr( program_t *prg, word_t i )
{
	char data[20];
	sprintf( data, "%ld", i );
	return stringAllocFull( prg, data, strlen(data) );
}

word_t strUord16( head_t *head )
{
	uchar *data = (uchar*)(head->data);
	ulong res;
	res =   (ulong)data[1];
	res |= ((ulong)data[0]) << 8;
	return res;
}

word_t strUord8( head_t *head )
{
	uchar *data = (uchar*)(head->data);
	ulong res = (ulong)data[0];
	return res;
}

head_t *makeLiteral( program_t *prg, long offset )
{
	return colm_string_alloc_pointer( prg,
			prg->rtd->litdata[offset],
			prg->rtd->litlen[offset] );
}

head_t *stringSprintf( program_t *prg, str_t *format, long integer )
{
	head_t *formatHead = format->value;
	long written = snprintf( 0, 0, stringData(formatHead), integer );
	head_t *head = initStrSpace( written+1 );
	written = snprintf( (char*)head->data, written+1, stringData(formatHead), integer );
	head->length -= 1;
	return head;
}
