/*
 *  Copyright 2002, 2006, 2009 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Aapl.
 *
 *  Aapl is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 2.1 of the License, or (at your option)
 *  any later version.
 *
 *  Aapl is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Aapl; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _RT_VECTOR_H
#define _RT_VECTOR_H

#include <new>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

template < class T > class RtVector
{
public:
	static inline long upResize( long existing, long needed )
	{ 
		return needed > existing ? (needed<<1) : existing;
	}

	static inline long downResize( long existing, long needed )
	{
		return needed < (existing>>2) ? (needed<<1) : existing;
	}

	RtVector() :
		data(0),
		tabLen(0),
		allocLen(0)
	{ }

	long length() const
		{ return tabLen; }

	T *data;
	long tabLen;
	long allocLen;


	/* Free all mem used by the vector. */
	~RtVector() { empty(); }

	/* Delete all items. */
	void empty();

	/* Stack operations. */
	void push( const T &t ) { append( t ); }
	void pop() { remove( tabLen - 1 ); }
	T &top() { return data[tabLen - 1]; }

	void remove(long pos)                 { remove(pos, 1); }
	void remove(long pos, long len);

	void append(const T &val)                { replace(tabLen, &val, 1); }
	void append(const T *val, long len)       { replace(tabLen, val, len); }

protected:
	void upResize(long len);
	void downResize(long len);

private:
	void replace(long pos, const T *val, long len);
};


/* Up resize the data for len elements using Resize::upResize to tell us the
 * new tabLen. Reads and writes allocLen. Does not read or write tabLen. */
template<class T> void RtVector<T>::
		upResize(long len)
{
	/* Ask the resizer what the new tabLen will be. */
	long newLen = upResize(allocLen, len);

	/* Did the data grow? */
	if ( newLen > allocLen ) {
		allocLen = newLen;
		if ( data != 0 ) {
			/* Table exists already, resize it up. */
			data = (T*) realloc( data, sizeof(T) * newLen );
			if ( data == 0 )
				throw std::bad_alloc();
		}
		else {
			/* Create the data. */
			data = (T*) malloc( sizeof(T) * newLen );
			if ( data == 0 )
				throw std::bad_alloc();
		}
	}
}

/* Down resize the data for len elements using Resize::downResize to determine
 * the new tabLen. Reads and writes allocLen. Does not read or write tabLen. */
template<class T> void RtVector<T>::
		downResize(long len)
{
	/* Ask the resizer what the new tabLen will be. */
	long newLen = downResize( allocLen, len );

	/* Did the data shrink? */
	if ( newLen < allocLen ) {
		allocLen = newLen;
		if ( newLen == 0 ) {
			/* Simply free the data. */
			free( data );
			data = 0;
		}
		else {
			/* Not shrinking to size zero, realloc it to the smaller size. */
			data = (T*) realloc( data, sizeof(T) * newLen );
			if ( data == 0 )
				throw std::bad_alloc();
		}
	}
}


template<class T> void RtVector<T>::
		empty()
{
	if ( data != 0 ) {
		/* Call All destructors. */
		T *pos = data;
		for ( long i = 0; i < tabLen; pos++, i++ )
			pos->~T();

		/* Free the data space. */
		free( data );
		data = 0;
		tabLen = allocLen = 0;
	}
}

template<class T> void RtVector<T>::
		replace(long pos, const T *val, long len)
{
	long endPos, i;
	T *item;

	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = tabLen + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	endPos = pos + len;

	/* Make sure we have enough space. */
	if ( endPos > tabLen ) {
		upResize( endPos );

		/* Delete any objects we need to delete. */
		item = data + pos;
		for ( i = pos; i < tabLen; i++, item++ )
			item->~T();
		
		/* We are extending the vector, set the new data length. */
		tabLen = endPos;
	}
	else {
		/* Delete any objects we need to delete. */
		item = data + pos;
		for ( i = pos; i < endPos; i++, item++ )
			item->~T();
	}

	/* Copy data in using copy constructor. */
	T *dst = data + pos;
	const T *src = val;
	for ( i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);
}

template<class T> void RtVector<T>::
		remove(long pos, long len)
{
	long newLen, lenToSlideOver, endPos;
	T *dst, *item;

	/* If we are given a negative position to remove at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = tabLen + pos;

	/* The first position after the last item deleted. */
	endPos = pos + len;

	/* The new data length. */
	newLen = tabLen - len;

	/* The place in the data we are deleting at. */
	dst = data + pos;

	/* Call Destructors. */
	item = dst;
	for ( long i = 0; i < len; i += 1, item += 1 )
		item->~T();
	
	/* Shift data over if necessary. */
	lenToSlideOver = tabLen - endPos;	
	if ( len > 0 && lenToSlideOver > 0 )
		memmove(dst, dst + len, sizeof(T)*lenToSlideOver);

	/* Shrink the data if necessary. */
	downResize( newLen );

	/* Set the new data length. */
	tabLen = newLen;
}

#endif 

