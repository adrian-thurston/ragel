/*
 * Copyright 2007-2012 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <colm/pool.h>
#include <colm/pdarun.h>
#include <colm/bytecode.h>

str_t *string_prefix( program_t *prg, str_t *str, long len )
{
	head_t *head = string_alloc_full( prg, str->value->data, len );
	return (str_t*)construct_string( prg, head );
}

str_t *string_suffix( program_t *prg, str_t *str, long pos )
{
	long len = str->value->length - pos;
	head_t *head = string_alloc_full( prg, str->value->data + pos, len );
	return (str_t*)construct_string( prg, head );
}

tree_t *construct_string( program_t *prg, head_t *s )
{
	str_t *str = (str_t*) tree_allocate( prg );
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

head_t *string_copy( program_t *prg, head_t *head )
{
	head_t *result = 0;
	if ( head != 0 ) {
		if ( (char*)(head+1) == head->data )
			result = string_alloc_full( prg, head->data, head->length );
		else
			result = colm_string_alloc_pointer( prg, head->data, head->length );

		if ( head->location != 0 ) {
			result->location = location_allocate( prg );
			result->location->name = head->location->name;
			result->location->line = head->location->line;
			result->location->column = head->location->column;
			result->location->byte = head->location->byte;
		}
	}
	return result;
}

void string_free( program_t *prg, head_t *head )
{
	if ( head != 0 ) {
		if ( head->location != 0 )
			location_free( prg, head->location );

		if ( (char*)(head+1) == head->data ) {
			/* Full string allocation. */
			free( head );
		}
		else {
			/* Just a string head. */
			head_free( prg, head );
		}
	}
}

const char *string_data( head_t *head )
{
	if ( head == 0 )
		return 0;
	return head->data;
}

long string_length( head_t *head )
{
	if ( head == 0 )
		return 0;
	return head->length;
}

void string_shorten( head_t *head, long newlen )
{
	assert( newlen <= head->length );
	head->length = newlen;
}

head_t *init_str_space( long length )
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
head_t *string_alloc_full( program_t *prg, const char *data, long length )
{
	/* Init space for the data. */
	head_t *head = init_str_space( length );

	/* Copy in the data. */
	memcpy( (head+1), data, length );

	return head;
}

/* Create from a c-style string. */
head_t *colm_string_alloc_pointer( program_t *prg, const char *data, long length )
{
	/* Find the length and allocate the space for the shared string. */
	head_t *head = head_allocate( prg );

	/* Init the header. */
	head->data = data;
	head->length = length;

	return head;
}

head_t *concat_str( head_t *s1, head_t *s2 )
{
	long s1Len = s1->length;
	long s2Len = s2->length;

	/* Init space for the data. */
	head_t *head = init_str_space( s1Len + s2Len );

	/* Copy in the data. */
	memcpy( (head+1), s1->data, s1Len );
	memcpy( (char*)(head+1) + s1Len, s2->data, s2Len );

	return head;
}

head_t *string_to_upper( head_t *s )
{
	/* Init space for the data. */
	long len = s->length;
	head_t *head = init_str_space( len );

	/* Copy in the data. */
	const char *src = s->data;
	char *dst = (char*)(head+1);
	int i;
	for ( i = 0; i < len; i++ )
		*dst++ = toupper( *src++ );
		
	return head;
}

head_t *string_to_lower( head_t *s )
{
	/* Init space for the data. */
	long len = s->length;
	head_t *head = init_str_space( len );

	/* Copy in the data. */
	const char *src = s->data;
	char *dst = (char*)(head+1);
	int i;
	for ( i = 0; i < len; i++ )
		*dst++ = tolower( *src++ );
		
	return head;
}


/* Compare two strings. If identical returns 1, otherwise 0. */
word_t cmp_string( head_t *s1, head_t *s2 )
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

word_t str_atoi( head_t *str )
{
	/* FIXME: need to implement this by hand. There is no null terminator. */
	char *nulled = (char*)malloc( str->length + 1 );
	memcpy( nulled, str->data, str->length );
	nulled[str->length] = 0;
	int res = atoi( nulled );
	free( nulled );
	return res;
}

word_t str_atoo( head_t *str )
{
	/* FIXME: need to implement this by hand. There is no null terminator. */
	char *nulled = (char*)malloc( str->length + 1 );
	memcpy( nulled, str->data, str->length );
	nulled[str->length] = 0;
	int res = strtol( nulled, 0, 8 );
	free( nulled );
	return res;
}

head_t *int_to_str( program_t *prg, word_t i )
{
	char data[20];
	sprintf( data, "%ld", i );
	return string_alloc_full( prg, data, strlen(data) );
}

word_t str_uord16( head_t *head )
{
	uchar *data = (uchar*)(head->data);
	ulong res;
	res =   (ulong)data[1];
	res |= ((ulong)data[0]) << 8;
	return res;
}

word_t str_uord8( head_t *head )
{
	uchar *data = (uchar*)(head->data);
	ulong res = (ulong)data[0];
	return res;
}

head_t *make_literal( program_t *prg, long offset )
{
	return colm_string_alloc_pointer( prg,
			prg->rtd->litdata[offset],
			prg->rtd->litlen[offset] );
}

head_t *string_sprintf( program_t *prg, str_t *format, long integer )
{
	head_t *format_head = format->value;
	long written = snprintf( 0, 0, string_data(format_head), integer );
	head_t *head = init_str_space( written+1 );
	written = snprintf( (char*)head->data, written+1, string_data(format_head), integer );
	head->length -= 1;
	return head;
}
