/*
 *  Copyright 2001, 2002 Adrian Thurston <adriant@ragel.ca>
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

/* This header is not wrapped in ifndefs because it is
 * not intended to be included by users directly. */

#include <new>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "table.h"

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/* Vector */
template < class T, class Resize = ResizeExpn > class Vector
	: public Table<T>, public Resize
{
private:
	typedef Table<T> BaseTable;

public:
	/**
	 * \brief Initialize an empty vector with no space allocated.  
	 *
	 * If a linear resizer is used, the step defaults to 256 units of T. For a
	 * runtime vector both up and down allocation schemes default to
	 * Exponential.
	 */
	Vector() { }

	/* Create a vector with an intial number of elements. */
	Vector( long size ) { setAsNew( size ); }

	/* Create a vector of specific size and allocation. */
	Vector( long size, long allocLen );

	/* Deep copy. */
	Vector( const Vector &v );

	/* Free all mem used by the vector. */
	~Vector() { empty(); }

	/* Delete all items. */
	void empty();

	/* Abandon the contents of the vector without deleteing. */
	void abandon();

	/* Performs a shallow copy of another vector into this vector. If this
	 * vector is non-empty then its contents are lost (not freed). */
	void shallowCopy( const Vector &v );

	/* Perform a deep copy of another vector into this vector. */
	Vector &operator=( const Vector &v );


	/*@{*/
	/* Insert one element at position pos. */
	void insert(long pos, const T &val)    { insert(pos, &val, 1); }

	/* Insert an array of values. */
	void insert(long pos, const T *val, long len);

	/* Insert the contents of another vector. */
	void insert(long pos, const Vector &v) { insert(pos, v.data, v.tabLen); }

	/* Insert len copies of val into the vector. */
	void insertDup(long pos, const T &val, long len);

	/* Insert one new item using the default constructor. */
	void insertNew(long pos)               { insertNew(pos, 1); }

	/* Insert len new items using default constructor. */
	void insertNew(long pos, long len);
	/*@}*/

	/*@{*/
	/* Delete one element. */
	void remove(long pos)                 { remove(pos, 1); }

	/* Delete a number of elements. */
	void remove(long pos, long len);
	/*@}*/

	/*@{*/
	/* Replace a single element. */
	void replace(long pos, const T &val)    { replace(pos, &val, 1); }

	/* Replace with an array of values. */
	void replace(long pos, const T *val, long len);

	/* Replace with the contents of another vector. */
	void replace(long pos, const Vector &v) { replace(pos, v.data, v.tabLen); }

	/* Replace len items with len copies of val. */
	void replaceDup(long pos, const T &val, long len);

	/* Replace one item at pos with a constructed object. */
	void replaceNew(long pos)               { replaceNew(pos, 1); }

	/* Replace len items at pos with newly constructed objects. */
	void replaceNew(long pos, long len);
	/*@}*/

	/*@{*/
	/* Set the vector to be val exactly. */
	void setAs(const T &val)             { setAs(&val, 1); }

	/* Set to the contents of an array. */
	void setAs(const T *val, long len);

	/* Set to the contents of another vector. */
	void setAs(const Vector &v)          { setAs(v.data, v.tabLen); }

	/* Set as len copies of item. */
	void setAsDup(const T &item, long len);

	/* Set as a newly constructed object using the default constructor. */
	void setAsNew()                      { setAsNew(1); }

	/* Set as newly constructed objects using the default constructor. */
	void setAsNew(long len);
	/*@}*/

	/*@{*/
	/* Append a single element. */
	void append(const T &val)                { replace(BaseTable::tabLen, &val, 1); }

	/* Append an array of elements. */
	void append(const T *val, long len)       { replace(BaseTable::tabLen, val, len); }

	/* Append to the end of the vector from another vector. */
	void append(const Vector &v)             { replace(BaseTable::tabLen, v.data, v.tabLen); }

	/* Append len copies of item. */
	void appendDup(const T &item, long len)   { replaceDup(BaseTable::tabLen, item, len); }

	/* Append a newly created item. Uses the copy constructor. */
	void appendNew()                         { replaceNew(BaseTable::tabLen, 1); }

	/* Append newly created items. Uses the copy constructor. */
	void appendNew(long len)                  { replaceNew(BaseTable::tabLen, len); }
	/*@}*/
	
	/*@{*/
	/* Prepend a single element. */
	void prepend(const T &val)               { insert(0, &val, 1); }

	/* Prepend an array of elements. */
	void prepend(const T *val, long len)      { insert(0, val, len); }

	/* Prepend to the front of the vector from another vector. */
	void prepend(const Vector &v)            { insert(0, v.data, v.tabLen); }

	/* Prepend len copies of item. */
	void prependDup(const T &item, long len)  { insertDup(0, item, len); }

	/* Prepend a newly created item. Uses the copy constructor to init. */
	void prependNew()                        { insertNew(0, 1); }

	/* Prepend newly created items. Uses the copy constructor to init. */
	void prependNew(long len)                 { insertNew(0, len); }
	/*@}*/

	/* Convenience access. */
	T &operator[](int i) const { return BaseTable::data[i]; }
	long size() const           { return BaseTable::tabLen; }

	/* Forward this so a ref can be used. */
	struct Iter;

	/* Various classes for setting the iterator */
	struct IterFirst { IterFirst( const Vector &v ) : v(v) { } const Vector &v; };
	struct IterLast { IterLast( const Vector &v ) : v(v) { } const Vector &v; };
	struct IterNext { IterNext( const Iter &i ) : i(i) { } const Iter &i; };
	struct IterPrev { IterPrev( const Iter &i ) : i(i) { } const Iter &i; };

	/** 
	 * \brief Vector Iterator.
	 * \ingroup iterators
	 */
	struct Iter
	{
		/* Construct, assign. */
		Iter() : ptr(0), ptrBeg(0), ptrEnd(0) { }

		/* Construct. */
		Iter( const Vector &v );
		Iter( const IterFirst &vf );
		Iter( const IterLast &vl );
		inline Iter( const IterNext &vn );
		inline Iter( const IterPrev &vp );

		/* Assign. */
		Iter &operator=( const Vector &v );
		Iter &operator=( const IterFirst &vf );
		Iter &operator=( const IterLast &vl );
		inline Iter &operator=( const IterNext &vf );
		inline Iter &operator=( const IterPrev &vl );

		/** \brief Less than end? */
		bool lte() const { return ptr != ptrEnd; }

		/** \brief At end? */
		bool end() const { return ptr == ptrEnd; }

		/** \brief Greater than beginning? */
		bool gtb() const { return ptr != ptrBeg; }

		/** \brief At beginning? */
		bool beg() const { return ptr == ptrBeg; }

		/** \brief At first element? */
		bool first() const { return ptr == ptrBeg+1; }

		/** \brief At last element? */
		bool last() const { return ptr == ptrEnd-1; }

		/* Return the position. */
		long pos() const { return ptr - ptrBeg - 1; }
		T &operator[](int i) const { return ptr[i]; }

		/** \brief Implicit cast to T*. */
		operator T*() const   { return ptr; }

		/** \brief Dereference operator returns T&. */
		T &operator *() const { return *ptr; }

		/** \brief Arrow operator returns T*. */
		T *operator->() const { return ptr; }

		/** \brief Move to next item. */
		T *operator++()       { return ++ptr; }

		/** \brief Move to next item. */
		T *operator++(int)    { return ptr++; }

		/** \brief Move to next item. */
		T *increment()        { return ++ptr; }

		/** \brief Move n items forward. */
		T *operator+=(long n)       { return ptr+=n; }

		/** \brief Move to previous item. */
		T *operator--()       { return --ptr; }

		/** \brief Move to previous item. */
		T *operator--(int)    { return ptr--; }

		/** \brief Move to previous item. */
		T *decrement()        { return --ptr; }
		
		/** \brief Move n items back. */
		T *operator-=(long n)       { return ptr-=n; }

		/** \brief Return the next item. Does not modify this. */
		inline IterNext next() const { return IterNext(*this); }

		/** \brief Return the previous item. Does not modify this. */
		inline IterPrev prev() const { return IterPrev(*this); }

		/** \brief The iterator is simply a pointer. */
		T *ptr;

		/* For testing endpoints. */
		T *ptrBeg, *ptrEnd;
	};

	/** \brief Return first element. */
	IterFirst first() { return IterFirst( *this ); }

	/** \brief Return last element. */
	IterLast last() { return IterLast( *this ); }

protected:
#ifdef VECT_COMPLEX
 	void makeRawSpaceFor(long pos, long len);
#endif

	void upResize(long len);
	void downResize(long len);
};

/* Create a vector with an intial number of elements and size. */
template<class T, class Resize> Vector<T, Resize>::
		Vector( long size, long allocLen )
{
	/* Allocate the space if we are given a positive allocLen. */
	BaseTable::allocLen = allocLen;
	if ( allocLen > 0 ) {
		BaseTable::data = (T*) malloc(sizeof(T) * BaseTable::allocLen);
		if ( BaseTable::data == 0 )
			throw std::bad_alloc();
	}

	/* Grow to the size specified. If we did not have enough space
	 * allocated that is ok. Table will be grown to right size. */
	setAsNew( size );
}

/* Init a vector iterator with just a vector. */
template <class T, class Resize> Vector<T, Resize>::Iter::Iter( const Vector &v ) 
{
	if ( v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = v.data;
		ptrBeg = v.data-1;
		ptrEnd = v.data+v.tabLen;
	}
}

/* Init a vector iterator with the first of a vector. */
template <class T, class Resize> Vector<T, Resize>::Iter::Iter( 
		const IterFirst &vf ) 
{
	if ( vf.v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = vf.v.data;
		ptrBeg = vf.v.data-1;
		ptrEnd = vf.v.data+vf.v.tabLen;
	}
}

/* Init a vector iterator with the last of a vector. */
template <class T, class Resize> Vector<T, Resize>::Iter::Iter( 
		const IterLast &vl ) 
{
	if ( vl.v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = vl.v.data+vl.v.tabLen-1;
		ptrBeg = vl.v.data-1;
		ptrEnd = vl.v.data+vl.v.tabLen;
	}
}

/* Init a vector iterator with the next of some other iterator. */
template <class T, class Resize> Vector<T, Resize>::Iter::Iter( 
		const IterNext &vn ) 
:
	ptr(vn.i.ptr+1), 
	ptrBeg(vn.i.ptrBeg),
	ptrEnd(vn.i.ptrEnd)
{
}

/* Init a vector iterator with the prev of some other iterator. */
template <class T, class Resize> Vector<T, Resize>::Iter::Iter( 
		const IterPrev &vp ) 
:
	ptr(vp.i.ptr-1),
	ptrBeg(vp.i.ptrBeg),
	ptrEnd(vp.i.ptrEnd)
{
}

/* Set a vector iterator with some vector. */
template <class T, class Resize> typename Vector<T, Resize>::Iter &
		Vector<T, Resize>::Iter::operator=( const Vector &v )    
{
	if ( v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = v.data; 
		ptrBeg = v.data-1; 
		ptrEnd = v.data+v.tabLen; 
	}
	return *this;
}

/* Set a vector iterator with the first element in a vector. */
template <class T, class Resize> typename Vector<T, Resize>::Iter &
		Vector<T, Resize>::Iter::operator=( const IterFirst &vf )    
{
	if ( vf.v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = vf.v.data; 
		ptrBeg = vf.v.data-1; 
		ptrEnd = vf.v.data+vf.v.tabLen; 
	}
	return *this;
}

/* Set a vector iterator with the last element in a vector. */
template <class T, class Resize> typename Vector<T, Resize>::Iter &
		Vector<T, Resize>::Iter::operator=( const IterLast &vl )    
{
	if ( vl.v.tabLen == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = vl.v.data+vl.v.tabLen-1; 
		ptrBeg = vl.v.data-1; 
		ptrEnd = vl.v.data+vl.v.tabLen; 
	}
	return *this;
}

/* Set a vector iterator with the next of some other iterator. */
template <class T, class Resize> typename Vector<T, Resize>::Iter &
		Vector<T, Resize>::Iter::operator=( const IterNext &vn )    
{
	ptr = vn.i.ptr+1; 
	ptrBeg = vn.i.ptrBeg;
	ptrEnd = vn.i.ptrEnd;
	return *this;
}

/* Set a vector iterator with the prev of some other iterator. */
template <class T, class Resize> typename Vector<T, Resize>::Iter &
		Vector<T, Resize>::Iter::operator=( const IterPrev &vp )    
{
	ptr = vp.i.ptr-1; 
	ptrBeg = vp.i.ptrBeg;
	ptrEnd = vp.i.ptrEnd;
	return *this;
}

/**
 * \brief Forget all elements in the vector.
 *
 * The contents of the vector are reset to null without without the space
 * being freed.
 */
template<class T, class Resize> void Vector<T, Resize>::
		abandon()
{
	BaseTable::data = 0;
	BaseTable::tabLen = 0;
	BaseTable::allocLen = 0;
}

/**
 * \brief Shallow copy another vector into this vector. 
 *
 * The dynamic array of the other vector is copied into this vector by
 * reference. If this vector is non-empty then its contents are lost. This
 * routine must be used with care. After a shallow copy one vector should
 * abandon its contents to prevent both destructors from attempting to free
 * the common array.
 */
template<class T, class Resize> void Vector<T, Resize>::
		shallowCopy( const Vector &v )
{
	BaseTable::data = v.data;
	BaseTable::tabLen = v.tabLen;
	BaseTable::allocLen = v.allocLen;
}

/**
 * \brief Deep copy another vector into this vector.
 *
 * Copies the entire contents of the other vector into this vector. Any
 * existing contents are first deleted. Equivalent to setAs.
 *
 * \returns A reference to this.
 */
template<class T, class Resize> Vector<T, Resize> &Vector<T, Resize>::
		operator=( const Vector &v )
{
	setAs(v.data, v.tabLen); 
	return *this;
}

/* Up resize the data for len elements using Resize::upResize to tell us the
 * new tabLen. Reads and writes allocLen. Does not read or write tabLen. */
template<class T, class Resize> void Vector<T, Resize>::
		upResize(long len)
{
	/* Ask the resizer what the new tabLen will be. */
	long newLen = Resize::upResize(BaseTable::allocLen, len);

	/* Did the data grow? */
	if ( newLen > BaseTable::allocLen ) {
		BaseTable::allocLen = newLen;
		if ( BaseTable::data != 0 ) {
			/* Table exists already, resize it up. */
			BaseTable::data = (T*) realloc( BaseTable::data, sizeof(T) * newLen );
			if ( BaseTable::data == 0 )
				throw std::bad_alloc();
		}
		else {
			/* Create the data. */
			BaseTable::data = (T*) malloc( sizeof(T) * newLen );
			if ( BaseTable::data == 0 )
				throw std::bad_alloc();
		}
	}
}

/* Down resize the data for len elements using Resize::downResize to determine
 * the new tabLen. Reads and writes allocLen. Does not read or write tabLen. */
template<class T, class Resize> void Vector<T, Resize>::
		downResize(long len)
{
	/* Ask the resizer what the new tabLen will be. */
	long newLen = Resize::downResize( BaseTable::allocLen, len );

	/* Did the data shrink? */
	if ( newLen < BaseTable::allocLen ) {
		BaseTable::allocLen = newLen;
		if ( newLen == 0 ) {
			/* Simply free the data. */
			free( BaseTable::data );
			BaseTable::data = 0;
		}
		else {
			/* Not shrinking to size zero, realloc it to the smaller size. */
			BaseTable::data = (T*) realloc( BaseTable::data, sizeof(T) * newLen );
			if ( BaseTable::data == 0 )
				throw std::bad_alloc();
		}
	}
}

#ifdef AAPL_NAMESPACE
}
#endif

