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

#ifndef _AAPL_SVECTOR_H
#define _AAPL_SVECTOR_H

#define VECT_COMPLEX
#include "svectcommon.h"
#undef VECT_COMPLEX

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \addtogroup vector
 * @{
 */

/** \class SVector
 * \brief Copy-on-write dynamic array for classes with non-trivial
 * initialization.
 *
 * SVector is a variant of Vector that employs copy-on-write behaviour. The
 * SVector copy constructor and = operator make shallow copies. If a vector
 * that references shared data is modified with insert, replace, append,
 * prepend, setAs or remove, a new copy is made so as not to interfere with
 * the shared data. However, shared individual elements may be modified by
 * bypassing the SVector interface.
 *
 * SVector is a dynamic array that can be used to contain complex data
 * structures that have constructors and destructors as well as simple types
 * such as integers and pointers.
 *
 * SVector supports inserting, overwriting, and removing single or multiple
 * elements at once. Constructors and destructors are called wherever
 * appropriate.  For example, before an element is overwritten, it's
 * destructor is called.
 *
 * SVector provides automatic resizing of allocated memory as needed and
 * offers different allocation schemes for controlling how the automatic
 * allocation is done.  Two senses of the the length of the data is
 * maintained: the amount of raw memory allocated to the vector and the number
 * of actual elements in the vector. The various allocation schemes control
 * how the allocated space is changed in relation to the number of elements in
 * the vector.
 */

/*@}*/

/** \fn SVector::SVector( long size )
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

/** \fn SVector::SVector( long size, long allocLen )
 * \brief Create a vector with a specified number of initial elements and
 * allocation.
 *
 * Default constructors are used to create the new elements. If the size is
 * greater than allocLen, then the space will be grown according to the
 * allocation scheme. Both size and allocLen are in units of T. If a linear
 * resizer is used, the step defaults to 256 units of T. For a runtime vector
 * both up and down allocation schemes default to Exponential.
 */

/** \fn SVector::~SVector()
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
template <class T, class Resize> void SVector<T, Resize>::
		empty()
{
	if ( BaseTable::data != 0 ) {
		/* Get the header and drop the refcount on the data. */
		STabHead *head = ((STabHead*) BaseTable::data) - 1;
		head->refCount -= 1;

		/* If the refcount just went down to zero nobody else is referencing
		 * the data. */
		if ( head->refCount == 0 ) {
			/* Call All destructors. */
			T *pos = BaseTable::data;
			for ( long i = 0; i < head->tabLen; pos++, i++ )
				pos->~T();

			/* Free the data space. */
			free( head );
		}

		/* Clear the pointer. */
		BaseTable::data = 0;
	}
}

/* Prepare for setting the contents of the vector to some array len long.
 * Handles reusing the existing space, detaching from a common space or
 * growing from zero length automatically. */
template <class T, class Resize> void SVector<T, Resize>::
		setAsCommon(long len)
{
	if ( BaseTable::data != 0 ) {
		/* Get the header. */
		STabHead *head = ((STabHead*)BaseTable::data) - 1;

		/* If the refCount is one, then we can reuse the space. Otherwise we
		 * must detach from the referenced data create new space. */
		if ( head->refCount == 1 ) {
			/* Call All destructors. */
			T *pos = BaseTable::data;
			for ( long i = 0; i < head->tabLen; pos++, i++ )
				pos->~T();

			/* Adjust the allocated length. */
			if ( len < head->tabLen )
				downResize( len );
			else if ( len > head->tabLen )
				upResize( len );

			if ( BaseTable::data != 0 ) {
				/* Get the header again and set the length. */
				head = ((STabHead*)BaseTable::data) - 1;
				head->tabLen = len;
			}
		}
		else {
			/* Just detach from the data. */
			head->refCount -= 1;
			BaseTable::data = 0;
			
			/* Make enough space. This will set the length. */
			upResizeFromEmpty( len );
		}
	}
	else {
		/* The table is currently empty. Make enough space. This will set the
		 * length. */
		upResizeFromEmpty( len );
	}
}

/** \fn SVector::setAs(const T &val)
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
template <class T, class Resize> void SVector<T, Resize>::
		setAs(const T *val, long len)
{
	/* Common stuff for setting the array to len long. */
	setAsCommon( len );

	/* Copy data in. */
	T *dst = BaseTable::data;
	const T *src = val;
	for ( long i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);
}


/** \fn SVector::setAs(const SVector<T, Resize> &v)
 * \brief Set the vector to exactly the contents of another vector.
 *
 * The vector becomes v.length() elements in length. Destructors are called
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
template <class T, class Resize> void SVector<T, Resize>::
		setAsDup(const T &item, long len)
{
	/* Do the common stuff for setting the array to len long. */
	setAsCommon( len );

	/* Copy item in one spot at a time. */
	T *dst = BaseTable::data;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(item);
}

/** \fn SVector::setAsNew()
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
template <class T, class Resize> void SVector<T, Resize>::
		setAsNew(long len)
{
	/* Do the common stuff for setting the array to len long. */
	setAsCommon( len );

	/* Create items using default constructor. */
	T *dst = BaseTable::data;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();
}

/* Make space in vector for a replacement at pos of len items. Handles reusing
 * existing space, detaching or growing from zero space. */
template <class T, class Resize> long SVector<T, Resize>::
		replaceCommon(long pos, long len)
{
	if ( BaseTable::data != 0 ) {
		/* Get the header. */
		STabHead *head = ((STabHead*)BaseTable::data) - 1;

		/* If we are given a negative position to replace at then treat it as
		 * a position relative to the length. This doesn't have any meaning
		 * unless the length is at least one. */
		if ( pos < 0 )
			pos = head->tabLen + pos;

		/* The end is the one past the last item that we want to write to. */
		long i, endPos = pos + len;

		if ( head->refCount == 1 ) {
			/* We can reuse the space. Make sure we have enough space. */
			if ( endPos > head->tabLen ) {
				upResize( endPos );

				/* Get the header again, whose addr may have changed after
				 * resizing. */
				head = ((STabHead*)BaseTable::data) - 1;

				/* Delete any objects we need to delete. */
				T *item = BaseTable::data + pos;
				for ( i = pos; i < head->tabLen; i++, item++ )
					item->~T();
		
				/* We are extending the vector, set the new data length. */
				head->tabLen = endPos;
			}
			else {
				/* Delete any objects we need to delete. */
				T *item = BaseTable::data + pos;
				for ( i = pos; i < endPos; i++, item++ )
					item->~T();
			}
		}
		else {
			/* Use endPos to calc the end of the vector. */
			long newLen = endPos;
			if ( newLen < head->tabLen )
				newLen = head->tabLen;

			/* Duplicate and grow up to endPos. This will set the length. */
			upResizeDup( newLen );

			/* Copy from src up to pos. */
			const T *src = (T*) (head + 1);
			T *dst = BaseTable::data;
			for ( i = 0; i < pos; i++, dst++, src++)
				new(dst) T(*src);

			/* Copy any items after the replace range. */
			for ( i += len, src += len, dst += len; 
					i < head->tabLen; i++, dst++, src++ )
				new(dst) T(*src);
		}
	}
	else {
		/* There is no data initially, must grow from zero. This will set the
		 * new length. */
		upResizeFromEmpty( len );
	}

	return pos;
}


/** \fn SVector::replace(long pos, const T &val)
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
template <class T, class Resize> void SVector<T, Resize>::
		replace(long pos, const T *val, long len)
{
	/* Common work for replacing in the vector. */
	pos = replaceCommon( pos, len );

	/* Copy data in using copy constructor. */
	T *dst = BaseTable::data + pos;
	const T *src = val;
	for ( long i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);
}

/** \fn SVector::replace(long pos, const SVector<T, Resize> &v)
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
template <class T, class Resize> void SVector<T, Resize>::
		replaceDup(long pos, const T &val, long len)
{
	/* Common replacement stuff. */
	pos = replaceCommon( pos, len );

	/* Copy data in using copy constructor. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(val);
}

/** \fn SVector::replaceNew(long pos)
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
template <class T, class Resize> void SVector<T, Resize>::
		replaceNew(long pos, long len)
{
	/* Do the common replacement stuff. */
	pos = replaceCommon( pos, len );

	/* Copy data in using copy constructor. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();
}

/** \fn SVector::remove(long pos)
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
template <class T, class Resize> void SVector<T, Resize>::
		remove(long pos, long len)
{
	/* If there is no data, we can't delete anything anyways. */
	if ( BaseTable::data != 0 ) {
		/* Get the header. */
		STabHead *head = ((STabHead*)BaseTable::data) - 1;

		/* If we are given a negative position to remove at then
		 * treat it as a position relative to the length. */
		if ( pos < 0 )
			pos = head->tabLen + pos;

		/* The first position after the last item deleted. */
		long endPos = pos + len;

		/* The New data length. */
		long i, newLen = head->tabLen - len;

		if ( head->refCount == 1 ) {
			/* We are the only ones using the data. We can reuse 
			 * the existing space. */

			/* The place in the data we are deleting at. */
			T *dst = BaseTable::data + pos;

			/* Call Destructors. */
			T *item = BaseTable::data + pos;
			for ( i = 0; i < len; i += 1, item += 1 )
				item->~T();

			/* Shift data over if necessary. */
			long lenToSlideOver = head->tabLen - endPos;	
			if ( len > 0 && lenToSlideOver > 0 )
				memmove(BaseTable::data + pos, dst + len, sizeof(T)*lenToSlideOver);

			/* Shrink the data if necessary. */
			downResize( newLen );

			if ( BaseTable::data != 0 ) {
				/* Get the header again (because of the resize) and set the
				 * new data length. */
				head = ((STabHead*)BaseTable::data) - 1;
				head->tabLen = newLen;
			}
		}
		else {
			/* Must detach from the common data. Just copy the non-deleted
			 * items from the common data. */

			/* Duplicate and grow down to newLen. This will set the length. */
			downResizeDup( newLen );

			/* Copy over just the non-deleted parts. */
			const T *src = (T*) (head + 1);
			T *dst = BaseTable::data;
			for ( i = 0; i < pos; i++, dst++, src++ )
				new(dst) T(*src);

			/* ... and the second half. */
			for ( i += len, src += len; i < head->tabLen; i++, src++, dst++ )
				new(dst) T(*src);
		}
	}
}

/* Shift over existing data. Handles reusing existing space, detaching or
 * growing from zero space. */
template <class T, class Resize> long SVector<T, Resize>::
		insertCommon(long pos, long len)
{
	if ( BaseTable::data != 0 ) {
		/* Get the header. */
		STabHead *head = ((STabHead*)BaseTable::data) - 1;

		/* If we are given a negative position to insert at then treat it as a
		 * position relative to the length. This only has meaning if there is
		 * existing data. */
		if ( pos < 0 )
			pos = head->tabLen + pos;

		/* Calculate the new length. */
		long i, newLen = head->tabLen + len;

		if ( head->refCount == 1 ) {
			/* Up resize, we are growing. */
			upResize( newLen );

			/* Get the header again, (the addr may have changed after
			 * resizing). */
			head = ((STabHead*)BaseTable::data) - 1;

			/* Shift over data at insert spot if needed. */
			if ( len > 0 && pos < head->tabLen ) {
				memmove( BaseTable::data + pos + len, BaseTable::data + pos,
						sizeof(T)*(head->tabLen - pos) );
			}

			/* Grow the length by the len inserted. */
			head->tabLen += len;
		}
		else {
			/* Need to detach from the existing array. Copy over the other
			 * parts. This will set the length. */
			upResizeDup( newLen );

			/* Copy over the parts around the insert. */
			const T *src = (T*) (head + 1);
			T *dst = BaseTable::data;
			for ( i = 0; i < pos; i++, dst++, src++ )
				new(dst) T(*src);

			/* ... and the second half. */
			for ( dst += len; i < head->tabLen; i++, src++, dst++ )
				new(dst) T(*src);
		}
	}
	else {
		/* There is no existing data. Start from zero. This will set the
		 * length. */
		upResizeFromEmpty( len );
	}

	return pos;
}


/** \fn SVector::insert(long pos, const T &val)
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
template <class T, class Resize> void SVector<T, Resize>::
		insert(long pos, const T *val, long len)
{
	/* Do the common insertion stuff. */
	pos = insertCommon( pos, len );

	/* Copy data in element by element. */
	T *dst = BaseTable::data + pos;
	const T *src = val;
	for ( long i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);
}

/** \fn SVector::insert(long pos, const SVector &v)
 * \brief Insert all the elements from another vector at position pos.
 *
 * Elements in this vector from pos onward are shifted v.length() spaces to
 * the right. The element's copy constructor is used to copy the items into
 * this vector. The other vector is left unchanged. If pos is off the end of
 * the vector, then undefined behaviour results. If pos is negative then it is
 * treated as an offset relative to the length of the vector. Equivalent to
 * vector.insert(pos, other.data, other.length()).
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
template <class T, class Resize> void SVector<T, Resize>::
		insertDup(long pos, const T &item, long len)
{
	/* Do the common insertion stuff. */
	pos = insertCommon( pos, len );

	/* Copy the data item in one at a time. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(item);
}


/** \fn SVector::insertNew(long pos)
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
template <class T, class Resize> void SVector<T, Resize>::
		insertNew(long pos, long len)
{
	/* Do the common insertion stuff. */
	pos = insertCommon( pos, len );

	/* Init new data with default constructors. */
	T *dst = BaseTable::data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();
}

/* Makes space for len items, Does not init the items in any way.  If pos is
 * greater than the length of the vector then undefined behaviour results.
 * Updates the length of the vector. */
template <class T, class Resize> void SVector<T, Resize>::
		makeRawSpaceFor(long pos, long len)
{
	if ( BaseTable::data != 0 ) {
		/* Get the header. */
		STabHead *head = ((STabHead*)BaseTable::data) - 1;

		/* Calculate the new length. */
		long i, newLen = head->tabLen + len;

		if ( head->refCount == 1 ) {
			/* Up resize, we are growing. */
			upResize( newLen );

			/* Get the header again, (the addr may have changed after
			 * resizing). */
			head = ((STabHead*)BaseTable::data) - 1;

			/* Shift over data at insert spot if needed. */
			if ( len > 0 && pos < head->tabLen ) {
				memmove( BaseTable::data + pos + len, BaseTable::data + pos,
						sizeof(T)*(head->tabLen - pos) );
			}

			/* Grow the length by the len inserted. */
			head->tabLen += len;
		}
		else {
			/* Need to detach from the existing array. Copy over the other
			 * parts. This will set the length. */
			upResizeDup( newLen );

			/* Copy over the parts around the insert. */
			const T *src = (T*) (head + 1);
			T *dst = BaseTable::data;
			for ( i = 0; i < pos; i++, dst++, src++ )
				new(dst) T(*src);

			/* ... and the second half. */
			for ( dst += len; i < head->tabLen; i++, src++, dst++ )
				new(dst) T(*src);
		}
	}
	else {
		/* There is no existing data. Start from zero. This will set the
		 * length. */
		upResizeFromEmpty( len );
	}
}

/** \fn SVector::append(const T &val)
 * \brief Append one elment to the end of the vector.
 *
 * Copy constructor is used to place the element in the vector.
 */

/** \fn SVector::append(const T *val, long len)
 * \brief Append len elements to the end of the vector. 
 *
 * Copy constructors are used to place the elements in the vector. 
 */

/** \fn SVector::append(const SVector &v)
 * \brief Append the contents of another vector.
 *
 * The other vector is left unchanged. Copy constructors are used to place the
 * elements in the vector.
 */

/** \fn SVector::appendDup(const T &item, long len)
 * \brief Append len copies of item.
 *
 * The copy constructor is used to place the item in the vector.
 */
	
/** \fn SVector::appendNew()
 * \brief Append a single newly created item. 
 *
 * The new element is initialized with the default constructor.
 */

/** \fn SVector::appendNew(long len)
 * \brief Append len newly created items.
 *
 * The new elements are initialized with the default constructor.
 */



/** \fn SVector::prepend(const T &val)
 * \brief Prepend one elment to the front of the vector.
 *
 * Copy constructor is used to place the element in the vector.
 */

/** \fn SVector::prepend(const T *val, long len)
 * \brief Prepend len elements to the front of the vector. 
 *
 * Copy constructors are used to place the elements in the vector. 
 */

/** \fn SVector::prepend(const SVector &v)
 * \brief Prepend the contents of another vector.
 *
 * The other vector is left unchanged. Copy constructors are used to place the
 * elements in the vector.
 */

/** \fn SVector::prependDup(const T &item, long len)
 * \brief Prepend len copies of item.
 *
 * The copy constructor is used to place the item in the vector.
 */
	
/** \fn SVector::prependNew()
 * \brief Prepend a single newly created item. 
 *
 * The new element is initialized with the default constructor.
 */

/** \fn SVector::prependNew(long len)
 * \brief Prepend len newly created items.
 *
 * The new elements are initialized with the default constructor.
 */

#ifdef AAPL_NAMESPACE
}
#endif


#endif /* _AAPL_SVECTOR_H */
