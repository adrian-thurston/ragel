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

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char Code;
typedef unsigned long Word;
typedef unsigned long Half;

typedef struct _RtCodeVect
{
	Code *data;
	long tabLen;
	long allocLen;

	/* FIXME: leak when freed. */
} RtCodeVect;

void rtCodeVectReplace( RtCodeVect *vect, long pos, const Code *val, long len );
void rtCodeVectEmpty( RtCodeVect *vect );
void rtCodeVectRemove( RtCodeVect *vect, long pos, long len );

void initRtCodeVect( RtCodeVect *codeVect );

//inline static void remove( RtCodeVect *vect, long pos );
inline static void append( RtCodeVect *vect, const Code val );
inline static void append2( RtCodeVect *vect, const Code *val, long len );
inline static void appendHalf( RtCodeVect *vect, Half half );
inline static void appendWord( RtCodeVect *vect, Word word );

inline static void append2( RtCodeVect *vect, const Code *val, long len )
{
	rtCodeVectReplace( vect, vect->tabLen, val, len );
}

inline static void append( RtCodeVect *vect, const Code val )
{
	rtCodeVectReplace( vect, vect->tabLen, &val, 1 );
}

inline static void appendHalf( RtCodeVect *vect, Half half )
{
	/* not optimal. */
	append( vect, half & 0xff );
	append( vect, (half>>8) & 0xff );
}

inline static void appendWord( RtCodeVect *vect, Word word )
{
	/* not optimal. */
	append( vect, word & 0xff );
	append( vect, (word>>8) & 0xff );
	append( vect, (word>>16) & 0xff );
	append( vect, (word>>24) & 0xff );
	#if SIZEOF_LONG == 8
	append( vect, (word>>32) & 0xff );
	append( vect, (word>>40) & 0xff );
	append( vect, (word>>48) & 0xff );
	append( vect, (word>>56) & 0xff );
	#endif
}

#ifdef __cplusplus
}
#endif

#endif 

