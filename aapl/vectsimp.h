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

#ifndef _AAPL_VECTSIMP_H
#define _AAPL_VECTSIMP_H

#define Vector VectSimp
#include "vectcommon.h"
#undef Vector

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \addtogroup vector
 * @{
 */

/** \class VectSimp
 * \brief Dynamic array for classes with trivial initialization.
 *
 * VectSimp is a dynamic array that can be used only to contain simple data
 * types that do not have constructors and destructors such as integers and
 * pointers. The trivial initialization allows the use of memcpy for putting
 * data into the vector and consequently VectSimp offers a significant
 * performance increase over Vector.
 *
 * VectSimp supports inserting, overwriting and removing single or multiple
 * elements at once. 
 *
 * VectSimp provides automatic resizing of allocated memory as needed and
 * offers different allocation schemes for controlling how the automatic
 * allocation is done.  Two senses of the the length of the data is
 * maintained: the amount of raw memory allocated to the vector and the number
 * of actual elements in the vector. The various allocation schemes control
 * how the allocated space is changed in relation to the number of elements in
 * the vector.
 */

/*@}*/

/** \fn VectSimp::VectSimp( long size )
 * \brief Create a vector with a specified number of initial elements.
 *
 * The new elements are not initialized. The initial space allocation is zero
 * and it is grown according to the up allocation scheme used.  If a linear
 * resizer is used, the step defaults to 256 units of T. For a runtime vector
 * both up and down allocation schemes default to Exponential.
 *
 * This constructor is not of any use to a vector with a constant up resizing
 * scheme as it starts with no space allocated and is unable to grow. It will
 * assertion fail with a non-zero size value.
 *
 */

/** \fn VectSimp::VectSimp( long size, long allocLen )
 * \brief Create a vector with a specified number of initial elements and
 * allocation.
 *
 * The new elements are not initialized. If the size is greater than
 * allocLen, then the space will be grown according to the allocation
 * scheme. Both size and allocLen are in units of T. If a linear resizer
 * is used, the step defaults to 256 units of T. For a runtime vector both up
 * and down allocation schemes default to Exponential.
 */

/**
 * \brief Perform a deep copy of the vector.
 *
 * The contents of the other vector are copied into this vector. This vector
 * gets the same allocation size as the other vector.  Memcpy is used to put
 * data into this vector.
 */
template<class T, class Resize> VectSimp<T, Resize>::
		VectSimp(const VectSimp<T, Resize> &b)
{
	BaseTable::tabLen = b.tabLen;
	BaseTable::allocLen = b.allocLen;

	if ( BaseTable::allocLen > 0 ) {
		/* Alloc the space for the data. */
		BaseTable::data = (T*) malloc(sizeof(T)*BaseTable::allocLen);
		if ( BaseTable::data == NULL )
			throw std::bad_alloc();

		/* Copy the data in. */
		memcpy( BaseTable::data, b.data, sizeof(T)*BaseTable::tabLen );
	}
	else {
		/* Noting is allocated. */
		BaseTable::data = NULL;
	}
}

/** \fn VectSimp::~VectSimp()
 * \brief Free all memory used by the vector. 
 *
 * The vector is reset to zero elements. The space allocated for the vector is
 * freed.
 */

/**
 * \brief Free all memory used by the vector. 
 *
 * The vector is reset to zero elements. The space allocated for the vector is
 * freed.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
		empty()
{
	if ( BaseTable::data != NULL ) {
		/* Free the data space. */
		free( BaseTable::data );
		BaseTable::data = NULL;
		BaseTable::tabLen = BaseTable::allocLen = 0;
	}
}

/** \fn VectSimp::setAs(const T &val)
 * \brief Set the contents of the vector to be val exactly.
 *
 * The vector becomes one element in length. Existing elements not
 * uninitialized. Memcpy is used to place the new item in the vector.
 */

/**
 * \brief Set the contents of the vector to be len elements exactly. 
 *
 * The vector becomes len elements in length. Any existing elements are not
 * uninitialzed. Memcpy is used to place the new elements in the vector. 
 */
template<class T, class Resize> void VectSimp<T, Resize>::
		setAs(const T *val, long len)
{
	/* Adjust the allocated length. */
	if ( len < BaseTable::tabLen )
		downResize(len);
	else if ( len > BaseTable::tabLen )
		upResize(len);

	/* Set the new data length to exactly len. */
	BaseTable::tabLen = len;
	
	/* Copy data in. */
	memcpy( BaseTable::data, val, sizeof(T)*len );
}

/** \fn VectSimp::setAs(const VectSimp<T, Resize> &v)
 * \brief Set the vector to exactly the contents of another vector.
 *
 * The vector becomes v.tabLen elements in length. Any existing elements
 * are not uninitialized. Memcpy is used to place the new elements in the
 * vector.
 */

/**
 * \brief Set the vector to len copies of item.
 *
 * The vector becomes len elements in length. Any existing elements are not
 * uninitialized. The assignment operator is used to copy the item into the
 * vector.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
		setAsDup(const T &item, long len)
{
	/* Adjust the allocated length. */
	if ( len < BaseTable::tabLen )
		downResize(len);
	else if ( len > BaseTable::tabLen )
		upResize(len);

	/* Set the new data length to exactly len. */
	BaseTable::tabLen = len;
	
	/* Copy the data item in one at a time. */
	T *dst = BaseTable::data;
	for ( long i = 0; i < len; i++, dst++ )
		*dst = item;
}

/** \fn VectSimp::setAsNew()
 * \brief Set the vector to exactly one new item.
 *
 * The vector becomes one element in length. Any existing elements are not
 * uninitialized. The new item is not initialized.
 */

/**
 * \brief Set the vector to exactly len new items.
 *
 * The vector becomes len elements in length. Any existing elements are not
 * uninitialized. The new items are not initialized.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
		setAsNew(long len)
{
	/* Adjust the allocated length. */
	if ( len < BaseTable::tabLen )
		downResize(len);
	else if ( len > BaseTable::tabLen )
		upResize(len);

	/* Set the new data length to exactly len. */
	BaseTable::tabLen = len;
}

/** \fn VectSimp::replace(long pos, const T &val)
 * \brief Replace one element at position pos.
 *
 * If there is an existing element at position pos (if pos is less than the
 * length of the vector) then it is not uninitialized in any way.  Memcpy is
 * used to place the new element into the vector. If pos is greater than the
 * length of the vector then undefined behaviour results. If pos is negative
 * then it is treated as an offset relative to the length of the vector.
 */

/**
 * \brief Replace len elements at position pos.
 *
 * If there are existing elements at the positions to be replaced, then they
 * are not uninitialized in any way. Memcpy is used to place the new elements
 * into the vector. It is allowable for the pos and length to specify a
 * replacement that overwrites existing elements and creates new ones. If pos
 * is greater than the length of the vector then undefined behaviour results.
 * If pos is negative, then it is treated as an offset relative to the length
 * of the vector.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
		replace(long pos, const T *val, long len)
{
	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	long endPos = pos + len;

	/* Make sure we have enough space. */
	if ( endPos > BaseTable::tabLen ) {
		upResize( endPos );

		/* Set the new length. */
		BaseTable::tabLen = endPos;
	}
	
	/* Copy data in. */
	memcpy( BaseTable::data + pos, val, sizeof(T)*len );
}

/** \fn VectSimp::replace(long pos, const VectSimp<T, Resize> &v)
 * \brief Replace at position pos with all the elements of another vector.
 *
 * Replace at position pos with all the elements of another vector. The other
 * vector is left unchanged. If there are existing elements at the positions
 * to be replaced then they are not uninitialized in any way. Memcpy is used
 * to place the elements into this vector. It is allowable for the pos and
 * length of the other vector to specify a replacement that overwrites
 * existing elements and creates new ones. If pos is greater than the length
 * of the vector then undefined behaviour results. If pos is negative, then it
 * is treated as an offset relative to the length of the vector.
 */

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
template<class T, class Resize> void VectSimp<T, Resize>::
		replaceDup(long pos, const T &item, long len)
{
	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	long endPos = pos + len;

	/* Make sure we have enough space. */
	if ( endPos > BaseTable::tabLen ) {
		upResize( endPos );

		/* Set the new length. */
		BaseTable::tabLen = endPos;
	}
	
	/* Copy the data item in one at a time. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		*dst = item;
}

/** \fn VectSimp::replaceNew(long pos)
 * \brief Replace at position pos with one new element.
 *
 * If there is an existing element at the position to be replaced (pos is less
 * than the length of the vector) then the element is not uninitialized in any
 * way. The new item is not initialized. If pos is greater than the length of
 * the vector then undefined behaviour results. If pos is negative, then it is
 * treated as an offset relative to the length of the vector.
 */

/**
 * \brief Replace at position pos with len new elements.
 *
 * If there are existing elements at the positions to be replaced then the
 * elements are not uninitialized in any way. The new items are not
 * initialized. It is allowable for the pos and length to specify a
 * replacement that overwrites existing elements and creates new ones. If pos
 * is greater than the length of the vector then undefined behaviour results.
 * If pos is negative, then it is treated as an offset relative to the length
 * of the vector.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
		replaceNew(long pos, long len)
{
	/* If we are given a negative position to replace at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;

	/* The end is the one past the last item that we want
	 * to write to. */
	long endPos = pos + len;

	/* Make sure we have enough space. */
	if ( endPos > BaseTable::tabLen ) {
		upResize( endPos );

		/* Set the new length. */
		BaseTable::tabLen = endPos;
	}
}

/** \fn VectSimp::remove(long pos)
 * \brief Remove one element at position pos.
 *
 * The space used by the element is not uninitialized in any way. Elements to
 * the right of pos are shifted one space to the left to take up the free
 * space. If pos is greater than or equal to the length of the vector then
 * undefined behavior results. If pos is negative then it is treated as an
 * offset relative to the length of the vector.
 */

/**
 * \brief Remove len elements at position pos.
 *
 * The space used by the elements is not uninitialized in any way. Elements to
 * the right of pos are shifted len spaces to the left to take up the free
 * space. If pos is greater than or equal to the length of the vector then
 * undefined behavior results. If pos is negative then it is treated as an
 * offset relative to the length of the vector.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
		remove(long pos, long len)
{
	long newLen, lenToSlideOver, endPos;

	/* If we are given a negative position to remove at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = BaseTable::tabLen + pos;

	/* The first position after the last item deleted. */
	endPos = pos + len;

	/* The new data length. */
	newLen = BaseTable::tabLen - len;

	/* The place in the data we are deleting at. */
	T *dst = BaseTable::data + pos;

	/* Shift data over if necessary. */
	lenToSlideOver = BaseTable::tabLen - endPos;	
	if ( len > 0 && lenToSlideOver > 0 )
		memmove(dst, dst + len, sizeof(T)*lenToSlideOver);

	/* Shrink the data if necessary. */
	downResize( newLen );

	/* Set the new data length. */
	BaseTable::tabLen = newLen;
}

/** \fn VectSimp::insert(long pos, const T &val)
 * \brief Insert one element at position pos.
 *
 * Elements in the vector from pos onward are shifted one space to the right.
 * Memcop is used to place the element into the vector. If pos is greater than
 * the length of the vector then undefined behaviour results. If pos is
 * negative then it is treated as an offset relative to the length of the
 * vector.
 */


/**
 * \brief Insert len elements at position pos.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * Memcpy is used to place the elements in the vector. If pos is greater than
 * the length of the vector then undefined behaviour results. If pos is
 * negative then it is treated as an offset relative to the length of the
 * vector.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
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

	/* Copy data in. */
	memcpy( BaseTable::data + pos, val, sizeof(T)*len );

	/* Set the new length. */
	BaseTable::tabLen = newLen;
}

/** \fn VectSimp::insert(long pos, const VectSimp &v)
 * \brief Insert all the elements from another vector at position pos.
 *
 * Elements in this vector from pos onward are shifted v.tabLen spaces to
 * the right. Memcpy is used to copy the elements into this vector. The other
 * vector is left unchanged. If pos is off the end of the vector, then
 * undefined behaviour results. If pos is negative then it is treated as an
 * offset relative to the length of the vector. Equivalent to
 * vector.insert(pos, other.data, other.tabLen).
 */


/**
 * \brief Insert len copies of item at position pos.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * The assignment operator is used to place the element into vector. If pos is
 * greater than the length of the vector then undefined behaviour results. If
 * pos is negative then it is treated as an offset relative to the length of
 * the vector.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
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
		*dst = item;

	/* Set the new length. */
	BaseTable::tabLen = newLen;
}


/** \fn VectSimp::insertNew(long pos)
 * \brief Insert one new element using the default constrcutor.
 *
 * Elements in the vector from pos onward are shifted one space to the right.
 * The new element is not initialized. If pos is greater than the length of
 * the vector then undefined behaviour results. If pos is negative then it is
 * treated as an offset relative to the length of the vector.
 */

/**
 * \brief Insert len new elements using the default constrcutor.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * The new items are not initialized. If pos is greater than the length of the
 * vector then undefined behaviour results. If pos is negative then it is
 * treated as an offset relative to the length of the vector.
 */
template<class T, class Resize> void VectSimp<T, Resize>::
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

	/* Set the new length. */
	BaseTable::tabLen = newLen;
}


/** \fn VectSimp::append(const T &val)
 * \brief Append one elment to the end of the vector.
 *
 * Memcpy is used to place the element in the vector.
 */

/** \fn VectSimp::append(const T *val, long len)
 * \brief Append len elements to the end of the vector. 
 *
 * Memcpy is used to place the elements in the vector. 
 */

/** \fn VectSimp::append(const VectSimp &v)
 * \brief Append the contents of another vector.
 *
 * The other vector is left unchanged. Memcpy is used to place the elements in
 * the vector.
 */

/** \fn VectSimp::appendDup(const T &item, long len)
 * \brief Append len copies of item.
 *
 * The assignment operator is used to place the item in the vector.
 */
	
/** \fn VectSimp::appendNew()
 * \brief Append a single newly created item. 
 *
 * The new element is not initialized.
 */

/** \fn VectSimp::appendNew(long len)
 * \brief Append len newly created items.
 *
 * The new elements are not initialized.
 */


/** \fn VectSimp::prepend(const T &val)
 * \brief Prepend one elment to the front of the vector.
 *
 * Memcpy is used to place the element in the vector.
 */

/** \fn VectSimp::prepend(const T *val, long len)
 * \brief Prepend len elements to the front of the vector. 
 *
 * Memcpy is used to place the elements in the vector. 
 */

/** \fn VectSimp::prepend(const VectSimp &v)
 * \brief Prepend the contents of another vector.
 *
 * The other vector is left unchanged. Memcpy is used to place the elements in
 * the vector.
 */

/** \fn VectSimp::prependDup(const T &item, long len)
 * \brief Prepend len copies of item.
 *
 * The assignment operator is used to place the item in the vector.
 */
	
/** \fn VectSimp::prependNew()
 * \brief Prepend a single newly created item. 
 *
 * The new element is not initialized.
 */

/** \fn VectSimp::prependNew(long len)
 * \brief Prepend len newly created items.
 *
 * The new elements are not initialized.
 */

#ifdef AAPL_NAMESPACE
}
#endif


#endif /* _AAPL_VECTSIMP_H */
