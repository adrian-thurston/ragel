/*
 *  Copyright 2002 Adrian Thurston <thurston@complang.org>
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

#ifndef _AAPL_ASTRING_H
#define _AAPL_ASTRING_H

#include <new>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <assert.h>

#include "tree.h"

struct colm_data;

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

#ifdef AAPL_DOCUMENTATION

/**
 * \defgroup astring String
 * \brief Implicitly shared copy-on-write string.
 * 
 * @{
 */

/**
 * \class String
 * \brief Implicitly shared copy-on-write string.
 */

/*@}*/

class String
{
public:
	/**
	 * \brief Create a null string. Data points to NULL.
	 */
	String();

	/**
	 * \brief Construct a string from a c-style string.
	 *
	 * A new buffer is allocated for the c string. Initially, this string will
	 * be the only String class referencing the data.
	 */
	String( const char *s );

	/**
	 * \brief Construct a string from a c-style string of specific length.
	 *
	 * A new buffer is allocated for the c string. Initially, this string will
	 * be the only String class referencing the data.
	 */
	String( const char *s, long len );

	/**
	 * \brief Construct a string from another String.
	 *
	 * A refernce to the buffer allocated for s is taken. A new buffer is
	 * not allocated.
	 */
	String( const String &s );

	/**
	 * \brief Construct a string using snprintf.
	 *
	 * Requires a maximum length for the resulting string. If the formatting
	 * (not including trailing null) requires more space than maxLen, the
	 * result will be truncated to maxLen long. Only the length actually
	 * written will be used by the new string. This string will be the only
	 * String class referencing the data.
	 */
	String( long maxLen, const char *format, ... )

	/**
	 * \brief Clean up the string.
	 *
	 * If the string is not null, the referenced data is detached. If no other
	 * string refernces the detached data, it is deleted.
	 */
	~String();

	/**
	 * \brief Set the string from a c-style string.
	 *
	 * If this string is not null, the current buffer is dereferenced and
	 * possibly deleted. A new buffer is allocated (or possibly the old buffer
	 * reused) for the string. Initially, this string will be the only String
	 * class referencing the data.
	 *
	 * If s is null, then this string becomes a null ptr.
	 *
	 * \returns A reference to this.
	 */
	String &operator=( const char *s );

	/**
	 * \brief Set the string from a c-style of specific length.
	 *
	 * If this string is not null, the current buffer is dereferenced and
	 * possibly deleted. A new buffer is allocated (or possibly the old buffer
	 * reused) for the string. Initially, this string will be the only String
	 * class referencing the data.
	 *
	 * If s is null, then this string becomes a null ptr.
	 *
	 * \returns A reference to this.
	 */
	void setAs( const char *s, long len );

	/**
	 * \brief Set the string from a single char.
	 *
	 * The current buffer is dereferenced and possibly deleted. A new buffer
	 * is allocated (or possibly the old buffer reused) for the string.
	 * Initially, this string will be the only String class referencing the
	 * data.
	 *
	 * If s is null, then this string becomes a null ptr.
	 *
	 * \returns A reference to this.
	 */
	String &operator=( const char c );


	/**
	 * \brief Set the string from another String.
	 *
	 * If this string is not null, the current buffer is dereferenced and
	 * possibly deleted. A reference to the buffer allocated for s is taken.
	 * A new buffer is not allocated.
	 *
	 * If s is null, then this string becomes a null ptr.
	 *
	 * \returns a reference to this.
	 */
	String &operator=( const String &s );

	/**
	 * \brief Append a c string to the end of this string.
	 *
	 * If this string shares its allocation with another, a copy is first
	 * taken. The buffer for this string is grown and s is appended to the
	 * end.
	 * 
	 * If s is null nothing happens.
	 *
	 * \returns a reference to this.
	 */
	String &operator+=( const char *s );

	/**
	 * \brief Append a c string of specific length to the end of this string.
	 *
	 * If this string shares its allocation with another, a copy is first
	 * taken. The buffer for this string is grown and s is appended to the
	 * end.
	 * 
	 * If s is null nothing happens.
	 *
	 * \returns a reference to this.
	 */
	void append( const char *s, long len );

	/**
	 * \brief Append a single char to the end of this string.
	 *
	 * If this string shares its allocation with another, a copy is first
	 * taken. The buffer for this string is grown and s is appended to the
	 * end.
	 * 
	 * \returns a reference to this.
	 */
	String &operator+=( const char c );

	/**
	 * \brief Append a String to the end of this string.
	 *
	 * If this string shares its allocation with another, a copy is first
	 * taken. The buffer for this string is grown and the data of s is
	 * appeneded to the end.
	 *
	 * If s is null nothing happens.
	 *
	 * returns a reference to this.
	 */
	String &operator+=( const String &s );

	/**
	 * \brief Cast to a char star.
	 *
	 * \returns the string data. A null string returns 0.
	 */
	operator char*() const;

	/**
	 * \brief Get a pointer to the data.
	 *
	 * \returns the string Data
	 */
	char *get() const;

	/**
	 * \brief Get the length of the string
	 *
	 * If the string is null, then undefined behaviour results.
	 *
	 * \returns the length of the string.
	 */
	long length() const;

	/**
	 * \brief Pointer to the data.
	 *
	 * Publically accessible pointer to the data. Immediately in front of the
	 * string data block is the string header which stores the refcount and
	 * length. Consequently, care should be taken if modifying this pointer.
	 */
	char *data;
};

/**
 * \relates String
 * \brief Concatenate a c-style string and a String.
 *
 * \returns The concatenation of the two strings in a String.
 */
String operator+( const String &s1, const char *s2 );

/**
 * \relates String
 * \brief Concatenate a String and a c-style string.
 *
 * \returns The concatenation of the two strings in a String.
 */
String operator+( const char *s1, const String &s2 );

/**
 * \relates String
 * \brief Concatenate two String classes.
 *
 * \returns The concatenation of the two strings in a String.
 */
String operator+( const String &s1, const String &s2 );

#endif 

template<class T> class StrTmpl
{
public:
	class Fresh {};

	/* Header located just before string data. Keeps the length and a refcount on
	 * the data. */
	struct Head
	{
		long refCount;
		long length;
	};

	/**
	 * \brief Create a null string.
	 */
	StrTmpl() : data(0) { }

	/* Clean up the string. */
	~StrTmpl();

	/* Construct a string from a c-style string. */
	StrTmpl( const char *s );

	/* Construct a string from a c-style string of specific len. */
	StrTmpl( const char *s, long len );

	/* Allocate len spaces. */
	StrTmpl( const Fresh &, long len );

	/* Construct a string from another StrTmpl.  */
	StrTmpl( const StrTmpl &s );

	/* Construct a string from with, sprintf. */
	StrTmpl( long lenGuess, const char *format, ... );

	/* Construct a string from with, sprintf. */
	StrTmpl( const colm_data *cd );

	/* Set the string from a c-style string. */
	StrTmpl &operator=( const char *s );

	/* Set the string from a c-style string of specific len. */
	void setAs( const char *s, long len );

	/* Allocate len spaces. */
	void setAs( const Fresh &, long len );

	void chop( long len );

	/* Construct a string from with, sprintf. */
	void setAs( long lenGuess, const char *format, ... );

	/* Set the string from a single char. */
	StrTmpl &operator=( const char c );

	/* Set the string from another StrTmpl. */
	StrTmpl &operator=( const StrTmpl &s );

	/* Append a c string to the end of this string. */
	StrTmpl &operator+=( const char *s );

	/* Append a c string to the end of this string of specifi len. */
	void append( const char *s, long len );

	/* Append a single char to the end of this string. */
	StrTmpl &operator+=( const char c );

	/* Append an StrTmpl to the end of this string. */
	StrTmpl &operator+=( const StrTmpl &s );

	/* Cast to a char star. */
	operator char*() const { return data; }

	/* Get a pointer to the data. */
	char *get() const { return data; }

	/* Return the length of the string. Must check for null data pointer. */
	long length() const { return data ? (((Head*)data)-1)->length : 0; }

	/**
	 * \brief Pointer to the data.
	 */
	char *data;

protected:
	/* Make space for a string of length len to be appended. */
	char *appendSpace( long len );
	void initSpace( long length );
	void setSpace( long length );

 	template <class FT> friend StrTmpl<FT> operator+( 
			const StrTmpl<FT> &s1, const char *s2 );
	template <class FT> friend StrTmpl<FT> operator+( 
			const char *s1, const StrTmpl<FT> &s2 );
	template <class FT> friend StrTmpl<FT> operator+( 
			const StrTmpl<FT> &s1, const StrTmpl<FT> &s2 );

private:
	/* A dummy struct solely to make a constructor that will never be
	 * ambiguous with the public constructors. */
	struct DisAmbig { };
	StrTmpl( char *data, const DisAmbig & ) : data(data) { }
};

/* Free all mem used by the string. */
template<class T> StrTmpl<T>::~StrTmpl()
{
	if ( data != 0 ) {
		/* If we are the only ones referencing the string, then delete it. */
		Head *head = ((Head*) data) - 1;
		head->refCount -= 1;
		if ( head->refCount == 0 )
			free( head );
	}
}

/* Create from a c-style string. */
template<class T> StrTmpl<T>::StrTmpl( const char *s )
{
	if ( s == 0 )
		data = 0;
	else {
		/* Find the length and allocate the space for the shared string. */
		long length = strlen( s );

		/* Init space for the data. */
		initSpace( length );

		/* Copy in the data. */
		memcpy( data, s, length+1 );
	}
}

/* Create from a c-style string. */
template<class T> StrTmpl<T>::StrTmpl( const char *s, long length )
{
	if ( s == 0 )
		data = 0;
	else {
		/* Init space for the data. */
		initSpace( length );

		/* Copy in the data. */
		memcpy( data, s, length );
		data[length] = 0;
	}
}

/* Create from a c-style string. */
template<class T> StrTmpl<T>::StrTmpl( const Fresh &, long length )
{
	/* Init space for the data. */
	initSpace( length );
	data[length] = 0;
}

/* Create from another string class. */
template<class T> StrTmpl<T>::StrTmpl( const StrTmpl &s )
{
	if ( s.data == 0 )
		data = 0;
	else {
		/* Take a reference to the string. */
		Head *strHead = ((Head*)s.data) - 1;
		strHead->refCount += 1;
		data = (char*) (strHead+1);
	}
}

/* Construct a string from with, sprintf. */
template<class T> StrTmpl<T>::StrTmpl( long lenGuess, const char *format, ... )
{
	/* Set the string for len. */
	initSpace( lenGuess );

	va_list args;

	va_start( args, format );
	long written = vsnprintf( data, lenGuess+1, format, args );
	va_end( args );

	if ( written > lenGuess ) {
		setSpace( written );
		va_start( args, format );
		written = vsnprintf( data, written+1, format, args );
		va_end( args );
	}
	chop( written );

	va_end( args );
}

/* Create from another string class. */
template<class T> StrTmpl<T>::StrTmpl( const colm_data *cd )
{
	if ( cd->data == 0 )
		data = 0;
	else {
		/* Init space for the data. */
		initSpace( cd->length );

		/* Copy in the data. */
		memcpy( data, cd->data, cd->length );
		data[cd->length] = 0;
	}
}



/* Construct a string from with, sprintf. */
template<class T> void StrTmpl<T>::setAs( long lenGuess, const char *format, ... )
{
	/* Set the string for len. */
	setSpace( lenGuess );

	va_list args;

	/* Write to the temporary buffer. */
	va_start( args, format );

	long written = vsnprintf( data, lenGuess+1, format, args );
	if ( written > lenGuess ) {
		setSpace( written );
		written = vsnprintf( data, written+1, format, args );
	}
	chop( written );

	va_end( args );
}

template<class T> void StrTmpl<T>::initSpace( long length )
{
	/* Find the length and allocate the space for the shared string. */
	Head *head = (Head*) malloc( sizeof(Head) + length+1 );
	if ( head == 0 )
		throw std::bad_alloc();

	/* Init the header. */
	head->refCount = 1;
	head->length = length;

	/* Save the pointer to the data. */
	data = (char*) (head+1);
}


/* Set this string to be the c string exactly. The old string is discarded.
 * Returns a reference to this. */
template<class T> StrTmpl<T> &StrTmpl<T>::operator=( const char *s )
{
	if ( s == 0 ) {
		/* Just free the data, we are being set to null. */
		if ( data != 0 ) {
			Head *head = ((Head*)data) - 1;
			head->refCount -= 1;
			if ( head->refCount == 0 )
				free(head);
			data = 0;
		}
	}
	else {
		/* Find the length of the string we are setting. */
		long length = strlen( s );
		
		/* Set the string for len. */
		setSpace( length );

		/* Copy in the data. */
		memcpy( data, s, length+1 );
	}
	return *this;
}

/* Set this string to be the c string exactly. The old string is discarded.
 * Returns a reference to this. */
template<class T> void StrTmpl<T>::setAs( const char *s, long length )
{
	if ( s == 0 ) {
		/* Just free the data, we are being set to null. */
		if ( data != 0 ) {
			Head *head = ((Head*)data) - 1;
			head->refCount -= 1;
			if ( head->refCount == 0 )
				free(head);
			data = 0;
		}
	}
	else {
		/* Set the string for len. */
		setSpace( length );

		/* Copy in the data. */
		memcpy( data, s, length );
		data[length] = 0;
	}
}

template<class T> void StrTmpl<T>::chop( long length )
{
	Head *head = ((Head*)data) - 1;
	assert( head->refCount == 1 );
	assert( length <= head->length );
	head->length = length;
	data[length] = 0;
}

/* Set this string to be the c string exactly. The old string is discarded.
 * Returns a reference to this. */
template<class T> void StrTmpl<T>::setAs( const Fresh &, long length )
{
	setSpace( length );
	data[length] = 0;
}

/* Set this string to be the single char exactly. The old string is discarded.
 * Returns a reference to this. */
template<class T> StrTmpl<T> &StrTmpl<T>::operator=( const char c )
{
	/* Set to length 1. */
	setSpace( 1 );

	/* Copy in the data. */
	data[0] = c;
	data[1] = 0;

	/* Return ourselves. */
	return *this;
}

/* Set this string to be the StrTmpl s exactly. The old string is
 * discarded. */
template<class T> StrTmpl<T> &StrTmpl<T>::operator=( const StrTmpl &s )
{
	/* Detach from the existing string. */
	if ( data != 0 ) {
		Head *head = ((Head*)data) - 1;
		head->refCount -= 1;
		if ( head->refCount == 0 )
			free( head );
	}

	if ( s.data != 0 ) {
		/* Take a reference to the string. */
		Head *strHead = ((Head*)s.data) - 1;
		strHead->refCount += 1;
		data = (char*)(strHead+1);
	}
	else {
		/* Setting from a null string, just null our pointer. */
		data = 0;
	}
	return *this;
}

/* Prepare the string to be set to something else of the given length. */
template<class T> void StrTmpl<T>::setSpace( long length )
{
	/* Detach from the existing string. */
	Head *head = ((Head*)data) - 1;
	if ( data != 0 && --head->refCount == 0 ) {
		/* Resuse the space. */
		head = (Head*) realloc( head, sizeof(Head) + length+1 );
	}
	else {
		/* Need to make new space, there is no usable old space. */
		head = (Head*) malloc( sizeof(Head) + length+1 );
	}
	if ( head == 0 )
		throw std::bad_alloc();

	/* Init the header. */
	head->refCount = 1;
	head->length = length;

	/* Copy in the data and save the pointer to it. */
	data = (char*) (head+1);
}


/* Append a c-style string to the end of this string. Returns a reference to
 * this */
template<class T> StrTmpl<T> &StrTmpl<T>::operator+=( const char *s )
{
	/* Find the length of the string appended. */
	if ( s != 0 ) {
		/* Get the string length and make space on the end. */
		long addedLen = strlen( s );
		char *dest = appendSpace( addedLen );

		/* Copy the data in. Plus one for the null. */
		memcpy( dest, s, addedLen+1 );
	}
	return *this;
}

/* Append a c-style string of specific length to the end of this string.
 * Returns a reference to this */
template<class T> void StrTmpl<T>::append( const char *s, long length )
{
	/* Find the length of the string appended. */
	if ( s != 0 ) {
		/* Make space on the end. */
		char *dest = appendSpace( length );

		/* Copy the data in. Plus one for the null. */
		memcpy( dest, s, length );
		dest[length] = 0;
	}
}

/* Append a single char to the end of this string. Returns a reference to
 * this */
template<class T> StrTmpl<T> &StrTmpl<T>::operator+=( const char c )
{
	/* Grow on the end. */
	char *dst = appendSpace( 1 );

	/* Append a single charachter. */
	dst[0] = c;
	dst[1] = 0;
	return *this;
}


/* Append an StrTmpl string to the end of this string. Returns a reference
 * to this */
template<class T> StrTmpl<T> &StrTmpl<T>::operator+=( const StrTmpl &s )
{
	/* Find the length of the string appended. */
	if ( s.data != 0 ) {
		/* Find the length to append. */
		long addedLen = (((Head*)s.data) - 1)->length;

		/* Make space on the end to put the string. */
		char *dest = appendSpace( addedLen );

		/* Append the data, add one for the null. */
		memcpy( dest, s.data, addedLen+1 );
	}
	return *this;
}

/* Make space for a string of length len to be appended. */
template<class T> char *StrTmpl<T>::appendSpace( long len )
{
	/* Find the length of this and the string appended. */
	Head *head = (((Head*)data) - 1);
	long thisLen = head->length;

	if ( head->refCount == 1 ) {
		/* No other string is using the space, grow this space. */
		head = (Head*) realloc( head, 
				sizeof(Head) + thisLen + len + 1 );
		if ( head == 0 )
			throw std::bad_alloc();
		data = (char*) (head+1);

		/* Adjust the length. */
		head->length += len;
	}
	else {
		/* Another string is using this space, make new space. */
		head->refCount -= 1;
		Head *newHead = (Head*) malloc(
				sizeof(Head) + thisLen + len + 1 );
		if ( newHead == 0 )
			throw std::bad_alloc();
		data = (char*) (newHead+1);

		/* Set the new header and data from this. */
		newHead->refCount = 1;
		newHead->length = thisLen + len;
		memcpy( data, head+1, thisLen );
	}

	/* Return writing position. */
	return data + thisLen;
}

/*  Concatenate a String and a c-style string. */
template<class T> StrTmpl<T> operator+( const StrTmpl<T> &s1, const char *s2 )
{
	/* Find s2 length and alloc the space for the result. */
	long str1Len = (((typename StrTmpl<T>::Head*)(s1.data)) - 1)->length;
	long str2Len = strlen( s2 );

	typename StrTmpl<T>::Head *head = (typename StrTmpl<T>::Head*) 
			malloc( sizeof(typename StrTmpl<T>::Head) + str1Len + str2Len + 1 );
	if ( head == 0 )
		throw std::bad_alloc();

	/* Set up the header. */
	head->refCount = 1;
	head->length = str1Len + str2Len;

	/* Save the pointer to data and copy the data in. */
	char *data = (char*) (head+1);
	memcpy( data, s1.data, str1Len );
	memcpy( data + str1Len, s2, str2Len + 1 );
	return StrTmpl<T>( data, typename StrTmpl<T>::DisAmbig() );
}

/* Concatenate a c-style string and a String. */
template<class T> StrTmpl<T> operator+( const char *s1, const StrTmpl<T> &s2 )
{
	/* Find s2 length and alloc the space for the result. */
	long str1Len = strlen( s1 );
	long str2Len = (((typename StrTmpl<T>::Head*)(s2.data)) - 1)->length;

	typename StrTmpl<T>::Head *head = (typename StrTmpl<T>::Head*) 
			malloc( sizeof(typename StrTmpl<T>::Head) + str1Len + str2Len + 1 );
	if ( head == 0 )
		throw std::bad_alloc();

	/* Set up the header. */
	head->refCount = 1;
	head->length = str1Len + str2Len;

	/* Save the pointer to data and copy the data in. */
	char *data = (char*) (head+1);
	memcpy( data, s1, str1Len );
	memcpy( data + str1Len, s2.data, str2Len + 1 );
	return StrTmpl<T>( data, typename StrTmpl<T>::DisAmbig() );
}

/* Add two StrTmpl strings. */
template<class T> StrTmpl<T> operator+( const StrTmpl<T> &s1, const StrTmpl<T> &s2 )
{
	/* Find s2 length and alloc the space for the result. */
	long str1Len = (((typename StrTmpl<T>::Head*)(s1.data)) - 1)->length;
	long str2Len = (((typename StrTmpl<T>::Head*)(s2.data)) - 1)->length;
	typename StrTmpl<T>::Head *head = (typename StrTmpl<T>::Head*) 
			malloc( sizeof(typename StrTmpl<T>::Head) + str1Len + str2Len + 1 );
	if ( head == 0 )
		throw std::bad_alloc();

	/* Set up the header. */
	head->refCount = 1;
	head->length = str1Len + str2Len;

	/* Save the pointer to data and copy the data in. */
	char *data = (char*) (head+1);
	memcpy( data, s1.data, str1Len );
	memcpy( data + str1Len, s2.data, str2Len + 1 );
	return StrTmpl<T>( data, typename StrTmpl<T>::DisAmbig() );
}

/* Operator used in case the compiler does not support the conversion. */
template <class T> inline std::ostream &operator<<( std::ostream &o, const StrTmpl<T> &s )
{
	return o.write( s.data, s.length() );
}

typedef StrTmpl<char> String;

/**
 * \brief Compare two null terminated character sequences.
 *
 * This comparision class is a wrapper for strcmp.
 */
template<class T> struct CmpStrTmpl
{
	/**
	 * \brief Compare two null terminated string types.
	 */
	static inline long compare( const char *k1, const char *k2 )
		{ return strcmp(k1, k2); }

	static int compare( const StrTmpl<T> &s1, const StrTmpl<T> &s2 )
	{
		if ( s1.length() < s2.length() )
			return -1;
		else if ( s1.length() > s2.length() )
			return 1;
		else
			return memcmp( s1.data, s2.data, s1.length() );
	}
};

typedef CmpStrTmpl<char> CmpStr;



#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_ASTRING_H */
