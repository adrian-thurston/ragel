/*
 *  Copyright 2003 Adrian Thurston <adriant@ragel.ca>
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

#ifndef _AAPL_TVECTSIMP_H
#define _AAPL_TVECTSIMP_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "vectsimp.h"

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/* TVectSimp */
template < class T, class Resize = ResizeExpn > class TVectSimp
	: public VectSimp<char, Resize>
{
private:
	typedef VectSimp<char, Resize> BaseVector;

public:
	/* Default constructor makes an empty vector. */
	TVectSimp() { }

	/* Create a vector with an intial number of elements. */
	TVectSimp( long size )
			{ BaseVector::setAsNew( sizeof(T)*size ); }

	/* Create a vector of specific size and allocation. */
	TVectSimp( long size, long allocLen )
			: BaseVector( sizeof(T)*size, sizeof(T)*allocLen ) { }

	/* Copy constructor performs a deep copy of the vector. */
	TVectSimp(const TVectSimp<T, Resize> &b) : BaseVector( b ) { }

	/* Perform a deep copy of another vector into this vector. This needs to
	 * be overridded to provide the correct return type. */
	TVectSimp &operator=( const TVectSimp &v )
			{ BaseVector::operator=( v ); return *this; }

	/*@{*/
	/* Insert one element at position pos. */
	void insert(long pos, const T &val)
			{ BaseVector::insert( sizeof(T)*pos, (char*) &val, sizeof(T) ); }

	/* Insert an array of values. */
	void insert(long pos, const T *val, long len)
			{ BaseVector::insert( sizeof(T)*pos, (char*) val, sizeof(T)*len ); }

	/* Insert the contents of another vector. */
	void insert(long pos, const TVectSimp &v)
			{ BaseVector::insert( sizeof(T)*pos, v.data, v.tabLen); }

	/* Insert len copies of val into the vector. */
	void insertDup(long pos, const T &val, long len);

	/* Insert one new item using the default constructor. */
	void insertNew(long pos)
			{ BaseVector::insertNew( sizeof(T)*pos, sizeof(T)); }

	/* Insert len new items using default constructor. */
	void insertNew(long pos, long len)
			{ BaseVector::insertNew( sizeof(T)*pos, sizeof(T)*len ); }
	/*@}*/

	/*@{*/
	/* Delete one element. */
	void remove(long pos)
			{ BaseVector::remove( sizeof(T)*pos, sizeof(T) ); }

	/* Delete a number of elements. */
	void remove(long pos, long len)
			{ BaseVector::remove( sizeof(T)*pos, sizeof(T)*len ); }

	/*@}*/

	/*@{*/
	/* Replace a single element. */
	void replace(long pos, const T &val)
			{ BaseVector::replace( sizeof(T)*pos, (char*)&val, sizeof(T) ); }

	/* Replace with an array of values. */
	void replace(long pos, const T *val, long len)
			{ BaseVector::replace( sizeof(T)*pos, (char*)val, sizeof(T)*len ); }

	/* Replace with the contents of another vector. */
	void replace(long pos, const TVectSimp &v)
			{ BaseVector::replace( sizeof(T)*pos, v.data, v.tabLen ); }

	/* Replace len items with len copies of val. */
	void replaceDup(long pos, const T &val, long len);

	/* Replace one item at pos with a constructed object. */
	void replaceNew(long pos)
			{ BaseVector::replaceNew( sizeof(T)*pos, sizeof(T) ); }

	/* Replace len items at pos with newly constructed objects. */
	void replaceNew(long pos, long len)
			{ BaseVector::replaceNew( sizeof(T)*pos, sizeof(T)*len ); }
	/*@}*/

	/*@{*/
	/* Set the vector to be val exactly. */
	void setAs(const T &val)
			{ BaseVector::setAs( (char*)&val, sizeof(T) ); }

	/* Set to the contents of an array. */
	void setAs(const T *val, long len)
			{ BaseVector::setAs( (char*)val, sizeof(T) ); }

	/* Set to the contents of another vector. */
	void setAs(const TVectSimp &v)
			{ BaseVector::setAs( v.data, v.tabLen ); }

	/* Set as len copies of item. */
	void setAsDup(const T &item, long len);

	/* Set as a newly constructed object using the default constructor. */
	void setAsNew()
			{ BaseVector::setAsNew( sizeof(T) ); }

	/* Set as newly constructed objects using the default constructor. */
	void setAsNew(long len)
			{ BaseVector::setAsNew( sizeof(T)*len ); }
	/*@}*/

	/*@{*/
	/* Append a single element. */
	void append(const T &val)
			{ BaseVector::replace( BaseVector::tabLen, (char*)&val, sizeof(T) ); }

	/* Append an array of elements. */
	void append(const T *val, long len)
			{ BaseVector::replace( BaseVector::tabLen, (char*)val, sizeof(T)*len ); }

	/* Append to the end of the vector from another vector. */
	void append(const TVectSimp &v)
			{ BaseVector::replace( BaseVector::tabLen, v.data, v.tabLen ); }

	/* Append len copies of item. */
	void appendDup(const T &item, long len);

	/* Append a newly created item. Uses the copy constructor. */
	void appendNew()
			{ BaseVector::replaceNew( BaseVector::tabLen, sizeof(T) ); }

	/* Append newly created items. Uses the copy constructor. */
	void appendNew(long len)
			{ BaseVector::replaceNew( BaseVector::tabLen, sizeof(T)*len); }
	/*@}*/

	/*@{*/
	/* Prepend a single element. */
	void prepend(const T &val)
			{ BaseVector::insert( 0, (char*)&val, sizeof(T) ); }

	/* Prepend an array of elements. */
	void prepend(const T *val, long len)
			{ BaseVector::insert( 0, (char*)val, sizeof(T)*len ); }

	/* Prepend to the front of the vector from another vector. */
	void prepend(const TVectSimp &v)
			{ BaseVector::insert( 0, v.data, v.tabLen ); }

	/* Prepend len copies of item. */
	void prependDup(const T &item, long len);

	/* Prepend a newly created item. Uses the copy constructor to init. */
	void prependNew()
			{ BaseVector::insertNew( 0, sizeof(T) ); }

	/* Prepend newly created items. Uses the copy constructor to init. */
	void prependNew(long len)
			{ BaseVector::insertNew( 0, sizeof(T)*len ); }
	/*@}*/

	/* Convenience access. */
	T &operator[](int i) const { return ((T*)BaseVector::data)[i]; }
	long length() const         { return BaseVector::tabLen/sizeof(T); }
	long size() const           { return BaseVector::tabLen/sizeof(T); }

	/* Forward this so a ref can be used. */
	struct Iter;

	/* Various classes for setting the iterator */
	struct IterFirst { IterFirst( const TVectSimp &v ) : v(v) { } const TVectSimp &v; };
	struct IterLast { IterLast( const TVectSimp &v ) : v(v) { } const TVectSimp &v; };
	struct IterNext { IterNext( const Iter &i ) : i(i) { } const Iter &i; };
	struct IterPrev { IterPrev( const Iter &i ) : i(i) { } const Iter &i; };

	/* Vector iterator. */
	struct Iter
	{
		/* Construct, assign. */
		Iter() : ptr(0), ptrBeg(0), ptrEnd(0) { }

		/* Construct. */
		Iter( const TVectSimp &v );
		Iter( const IterFirst &vf );
		Iter( const IterLast &vl );
		inline Iter( const IterNext &vn );
		inline Iter( const IterPrev &vp );

		/* Assign. */
		Iter &operator=( const TVectSimp &v );
		Iter &operator=( const IterFirst &vf );
		Iter &operator=( const IterLast &vl );
		inline Iter &operator=( const IterNext &vf );
		inline Iter &operator=( const IterPrev &vl );

		/* Off the end, beginning. */
		bool lte() const { return ptr != ptrEnd; }
		bool end() const { return ptr == ptrEnd; }
		bool gtb() const { return ptr != ptrBeg; }
		bool beg() const { return ptr == ptrBeg; }

		/* At the first, last element. */
		bool first() const { return ptr == ptrBeg+1; }
		bool last() const { return ptr == ptrEnd-1; }

		/* Return the position. */
		long pos() const { return ptr - ptrBeg - 1; }
		T &operator[](int i) const { return ptr[i]; }

		/* Cast, dereference, arrow ops. */
		operator T*() const   { return ptr; }
		T &operator *() const { return *ptr; }
		T *operator->() const { return ptr; }

		/* Increment. */
		T *operator++()       { return ++ptr; }
		T *operator++(int)    { return ptr++; }
		T *increment()        { return ++ptr; }

		/* Decrement. */
		T *operator--()       { return --ptr; }
		T *operator--(int)    { return ptr--; }
		T *decrement()        { return --ptr; }

		/* Return classes used to set an iterator to the next, prev. */
		inline IterNext next() const { return IterNext(*this); }
		inline IterPrev prev() const { return IterPrev(*this); }

		/* The iterator is simply a pointer. */
		T *ptr;
		T *ptrBeg, *ptrEnd;
	};

	/* Return classes used to set an iterator to the first element. */
	IterFirst first() { return IterFirst( *this ); }
	IterLast last() { return IterLast( *this ); }

protected:
#ifdef VECT_COMPLEX
 	void makeRawSpaceFor(long pos, long len);
#endif

	void upResize(long len);
	void downResize(long len);
};


/* Init a vector iterator with just a vector. */
template <class T, class Resize> TVectSimp<T, Resize>::Iter::Iter( const TVectSimp &v )
{
	if ( v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = (T*)v.data;
		ptrBeg = ((T*)v.data)-1;
		ptrEnd = (T*)(v.data+v.tabLen);
	}
}

/* Init a vector iterator with the first of a vector. */
template <class T, class Resize> TVectSimp<T, Resize>::Iter::Iter( 
		const IterFirst &vf )
{
	if ( vf.v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = (T*)vf.v.data;
		ptrBeg = ((T*)vf.v.data)-1;
		ptrEnd = (T*)(vf.v.data+vf.v.tabLen);
	}
}

/* Init a vector iterator with the last of a vector. */
template <class T, class Resize> TVectSimp<T, Resize>::Iter::Iter( 
		const IterLast &vl )
{
	if ( vl.v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = ((T*)(vl.v.data+vl.v.tabLen))-1;
		ptrBeg = ((T*)vl.v.data)-1;
		ptrEnd = (T*)(vl.v.data+vl.v.tabLen);
	}
}

/* Init a vector iterator with the next of some other iterator. */
template <class T, class Resize> TVectSimp<T, Resize>::Iter::Iter( 
		const IterNext &vn )
:
	ptr(vn.i.ptr+1), 
	ptrBeg(vn.i.ptrBeg),
	ptrEnd(vn.i.ptrEnd)
{
}

/* Init a vector iterator with the prev of some other iterator. */
template <class T, class Resize> TVectSimp<T, Resize>::Iter::Iter( 
		const IterPrev &vp )
:
	ptr(vp.i.ptr-1),
	ptrBeg(vp.i.ptrBeg),
	ptrEnd(vp.i.ptrEnd)
{
}

/* Set a vector iterator with some vector. */
template <class T, class Resize> typename TVectSimp<T, Resize>::Iter &
		TVectSimp<T, Resize>::Iter::operator=( const TVectSimp &v )
{
	if ( v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = (T*)v.data; 
		ptrBeg = ((T*)v.data)-1; 
		ptrEnd = (T*)(v.data+v.tabLen);
	}
	return *this;
}

/* Set a vector iterator with the first element in a vector. */
template <class T, class Resize> typename TVectSimp<T, Resize>::Iter &
		TVectSimp<T, Resize>::Iter::operator=( const IterFirst &vf )
{
	if ( vf.v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = (T*)vf.v.data; 
		ptrBeg = ((T*)vf.v.data)-1; 
		ptrEnd = (T*)(vf.v.data+vf.v.tabLen);
	}
	return *this;
}

/* Set a vector iterator with the last element in a vector. */
template <class T, class Resize> typename TVectSimp<T, Resize>::Iter &
		TVectSimp<T, Resize>::Iter::operator=( const IterLast &vl )
{
	if ( vl.v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = ((T*)(vl.v.data+vl.v.tabLen))-1; 
		ptrBeg = ((T*)vl.v.data)-1; 
		ptrEnd = (T*)(vl.v.data+vl.v.tabLen);
	}
	return *this;
}

/* Set a vector iterator with the next of some other iterator. */
template <class T, class Resize> typename TVectSimp<T, Resize>::Iter &
		TVectSimp<T, Resize>::Iter::operator=( const IterNext &vn )
{
	ptr = vn.i.ptr+1; 
	ptrBeg = vn.i.ptrBeg;
	ptrEnd = vn.i.ptrEnd;
	return *this;
}

/* Set a vector iterator with the prev of some other iterator. */
template <class T, class Resize> typename TVectSimp<T, Resize>::Iter &
		TVectSimp<T, Resize>::Iter::operator=( const IterPrev &vp )
{
	ptr = vp.i.ptr-1; 
	ptrBeg = vp.i.ptrBeg;
	ptrEnd = vp.i.ptrEnd;
	return *this;
}

/**
 * \brief Set the vector to len copies of item.
 *
 * The vector becomes len elements in length. Any existing elements are not
 * uninitialized. The assignment operator is used to copy the item into the
 * vector.
 */
template<class T, class Resize> void TVectSimp<T, Resize>::
		setAsDup(const T &item, long len)
{
	/* Set it with new elements. */
	BaseVector::setAsNew( sizeof(T)*len );
	
	/* Copy the data item in one at a time. */
	T *dst = (T*)BaseVector::data;
	for ( long i = 0; i < len; i++ )
		*dst++ = item;
}

/**
 * \brief Replace at position pos with len copies of an item.
 *
 * If there are existing elements at the positions to be replaced then they
 * are not uninitialized in any way. The assignment operator is used to place
 * the item into the vector. It is allowable for the pos and length to specify
 * a replacement that overwrites existing elements and creates new ones. If
 * pos is greater than the length of the vector then undefined behaviour
 * results.  If pos is negative, then it is treated as an offset relative to
 * the length of the vector.
 */
template<class T, class Resize> void TVectSimp<T, Resize>::
		replaceDup(long pos, const T &item, long len)
{
	/* First replace with new elements. */
	BaseVector::replaceNew( sizeof(T)*pos, sizeof(T)*len );

	/* Copy the data item in one at a time. */
	T *dst = (T*)BaseVector::data + pos;
	for ( long i = 0; i < len; i++ )
		*dst++ = item;
}


/**
 * \brief Insert len copies of item at position pos.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * The assignment operator is used to place the element into vector. If pos is
 * greater than the length of the vector then undefined behaviour results. If
 * pos is negative then it is treated as an offset relative to the length of
 * the vector.
 */
template<class T, class Resize> void TVectSimp<T, Resize>::
		insertDup(long pos, const T &item, long len)
{
	/* First insert with new elements. */
	BaseVector::insertNew( sizeof(T)*pos, sizeof(T)*len );

	/* Copy the data item in one at a time. */
	T *dst = (T*)BaseVector::data + pos;
	for ( long i = 0; i < len; i++ )
		*dst++ = item;
}



/**
 * \brief Append len copies of item.
 *
 * The assignment operator is used to place the item in the vector.
 */
template<class T, class Resize> void TVectSimp<T, Resize>::
		appendDup(const T &item, long len)
{
	/* Save the old table length. */
	long oldLen = BaseVector::tabLen;

	/* First append new elements. */
	BaseVector::replaceNew( BaseVector::tabLen, sizeof(T)*len );

	/* Copy the data item in one at a time. */
	T *dst = (T*)(BaseVector::data + oldLen);
	for ( long i = 0; i < len; i++ )
		*dst++ = item;
}
	
/**
 * \brief Prepend len copies of item.
 *
 * The assignment operator is used to place the item in the vector.
 */
template<class T, class Resize> void TVectSimp<T, Resize>::
		prependDup(const T &item, long len)
{
	/* First insert new elements. */
	BaseVector::insertNew( 0, sizeof(T)*len );

	/* Copy the data item in one at a time. */
	T *dst = (T*)BaseVector::data;
	for ( long i = 0; i < len; i++ )
		*dst++ = item;
}
	
#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_TVECTSIMP_H */
