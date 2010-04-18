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

typedef unsigned char Code;
typedef unsigned long Word;
typedef unsigned long Half;

struct RtCodeVect;

void rtCodeVectReplace( RtCodeVect *vect, long pos, const Code *val, long len );
void rtCodeVectEmpty( RtCodeVect *vect );
void rtCodeVectRemove( RtCodeVect *vect, long pos, long len );

void initRtCodeVect( RtCodeVect *codeVect );

struct RtCodeVect
{
	long length() const
		{ return tabLen; }

	Code *data;
	long tabLen;
	long allocLen;

	/* Free all mem used by the vector. */
	~RtCodeVect() { rtCodeVectEmpty( this ); }

	/* Stack operations. */
	void push( const Code &t ) { append( t ); }
	void pop() { remove( tabLen - 1 ); }
	Code &top() { return data[tabLen - 1]; }

	void remove(long pos)                 { rtCodeVectRemove( this, pos, 1 ); }

	void append(const Code &val)                { rtCodeVectReplace( this, tabLen, &val, 1 ); }
	void append(const Code *val, long len)       { rtCodeVectReplace( this, tabLen, val, len ); }

	void appendHalf( Half half )
	{
		/* not optimal. */
		append( half & 0xff );
		append( (half>>8) & 0xff );
	}
	
	void appendWord( Word word )
	{
		/* not optimal. */
		append( word & 0xff );
		append( (word>>8) & 0xff );
		append( (word>>16) & 0xff );
		append( (word>>24) & 0xff );
		#if SIZEOF_LONG == 8
		append( (word>>32) & 0xff );
		append( (word>>40) & 0xff );
		append( (word>>48) & 0xff );
		append( (word>>56) & 0xff );
		#endif
	}
};

#endif 

