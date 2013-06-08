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

/* 
 * In this system strings are not null terminated. Often strings come from a
 * parse, in which case the string is just a pointer into the the data string.
 * A block in a parsed stream can house many tokens and there is no room for
 * nulls.
 */

Head *stringCopy( Program *prg, Head *head )
{
	Head *result = 0;
	if ( head != 0 ) {
		if ( (char*)(head+1) == head->data )
			result = stringAllocFull( prg, head->data, head->length );
		else
			result = stringAllocPointer( prg, head->data, head->length );

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

void stringFree( Program *prg, Head *head )
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

const char *stringData( Head *head )
{
	if ( head == 0 )
		return 0;
	return head->data;
}

long stringLength( Head *head )
{
	if ( head == 0 )
		return 0;
	return head->length;
}

void stringShorten( Head *head, long newlen )
{
	assert( newlen <= head->length );
	head->length = newlen;
}

Head *initStrSpace( long length )
{
	/* Find the length and allocate the space for the shared string. */
	Head *head = (Head*) malloc( sizeof(Head) + length );

	/* Init the header. */
	head->data = (char*)(head+1);
	head->length = length;
	head->location = 0;

	/* Save the pointer to the data. */
	return head;
}

/* Create from a c-style string. */
Head *stringAllocFull( Program *prg, const char *data, long length )
{
	/* Init space for the data. */
	Head *head = initStrSpace( length );

	/* Copy in the data. */
	memcpy( (head+1), data, length );

	return head;
}

/* Create from a c-style string. */
Head *stringAllocPointer( Program *prg, const char *data, long length )
{
	/* Find the length and allocate the space for the shared string. */
	Head *head = headAllocate( prg );

	/* Init the header. */
	head->data = data;
	head->length = length;

	return head;
}

Head *concatStr( Head *s1, Head *s2 )
{
	long s1Len = s1->length;
	long s2Len = s2->length;

	/* Init space for the data. */
	Head *head = initStrSpace( s1Len + s2Len );

	/* Copy in the data. */
	memcpy( (head+1), s1->data, s1Len );
	memcpy( (char*)(head+1) + s1Len, s2->data, s2Len );

	return head;
}

Head *stringToUpper( Head *s )
{
	/* Init space for the data. */
	long len = s->length;
	Head *head = initStrSpace( len );

	/* Copy in the data. */
	const char *src = s->data;
	char *dst = (char*)(head+1);
	int i;
	for ( i = 0; i < len; i++ )
		*dst++ = toupper( *src++ );
		
	return head;
}

Head *stringToLower( Head *s )
{
	/* Init space for the data. */
	long len = s->length;
	Head *head = initStrSpace( len );

	/* Copy in the data. */
	const char *src = s->data;
	char *dst = (char*)(head+1);
	int i;
	for ( i = 0; i < len; i++ )
		*dst++ = tolower( *src++ );
		
	return head;
}


/* Compare two strings. If identical returns 1, otherwise 0. */
Word cmpString( Head *s1, Head *s2 )
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

Word strAtoi( Head *str )
{
	/* FIXME: need to implement this by hand. There is no null terminator. */
	char *nulled = (char*)malloc( str->length + 1 );
	memcpy( nulled, str->data, str->length );
	nulled[str->length] = 0;
	int res = atoi( nulled );
	free( nulled );
	return res;
}

Head *intToStr( Program *prg, Word i )
{
	char data[20];
	sprintf( data, "%ld", i );
	return stringAllocFull( prg, data, strlen(data) );
}

Word strUord16( Head *head )
{
	uchar *data = (uchar*)(head->data);
	ulong res;
	res =   (ulong)data[1];
	res |= ((ulong)data[0]) << 8;
	return res;
}

Word strUord8( Head *head )
{
	uchar *data = (uchar*)(head->data);
	ulong res = (ulong)data[0];
	return res;
}

Head *makeLiteral( Program *prg, long offset )
{
	return stringAllocPointer( prg,
			prg->rtd->litdata[offset],
			prg->rtd->litlen[offset] );
}

Head *stringSprintf( Program *prg, Str *format, Int *integer )
{
	Head *formatHead = format->value;
	long written = snprintf( 0, 0, stringData(formatHead), integer->value );
	Head *head = initStrSpace( written+1 );
	written = snprintf( (char*)head->data, written+1, stringData(formatHead), integer->value );
	head->length -= 1;
	return head;
}
