/*
 * Copyright 2010-2012 Adrian Thurston <thurston@colm.net>
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

#include <string.h>
#include <stdlib.h>

#include <colm/rtvector.h>
#include <colm/pdarun.h>

void init_rt_code_vect( struct rt_code_vect *vect )
{
	vect->data = 0;
	vect->tab_len = 0;
	vect->alloc_len = 0;
}

static long new_size_up( long existing, long needed )
{ 
	return needed > existing ? (needed<<1) : existing;
}

static long new_size_down( long existing, long needed )
{
	return needed < (existing>>2) ? (needed<<1) : existing;
}

/* Up resize the data for len elements using Resize::upResize to tell us the
 * new tabLen. Reads and writes allocLen. Does not read or write tabLen. */
static void up_resize( struct rt_code_vect *vect, long len )
{
	/* Ask the resizer what the new tabLen will be. */
	long new_len = new_size_up(vect->alloc_len, len);

	/* Did the data grow? */
	if ( new_len > vect->alloc_len ) {
		vect->alloc_len = new_len;
		if ( vect->data != 0 ) {
			/* Table exists already, resize it up. */
			vect->data = (code_t*) realloc( vect->data, sizeof(code_t) * new_len );
			//if ( vect->data == 0 )
			//	throw std::bad_alloc();
		}
		else {
			/* Create the data. */
			vect->data = (code_t*) malloc( sizeof(code_t) * new_len );
			//if ( vect->data == 0 )
			//	throw std::bad_alloc();
		}
	}
}

/* Down resize the data for len elements using Resize::downResize to determine
 * the new tabLen. Reads and writes allocLen. Does not read or write tabLen. */
static void down_resize( struct rt_code_vect *vect, long len)
{
	/* Ask the resizer what the new tabLen will be. */
	long new_len = new_size_down( vect->alloc_len, len );

	/* Did the data shrink? */
	if ( new_len < vect->alloc_len ) {
		vect->alloc_len = new_len;
		if ( new_len == 0 ) {
			/* Simply free the data. */
			free( vect->data );
			vect->data = 0;
		}
		else {
			/* Not shrinking to size zero, realloc it to the smaller size. */
			vect->data = (code_t*) realloc( vect->data, sizeof(code_t) * new_len );
			//if ( vect->data == 0 )
			//	throw std::bad_alloc();
		}
	}
}


void colm_rt_code_vect_empty( struct rt_code_vect *vect )
{
	if ( vect->data != 0 ) {
		/* Free the data space. */
		free( vect->data );
		vect->data = 0;
		vect->tab_len = vect->alloc_len = 0;
	}
}

void colm_rt_code_vect_replace( struct rt_code_vect *vect, long pos,
		const code_t *val, long len )
{
	long end_pos, i;
	//code_t *item;

	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = vect->tab_len + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	end_pos = pos + len;

	/* Make sure we have enough space. */
	if ( end_pos > vect->tab_len ) {
		up_resize( vect, end_pos );

		/* Delete any objects we need to delete. */
		//item = vect->data + pos;
		//for ( i = pos; i < vect->tabLen; i++, item++ )
		//	item->~code_t();
		
		/* We are extending the vector, set the new data length. */
		vect->tab_len = end_pos;
	}
	else {
		/* Delete any objects we need to delete. */
		//item = vect->data + pos;
		//for ( i = pos; i < endPos; i++, item++ )
		//	item->~code_t();
	}

	/* Copy data in using copy constructor. */
	code_t *dst = vect->data + pos;
	const code_t *src = val;
	for ( i = 0; i < len; i++, dst++, src++ )
		*dst = *src;
}

void colm_rt_code_vect_remove( struct rt_code_vect *vect, long pos, long len )
{
	long new_len, len_to_slide_over, end_pos;
	code_t *dst;//, *item;

	/* If we are given a negative position to remove at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = vect->tab_len + pos;

	/* The first position after the last item deleted. */
	end_pos = pos + len;

	/* The new data length. */
	new_len = vect->tab_len - len;

	/* The place in the data we are deleting at. */
	dst = vect->data + pos;

	/* Call Destructors. */
	//item = dst;
	//for ( long i = 0; i < len; i += 1, item += 1 )
	//	item->~code_t();
	
	/* Shift data over if necessary. */
	len_to_slide_over = vect->tab_len - end_pos;	
	if ( len > 0 && len_to_slide_over > 0 )
		memmove(dst, dst + len, sizeof(code_t)*len_to_slide_over);

	/* Shrink the data if necessary. */
	down_resize( vect, new_len );

	/* Set the new data length. */
	vect->tab_len = new_len;
}


