/*
 *  Copyright 2002 Adrian Thurston <thurston@cs.queensu.ca>
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

#ifndef _AAPL_VECTOR_H
#define _AAPL_VECTOR_H

#define VECT_COMPLEX
#include "vectcommon.h"
#undef VECT_COMPLEX

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \addtogroup vector
 * @{
 */

/** \class Vector
 * \brief Dynamic array for classes with non-trivial initialization.
 *
 * This is typical vector implementation. It is a dynamic array that can be
 * used to contain complex data structures that have constructors and
 * destructors as well as simple types such as integers and pointers.
 *
 * Vector supports inserting, overwriting, and removing single or multiple
 * elements at once. Constructors and destructors are called wherever
 * appropriate.  For example, before an element is overwritten, it's
 * destructor is called.
 *
 * Vector provides automatic resizing of allocated memory as needed and offers
 * different allocation schemes for controlling how the automatic allocation
 * is done.  Two senses of the the length of the data is maintained: the
 * amount of raw memory allocated to the vector and the number of actual
 * elements in the vector. The various allocation schemes control how the
 * allocated space is changed in relation to the number of elements in the
 * vector.
 *
 * \include ex_vector.cpp
 */

/*@}*/

/** \fn Vector::Vector( long size )
 * \brief Create a vector with a specified number of initial elements.
 *
 * Default constructors are used to create the new elements. The initial space
 * allocation is zero and it is grown according to the up allocation scheme
 * used.  If a linear resizer is used, the step defaults to 256 units of T.
 * For a runtime vector both up and down allocation schemes default to
 * Exponential.
 * 
 * This constructor is not of any use to a vector with a constant up resizing
 * scheme as it starts with no space allocated and is unable to grow. It will
 * assertion fail with a non-zero size value.
 *
 */

/** \fn Vector::Vector( long size, long allocLen )
 * \brief Create a vector with a specified number of initial elements and
 * allocation.
 *
 * Default constructors are used to create the new elements. If the size is
 * greater than allocLen, then the space will be grown according to the
 * allocation scheme. Both size and allocLen are in units of T. If a linear
 * resizer is used, the step defaults to 256 units of T. For a runtime vector
 * both up and down allocation schemes default to Exponential.
 */

/**
 * \brief Perform a deep copy of the vector.
 *
 * The contents of the other vector are copied into this vector. This vector
 * gets the same allocation size as the other vector. All items are copied
 * using the element's copy constructor.
 */
template<class T, class Resize> Vector<T, Resize>::
		Vector(const Vector<T, Resize> &v)
{
	BaseTable::tabLen = v.tabLen;
	BaseTable::allocLen = v.allocLen;

	if ( BaseTable::allocLen > 0 ) {
		/* Allocate needed space. */
		BaseTable::data = (T*) malloc(sizeof(T) * BaseTable::allocLen);
		if ( BaseTable::data == 0 )
			throw std::bad_alloc();

		/* If there are any items in the src data, copy them in. */
		T *dst = BaseTable::data, *src = v.data;
		for (long pos = 0; pos < BaseTable::tabLen; pos++, dst++, src++ )
			new(dst) T(*src);
	}
	else {
		/* Nothing allocated. */
		BaseTable::data = 0;
	}
}

/** \fn Vector::~Vector()
 * \brief Free all memory used by the vector. 
 *
 * The vector is reset to zero elements. Destructors are called on all
 * elements in the vector. The space allocated for the vector is freed.
 */


/**
 * \brief Free all memory used by the vector. 
 *
 * The vector is reset to zero elements. Destructors are called on all
 * elements in the vector. The space allocated for the vector is freed.
 */
template<class T, class Resize> void Vector<T, Resize>::
		empty()
{
	if ( BaseTable::data != 0 ) {
		/* Call All destructors. */
		T *pos = BaseTable::data;
		for ( long i = 0; i < BaseTable::tabLen; pos++, i++ )
			pos->~T();

		/* Free the data space. */
		free( BaseTable::data );
		BaseTable::data = 0;
		BaseTable::tabLen = BaseTable::allocLen = 0;
	}
}

/** \fn Vector::setAs(const T &val)
 * \brief Set the contents of the vector to be val exactly.
 *
 * The vector becomes one element in length. Destructors are called on any
 * existing elements in the vector. The element's copy constructor is used to
 * place the val in the vector.
 */

/**
 * \brief Set the contents of the vector to be len elements exactly. 
 *
 * The vector becomes len elements in length. Destructors are called on any
 * existing elements in the vector. Copy constructors are used to place the
 * new elements in the vector. 
 */
template<class T, class Resize> void Vector<T, Resize>::
		setAs(const T *val, long len)
{
	/* Call All destructors. */
	long i;
	T *pos = BaseTable::data;
	for ( i = 0; i < BaseTable::tabLen; pos++, i++ )
		pos->~T();

	/* Adjust the allocated length. */
	if ( len < BaseTable::tabLen )
		downResize( len );
	else if ( len > BaseTable::tabLen )
		upResize( len );

	/* Set the new data length to exactly len. */
	BaseTable::tabLen = len;	
	
	/* Copy data in. */
	T *dst = BaseTable::data;
	const T *src = val;
	for ( i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);
}

/** \fn Vector::setAs(const Vector<T, Resize> &v)
 * \brief Set the vector to exactly the contents of another vector.
 *
 * The vector becomes v.tabLen elements in length. Destructors are called
 * on any existing elements. Copy constructors are used to place the new
 * elements in the vector.
 */

/**
 * \brief Set the vector to len copies of item.
 *
 * The vector becomes len elements in length. Destructors are called on any
 * existing elements in the vector. The element's copy constructor is used to
 * copy the item into the vector.
 */
template<class T, class Resize> void Vector<T, Resize>::
		setAsDup(const T &item, long len)
{
	/* Call All destructors. */
	T *pos = BaseTable::data;
	for ( long i = 0; i < BaseTable::tabLen; pos++, i++ )
		pos->~T();

	/* Adjust the allocated length. */
	if ( len < BaseTable::tabLen )
		downResize( len );
	else if ( len > BaseTable::tabLen )
		upResize( len );

	/* Set the new data length to exactly len. */
	BaseTable::tabLen = len;	
	
	/* Copy item in one spot at a time. */
	T *dst = BaseTable::data;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(item);
}

/** \fn Vector::setAsNew()
 * \brief Set the vector to exactly one new item.
 *
 * The vector becomes one element in length. Destructors are called on any
 * existing elements in the vector. The default constructor is used to init
 * the new item.
 */

/**
 * \brief Set the vector to exactly len new items.
 *
 * The vector becomes len elements in length. Destructors are called on any
 * existing elements in the vector. Default constructors are used to init the
 * new items.
 */
template<class T, class Resize> void Vector<T, Resize>::
		setAsNew(long len)
{
	/* Call All destructors. */
	T *pos = BaseTable::data;
	for ( long i = 0; i < BaseTable::tabLen; pos++, i++ )
		pos->~T();

	/* Adjust the allocated length. */
	if ( len < BaseTable::tabLen )
		downResize( len );
	else if ( len > BaseTable::tabLen )
		upResize( len );

	/* Set the new data length to exactly len. */
	BaseTable::tabLen = len;	
	
	/* Create items using default constructor. */
	T *dst = BaseTable::data;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();
}

/** \fn Vector::replace(long pos, const T &val)
 * \brief Replace one element at position pos.
 *
 * If there is an existing element at position pos (if pos is less than the
 * length of the vector) then its destructor is called before the space is
 * used. The copy constructor is used to place the element into the vector.
 * If pos is greater than the length of the vector then undefined behaviour
 * results.  If pos is negative then it is treated as an offset relative to
 * the length of the vector.
 */

/**
 * \brief Replace len elements at position pos.
 *
 * If there are existing elements at the positions to be replaced, then
 * destructors are called before the space is used. Copy constructors are used
 * to place the elements into the vector. It is allowable for the pos and
 * length to specify a replacement that overwrites existing elements and
 * creates new ones.  If pos is greater than the length of the vector then
 * undefined behaviour results. If pos is negative, then it is treated as an
 * offset relative to the length of the vector.
 */
template<class T, class Resize> void Vector<T, Resize>::
		replace(long pos, const T *val, long len)
{
	long endPos, i;
	T *item;

	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	endPos = pos + len;

	/* Make sure we have enough space. */
	if ( endPos > BaseTable::tabLen ) {
		upResize( endPos );

		/* Delete any objects we need to delete. */
		item = BaseTable::data + pos;
		for ( i = pos; i < BaseTable::tabLen; i++, item++ )
			item->~T();
		
		/* We are extending the vector, set the new data length. */
		BaseTable::tabLen = endPos;
	}
	else {
		/* Delete any objects we need to delete. */
		item = BaseTable::data + pos;
		for ( i = pos; i < endPos; i++, item++ )
			item->~T();
	}

	/* Copy data in using copy constructor. */
	T *dst = BaseTable::data + pos;
	const T *src = val;
	for ( i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);
}

/** \fn Vector::replace(long pos, const Vector<T, Resize> &v)
 * \brief Replace at position pos with all the elements of another vector.
 *
 * Replace at position pos with all the elements of another vector. The other
 * vector is left unchanged. If there are existing elements at the positions
 * to be replaced, then destructors are called before the space is used. Copy
 * constructors are used to place the elements into this vector. It is
 * allowable for the pos and length of the other vector to specify a
 * replacement that overwrites existing elements and creates new ones.  If pos
 * is greater than the length of the vector then undefined behaviour results.
 * If pos is negative, then it is treated as an offset relative to the length
 * of the vector.
 */

/**
 * \brief Replace at position pos with len copies of an item.
 *
 * If there are existing elements at the positions to be replaced, then
 * destructors are called before the space is used. The copy constructor is
 * used to place the element into this vector. It is allowable for the pos and
 * length to specify a replacement that overwrites existing elements and
 * creates new ones. If pos is greater than the length of the vector then
 * undefined behaviour results.  If pos is negative, then it is treated as an
 * offset relative to the length of the vector.
 */
template<class T, class Resize> void Vector<T, Resize>::
		replaceDup(long pos, const T &val, long len)
{
	long endPos, i;
	T *item;

	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	endPos = pos + len;

	/* Make sure we have enough space. */
	if ( endPos > BaseTable::tabLen ) {
		upResize( endPos );

		/* Delete any objects we need to delete. */
		item = BaseTable::data + pos;
		for ( i = pos; i < BaseTable::tabLen; i++, item++ )
			item->~T();
		
		/* We are extending the vector, set the new data length. */
		BaseTable::tabLen = endPos;
	}
	else {
		/* Delete any objects we need to delete. */
		item = BaseTable::data + pos;
		for ( i = pos; i < endPos; i++, item++ )
			item->~T();
	}

	/* Copy data in using copy constructor. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(val);
}

/** \fn Vector::replaceNew(long pos)
 * \brief Replace at position pos with one new element.
 *
 * If there is an existing element at the position to be replaced (pos is less
 * than the length of the vector) then the element's destructor is called
 * before the space is used. The default constructor is used to initialize the
 * new element. If pos is greater than the length of the vector then undefined
 * behaviour results. If pos is negative, then it is treated as an offset
 * relative to the length of the vector.
 */

/**
 * \brief Replace at position pos with len new elements.
 *
 * If there are existing elements at the positions to be replaced, then
 * destructors are called before the space is used. The default constructor is
 * used to initialize the new elements. It is allowable for the pos and length
 * to specify a replacement that overwrites existing elements and creates new
 * ones. If pos is greater than the length of the vector then undefined
 * behaviour results. If pos is negative, then it is treated as an offset
 * relative to the length of the vector.
 */
template<class T, class Resize> void Vector<T, Resize>::
		replaceNew(long pos, long len)
{
	long endPos, i;
	T *item;

	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	endPos = pos + len;

	/* Make sure we have enough space. */
	if ( endPos > BaseTable::tabLen ) {
		upResize( endPos );

		/* Delete any objects we need to delete. */
		item = BaseTable::data + pos;
		for ( i = pos; i < BaseTable::tabLen; i++, item++ )
			item->~T();
		
		/* We are extending the vector, set the new data length. */
		BaseTable::tabLen = endPos;
	}
	else {
		/* Delete any objects we need to delete. */
		item = BaseTable::data + pos;
		for ( i = pos; i < endPos; i++, item++ )
			item->~T();
	}

	/* Copy data in using copy constructor. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();
}

/** \fn Vector::remove(long pos)
 * \brief Remove one element at position pos.
 *
 * The element's destructor is called. Elements to the right of pos are
 * shifted one space to the left to take up the free space. If pos is greater
 * than or equal to the length of the vector then undefined behavior results.
 * If pos is negative then it is treated as an offset relative to the length
 * of the vector.
 */

/**
 * \brief Remove len elements at position pos.
 *
 * Destructor is called on all elements removed. Elements to the right of pos
 * are shifted len spaces to the left to take up the free space. If pos is
 * greater than or equal to the length of the vector then undefined behavior
 * results. If pos is negative then it is treated as an offset relative to the
 * length of the vector.
 */
template<class T, class Resize> void Vector<T, Resize>::
		remove(long pos, long len)
{
	long newLen, lenToSlideOver, endPos;
	T *dst, *item;

	/* If we are given a negative position to remove at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;

	/* The first position after the last item deleted. */
	endPos = pos + len;

	/* The new data length. */
	newLen = BaseTable::tabLen - len;

	/* The place in the data we are deleting at. */
	dst = BaseTable::data + pos;

	/* Call Destructors. */
	item = dst;
	for ( long i = 0; i < len; i += 1, item += 1 )
		item->~T();
	
	/* Shift data over if necessary. */
	lenToSlideOver = BaseTable::tabLen - endPos;	
	if ( len > 0 && lenToSlideOver > 0 )
		memmove(dst, dst + len, sizeof(T)*lenToSlideOver);

	/* Shrink the data if necessary. */
	downResize( newLen );

	/* Set the new data length. */
	BaseTable::tabLen = newLen;
}

/** \fn Vector::insert(long pos, const T &val)
 * \brief Insert one element at position pos.
 *
 * Elements in the vector from pos onward are shifted one space to the
 * right. The copy constructor is used to place the element into this
 * vector. If pos is greater than the length of the vector then undefined
 * behaviour results. If pos is negative then it is treated as an offset
 * relative to the length of the vector.
 */

/**
 * \brief Insert len elements at position pos.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * The copy constructor is used to place the elements into this vector. If pos
 * is greater than the length of the vector then undefined behaviour results.
 * If pos is negative then it is treated as an offset relative to the length
 * of the vector.
 */
template<class T, class Resize> void Vector<T, Resize>::
		insert(long pos, const T *val, long len)
{
	/* If we are given a negative position to insert at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;
	
	/* Calculate the new length. */
	long newLen = BaseTable::tabLen + len;

	/* Up resize, we are growing. */
	upResize( newLen );

	/* Shift over data at insert spot if needed. */
	if ( len > 0 && pos < BaseTable::tabLen ) {
		memmove(BaseTable::data + pos + len, BaseTable::data + pos,
				sizeof(T)*(BaseTable::tabLen-pos));
	}

	/* Copy data in element by element. */
	T *dst = BaseTable::data + pos;
	const T *src = val;
	for ( long i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);

	/* Set the new length. */
	BaseTable::tabLen = newLen;
}

/** \fn Vector::insert(long pos, const Vector &v)
 * \brief Insert all the elements from another vector at position pos.
 *
 * Elements in this vector from pos onward are shifted v.tabLen spaces to
 * the right. The element's copy constructor is used to copy the items into
 * this vector. The other vector is left unchanged. If pos is off the end of
 * the vector, then undefined behaviour results. If pos is negative then it is
 * treated as an offset relative to the length of the vector. Equivalent to
 * vector.insert(pos, other.data, other.tabLen).
 */

/**
 * \brief Insert len copies of item at position pos.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * The copy constructor is used to place the element into this vector. If pos
 * is greater than the length of the vector then undefined behaviour results.
 * If pos is negative then it is treated as an offset relative to the length
 * of the vector.
 */
template<class T, class Resize> void Vector<T, Resize>::
		insertDup(long pos, const T &item, long len)
{
	/* If we are given a negative position to insert at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;
	
	/* Calculate the new length. */
	long newLen = BaseTable::tabLen + len;

	/* Up resize, we are growing. */
	upResize( newLen );

	/* Shift over data at insert spot if needed. */
	if ( len > 0 && pos < BaseTable::tabLen ) {
		memmove(BaseTable::data + pos + len, BaseTable::data + pos,
				sizeof(T)*(BaseTable::tabLen-pos));
	}

	/* Copy the data item in one at a time. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(item);

	/* Set the new length. */
	BaseTable::tabLen = newLen;
}

/** \fn Vector::insertNew(long pos)
 * \brief Insert one new element using the default constrcutor.
 *
 * Elements in the vector from pos onward are shifted one space to the right.
 * The default constructor is used to init the new element. If pos is greater
 * than the length of the vector then undefined behaviour results. If pos is
 * negative then it is treated as an offset relative to the length of the
 * vector.
 */

/**
 * \brief Insert len new elements using the default constructor.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * Default constructors are used to init the new elements. If pos is off the
 * end of the vector then undefined behaviour results. If pos is negative then
 * it is treated as an offset relative to the length of the vector.
 */
template<class T, class Resize> void Vector<T, Resize>::
		insertNew(long pos, long len)
{
	/* If we are given a negative position to insert at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;
	
	/* Calculate the new length. */
	long newLen = BaseTable::tabLen + len;

	/* Up resize, we are growing. */
	upResize( newLen );

	/* Shift over data at insert spot if needed. */
	if ( len > 0 && pos < BaseTable::tabLen ) {
		memmove(BaseTable::data + pos + len, BaseTable::data + pos,
				sizeof(T)*(BaseTable::tabLen-pos));
	}

	/* Init new data with default constructors. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();

	/* Set the new length. */
	BaseTable::tabLen = newLen;
}

/* Makes space for len items, Does not init the items in any way.  If pos is
 * greater than the length of the vector then undefined behaviour results.
 * Updates the length of the vector. */
template<class T, class Resize> void Vector<T, Resize>::
		makeRawSpaceFor(long pos, long len)
{
	/* Calculate the new length. */
	long newLen = BaseTable::tabLen + len;

	/* Up resize, we are growing. */
	upResize( newLen );

	/* Shift over data at insert spot if needed. */
	if ( len > 0 && pos < BaseTable::tabLen ) {
		memmove(BaseTable::data + pos + len, BaseTable::data + pos,
			sizeof(T)*(BaseTable::tabLen-pos));
	}

	/* Save the new length. */
	BaseTable::tabLen = newLen;
}

/** \fn Vector::append(const T &val)
 * \brief Append one elment to the end of the vector.
 *
 * Copy constructor is used to place the element in the vector.
 */

/** \fn Vector::append(const T *val, long len)
 * \brief Append len elements to the end of the vector. 
 *
 * Copy constructors are used to place the elements in the vector. 
 */

/** \fn Vector::append(const Vector &v)
 * \brief Append the contents of another vector.
 *
 * The other vector is left unchanged. Copy constructors are used to place the
 * elements in the vector.
 */

/** \fn Vector::appendDup(const T &item, long len)
 * \brief Append len copies of item.
 *
 * The copy constructor is used to place the item in the vector.
 */
	
/** \fn Vector::appendNew()
 * \brief Append a single newly created item. 
 *
 * The new element is initialized with the default constructor.
 */

/** \fn Vector::appendNew(long len)
 * \brief Append len newly created items.
 *
 * The new elements are initialized with the default constructor.
 */



/** \fn Vector::prepend(const T &val)
 * \brief Prepend one elment to the front of the vector.
 *
 * Copy constructor is used to place the element in the vector.
 */

/** \fn Vector::prepend(const T *val, long len)
 * \brief Prepend len elements to the front of the vector. 
 *
 * Copy constructors are used to place the elements in the vector. 
 */

/** \fn Vector::prepend(const Vector &v)
 * \brief Prepend the contents of another vector.
 *
 * The other vector is left unchanged. Copy constructors are used to place the
 * elements in the vector.
 */

/** \fn Vector::prependDup(const T &item, long len)
 * \brief Prepend len copies of item.
 *
 * The copy constructor is used to place the item in the vector.
 */
	
/** \fn Vector::prependNew()
 * \brief Prepend a single newly created item. 
 *
 * The new element is initialized with the default constructor.
 */

/** \fn Vector::prependNew(long len)
 * \brief Prepend len newly created items.
 *
 * The new elements are initialized with the default constructor.
 */

#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_VECTOR_H */
