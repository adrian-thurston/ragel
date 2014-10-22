/*
 *  Copyright 2010-2012 Adrian Thurston <thurston@complang.org>
 */

#include <colm/rtvector.h>
#include <colm/pdarun.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>


void initRtCodeVect( RtCodeVect *vect )
{
	vect->data = 0;
	vect->tabLen = 0;
	vect->allocLen = 0;
}

static long newSizeUp( long existing, long needed )
{ 
	return needed > existing ? (needed<<1) : existing;
}

static long newSizeDown( long existing, long needed )
{
	return needed < (existing>>2) ? (needed<<1) : existing;
}

/* Up resize the data for len elements using Resize::upResize to tell us the
 * new tabLen. Reads and writes allocLen. Does not read or write tabLen. */
static void upResize( RtCodeVect *vect, long len )
{
	/* Ask the resizer what the new tabLen will be. */
	long newLen = newSizeUp(vect->allocLen, len);

	/* Did the data grow? */
	if ( newLen > vect->allocLen ) {
		vect->allocLen = newLen;
		if ( vect->data != 0 ) {
			/* Table exists already, resize it up. */
			vect->data = (Code*) realloc( vect->data, sizeof(Code) * newLen );
			//if ( vect->data == 0 )
			//	throw std::bad_alloc();
		}
		else {
			/* Create the data. */
			vect->data = (Code*) malloc( sizeof(Code) * newLen );
			//if ( vect->data == 0 )
			//	throw std::bad_alloc();
		}
	}
}

/* Down resize the data for len elements using Resize::downResize to determine
 * the new tabLen. Reads and writes allocLen. Does not read or write tabLen. */
static void downResize( RtCodeVect *vect, long len)
{
	/* Ask the resizer what the new tabLen will be. */
	long newLen = newSizeDown( vect->allocLen, len );

	/* Did the data shrink? */
	if ( newLen < vect->allocLen ) {
		vect->allocLen = newLen;
		if ( newLen == 0 ) {
			/* Simply free the data. */
			free( vect->data );
			vect->data = 0;
		}
		else {
			/* Not shrinking to size zero, realloc it to the smaller size. */
			vect->data = (Code*) realloc( vect->data, sizeof(Code) * newLen );
			//if ( vect->data == 0 )
			//	throw std::bad_alloc();
		}
	}
}


void rtCodeVectEmpty( RtCodeVect *vect )
{
	if ( vect->data != 0 ) {
		/* Free the data space. */
		free( vect->data );
		vect->data = 0;
		vect->tabLen = vect->allocLen = 0;
	}
}

void rtCodeVectReplace( RtCodeVect *vect, long pos, const Code *val, long len )
{
	long endPos, i;
	//Code *item;

	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = vect->tabLen + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	endPos = pos + len;

	/* Make sure we have enough space. */
	if ( endPos > vect->tabLen ) {
		upResize( vect, endPos );

		/* Delete any objects we need to delete. */
		//item = vect->data + pos;
		//for ( i = pos; i < vect->tabLen; i++, item++ )
		//	item->~Code();
		
		/* We are extending the vector, set the new data length. */
		vect->tabLen = endPos;
	}
	else {
		/* Delete any objects we need to delete. */
		//item = vect->data + pos;
		//for ( i = pos; i < endPos; i++, item++ )
		//	item->~Code();
	}

	/* Copy data in using copy constructor. */
	Code *dst = vect->data + pos;
	const Code *src = val;
	for ( i = 0; i < len; i++, dst++, src++ )
		*dst = *src;
}

void rtCodeVectRemove( RtCodeVect *vect, long pos, long len )
{
	long newLen, lenToSlideOver, endPos;
	Code *dst;//, *item;

	/* If we are given a negative position to remove at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = vect->tabLen + pos;

	/* The first position after the last item deleted. */
	endPos = pos + len;

	/* The new data length. */
	newLen = vect->tabLen - len;

	/* The place in the data we are deleting at. */
	dst = vect->data + pos;

	/* Call Destructors. */
	//item = dst;
	//for ( long i = 0; i < len; i += 1, item += 1 )
	//	item->~Code();
	
	/* Shift data over if necessary. */
	lenToSlideOver = vect->tabLen - endPos;	
	if ( len > 0 && lenToSlideOver > 0 )
		memmove(dst, dst + len, sizeof(Code)*lenToSlideOver);

	/* Shrink the data if necessary. */
	downResize( vect, newLen );

	/* Set the new data length. */
	vect->tabLen = newLen;
}


