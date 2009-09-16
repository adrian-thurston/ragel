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


/* Default shared table header class. */
struct RtSTabHead
{
	/**
	 * \brief Table length.
	 *
	 * The number of items of type T in the table.
	 */
	long tabLen;

	/**
	 * \brief Allocated length.
	 *
	 * The number of items for which there is room in the current allocation.
	 */
	long allocLen;

	/**
	 * \brief Ref Count.
	 *
	 * The number of shared vectors referencing this data.
	 */
	long refCount;
};


/* STable class. */
template <class T> class RtSTable
{
public:
	/* Default Constructor. */
	inline RtSTable();

	/**
	 * \brief Get the length of the shared vector.
	 *
	 * \returns the length of the shared vector.
	 */
	long length() const
		{ return data == 0 ? 0 : (((RtSTabHead*)data) - 1)->tabLen; }
	
	/**
	 * \brief Get header of the shared vector.
	 *
	 * \returns the header of the shared vector.
	 */
	RtSTabHead *header() const
		{ return data == 0 ? 0 : (((RtSTabHead*)data) - 1); }

	/**
	 * \brief Table data.
	 *
	 * The pointer to the elements in the vector. The shared table header is
	 * located just behind the data. Modifying the vector may cause this
	 * pointer to change.
	 */
	T *data;
};

/**
 * \brief Default constructor
 *
 * Initialize shared table data to empty.
 */
template <class T> inline RtSTable<T>::RtSTable()
:
	data(0)
{
}

/* If needed is greater than existing, give twice needed. */
#define EXPN_UP( existing, needed ) \
		needed > existing ? (needed<<1) : existing
	
/* If needed is less than 1 quarter existing, give twice needed. */
#define EXPN_DOWN( existing, needed ) \
		needed < (existing>>2) ? (needed<<1) : existing

template < class T > class RtVector
{
public:
	static inline long upResize( long existing, long needed )
		{ return EXPN_UP( existing, needed ); }
	static inline long downResize( long existing, long needed )
		{ return EXPN_DOWN( existing, needed ); }

#undef EXPN_UP
#undef EXPN_DOWN

	long length() const
		{ return tabLen; }

	T *data;
	long tabLen;
	long allocLen;

	/**
	 * \brief Initialize an empty vector with no space allocated.  
	 *
	 * If a linear resizer is used, the step defaults to 256 units of T. For a
	 * runtime vector both up and down allocation schemes default to
	 * Exponential.
	 */
	RtVector() :
		data(0),
		tabLen(0),
		allocLen(0)
	
	{ }

	/* Free all mem used by the vector. */
	~RtVector() { empty(); }

	/* Delete all items. */
	void empty();

	/* Abandon the contents of the vector without deleteing. */
	void abandon();

	/* Transfers the elements of another vector into this vector. First emptys
	 * the current vector. */
	void transfer( RtVector &v );

	/* Perform a deep copy of another vector into this vector. */
	RtVector &operator=( const RtVector &v );

	/* Stack operations. */
	void push( const T &t ) { append( t ); }
	void pop() { remove( tabLen - 1 ); }
	T &top() { return data[tabLen - 1]; }

	/*@{*/
	/**
	 * \brief Insert one element at position pos.
	 *
	 * Elements in the vector from pos onward are shifted one space to the
	 * right. The copy constructor is used to place the element into this
	 * vector. If pos is greater than the length of the vector then undefined
	 * behaviour results. If pos is negative then it is treated as an offset
	 * relative to the length of the vector.
	 */
	void insert(long pos, const T &val)    { insert(pos, &val, 1); }

	/* Insert an array of values. */
	void insert(long pos, const T *val, long len);

	/**
	 * \brief Insert all the elements from another vector at position pos.
	 *
	 * Elements in this vector from pos onward are shifted v.tabLen spaces to
	 * the right. The element's copy constructor is used to copy the items
	 * into this vector. The other vector is left unchanged. If pos is off the
	 * end of the vector, then undefined behaviour results. If pos is negative
	 * then it is treated as an offset relative to the length of the vector.
	 * Equivalent to vector.insert(pos, other.data, other.tabLen).
	 */
	void insert(long pos, const RtVector &v) { insert(pos, v.data, v.tabLen); }

	/* Insert len copies of val into the vector. */
	void insertDup(long pos, const T &val, long len);

	/**
	 * \brief Insert one new element using the default constrcutor.
	 *
	 * Elements in the vector from pos onward are shifted one space to the
	 * right.  The default constructor is used to init the new element. If pos
	 * is greater than the length of the vector then undefined behaviour
	 * results. If pos is negative then it is treated as an offset relative to
	 * the length of the vector.
	 */
	void insertNew(long pos)               { insertNew(pos, 1); }

	/* Insert len new items using default constructor. */
	void insertNew(long pos, long len);
	/*@}*/

	/*@{*/
	/**
	 * \brief Remove one element at position pos.
	 *
	 * The element's destructor is called. Elements to the right of pos are
	 * shifted one space to the left to take up the free space. If pos is greater
	 * than or equal to the length of the vector then undefined behavior results.
	 * If pos is negative then it is treated as an offset relative to the length
	 * of the vector.
	 */
	void remove(long pos)                 { remove(pos, 1); }

	/* Delete a number of elements. */
	void remove(long pos, long len);
	/*@}*/

	/*@{*/
	/**
	 * \brief Replace one element at position pos.
	 *
	 * If there is an existing element at position pos (if pos is less than
	 * the length of the vector) then its destructor is called before the
	 * space is used. The copy constructor is used to place the element into
	 * the vector.  If pos is greater than the length of the vector then
	 * undefined behaviour results.  If pos is negative then it is treated as
	 * an offset relative to the length of the vector.
	 */
	void replace(long pos, const T &val)    { replace(pos, &val, 1); }

	/* Replace with an array of values. */
	void replace(long pos, const T *val, long len);

	/**
	 * \brief Replace at position pos with all the elements of another vector.
	 *
	 * Replace at position pos with all the elements of another vector. The
	 * other vector is left unchanged. If there are existing elements at the
	 * positions to be replaced, then destructors are called before the space
	 * is used. Copy constructors are used to place the elements into this
	 * vector. It is allowable for the pos and length of the other vector to
	 * specify a replacement that overwrites existing elements and creates new
	 * ones.  If pos is greater than the length of the vector then undefined
	 * behaviour results.  If pos is negative, then it is treated as an offset
	 * relative to the length of the vector.
	 */
	void replace(long pos, const RtVector &v) { replace(pos, v.data, v.tabLen); }

	/* Replace len items with len copies of val. */
	void replaceDup(long pos, const T &val, long len);

	/**
	 * \brief Replace at position pos with one new element.
	 *
	 * If there is an existing element at the position to be replaced (pos is
	 * less than the length of the vector) then the element's destructor is
	 * called before the space is used. The default constructor is used to
	 * initialize the new element. If pos is greater than the length of the
	 * vector then undefined behaviour results. If pos is negative, then it is
	 * treated as an offset relative to the length of the vector.
	 */
	void replaceNew(long pos)               { replaceNew(pos, 1); }

	/* Replace len items at pos with newly constructed objects. */
	void replaceNew(long pos, long len);
	/*@}*/

	/*@{*/
	/**
	 * \brief Set the contents of the vector to be val exactly.
	 *
	 * The vector becomes one element in length. Destructors are called on any
	 * existing elements in the vector. The element's copy constructor is used
	 * to place the val in the vector.
	 */
	void setAs(const T &val)             { setAs(&val, 1); }

	/* Set to the contents of an array. */
	void setAs(const T *val, long len);

	/**
	 * \brief Set the vector to exactly the contents of another vector.
	 *
	 * The vector becomes v.tabLen elements in length. Destructors are called
	 * on any existing elements. Copy constructors are used to place the new
	 * elements in the vector.
	 */
	void setAs(const RtVector &v)          { setAs(v.data, v.tabLen); }

	/* Set as len copies of item. */
	void setAsDup(const T &item, long len);

	/**
	 * \brief Set the vector to exactly one new item.
	 *
	 * The vector becomes one element in length. Destructors are called on any
	 * existing elements in the vector. The default constructor is used to
	 * init the new item.
	 */
	void setAsNew()                      { setAsNew(1); }

	/* Set as newly constructed objects using the default constructor. */
	void setAsNew(long len);
	/*@}*/

	/*@{*/
	/** 
	 * \brief Append one elment to the end of the vector.
	 *
	 * Copy constructor is used to place the element in the vector.
	 */
	void append(const T &val)                { replace(tabLen, &val, 1); }

	/**
	 * \brief Append len elements to the end of the vector. 
	 *
	 * Copy constructors are used to place the elements in the vector. 
	 */
	void append(const T *val, long len)       { replace(tabLen, val, len); }

	/**
	 * \brief Append the contents of another vector.
	 *
	 * The other vector is left unchanged. Copy constructors are used to place the
	 * elements in the vector.
	 */
	void append(const RtVector &v)             { replace(tabLen, v.data, v.tabLen); }

	/**
	 * \brief Append len copies of item.
	 *
	 * The copy constructor is used to place the item in the vector.
	 */
	void appendDup(const T &item, long len)   { replaceDup(tabLen, item, len); }

	/**
	 * \brief Append a single newly created item. 
	 *
	 * The new element is initialized with the default constructor.
	 */
	void appendNew()                         { replaceNew(tabLen, 1); }

	/**
	 * \brief Append len newly created items.
	 *
	 * The new elements are initialized with the default constructor.
	 */
	void appendNew(long len)                  { replaceNew(tabLen, len); }
	/*@}*/
	
	/*@{*/
	/** \fn RtVector::prepend(const T &val)
	 * \brief Prepend one elment to the front of the vector.
	 *
	 * Copy constructor is used to place the element in the vector.
	 */
	void prepend(const T &val)               { insert(0, &val, 1); }

	/**
	 * \brief Prepend len elements to the front of the vector. 
	 *
	 * Copy constructors are used to place the elements in the vector. 
	 */
	void prepend(const T *val, long len)      { insert(0, val, len); }

	/**
	 * \brief Prepend the contents of another vector.
	 *
	 * The other vector is left unchanged. Copy constructors are used to place the
	 * elements in the vector.
	 */
	void prepend(const RtVector &v)            { insert(0, v.data, v.tabLen); }

	/**
	 * \brief Prepend len copies of item.
	 *
	 * The copy constructor is used to place the item in the vector.
	 */
	void prependDup(const T &item, long len)  { insertDup(0, item, len); }

	/**
	 * \brief Prepend a single newly created item. 
	 *
	 * The new element is initialized with the default constructor.
	 */
	void prependNew()                        { insertNew(0, 1); }

	/**
	 * \brief Prepend len newly created items.
	 *
	 * The new elements are initialized with the default constructor.
	 */
	void prependNew(long len)                 { insertNew(0, len); }
	/*@}*/

	/* Convenience access. */
	long size() const           { return tabLen; }

protected:
	void upResize(long len);
	void downResize(long len);
};


template<class T> void RtVector<T>::
		abandon()
{
	data = 0;
	tabLen = 0;
	allocLen = 0;
}

template<class T> void RtVector<T>::
		transfer( RtVector &v )
{
	empty();

	data = v.data;
	tabLen = v.tabLen;
	allocLen = v.allocLen;

	v.abandon();
}

/**
 * \brief Deep copy another vector into this vector.
 *
 * Copies the entire contents of the other vector into this vector. Any
 * existing contents are first deleted. Equivalent to setAs.
 *
 * \returns A reference to this.
 */
template<class T> RtVector<T> &RtVector<T>::
		operator=( const RtVector &v )
{
	setAs(v.data, v.tabLen); 
	return *this;
}

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

/**
 * \brief Set the contents of the vector to be len elements exactly. 
 *
 * The vector becomes len elements in length. Destructors are called on any
 * existing elements in the vector. Copy constructors are used to place the
 * new elements in the vector. 
 */
template<class T> void RtVector<T>::
		setAs(const T *val, long len)
{
	/* Call All destructors. */
	long i;
	T *pos = data;
	for ( i = 0; i < tabLen; pos++, i++ )
		pos->~T();

	/* Adjust the allocated length. */
	if ( len < tabLen )
		downResize( len );
	else if ( len > tabLen )
		upResize( len );

	/* Set the new data length to exactly len. */
	tabLen = len;	
	
	/* Copy data in. */
	T *dst = data;
	const T *src = val;
	for ( i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);
}

/**
 * \brief Set the vector to len copies of item.
 *
 * The vector becomes len elements in length. Destructors are called on any
 * existing elements in the vector. The element's copy constructor is used to
 * copy the item into the vector.
 */
template<class T> void RtVector<T>::
		setAsDup(const T &item, long len)
{
	/* Call All destructors. */
	T *pos = data;
	for ( long i = 0; i < tabLen; pos++, i++ )
		pos->~T();

	/* Adjust the allocated length. */
	if ( len < tabLen )
		downResize( len );
	else if ( len > tabLen )
		upResize( len );

	/* Set the new data length to exactly len. */
	tabLen = len;	
	
	/* Copy item in one spot at a time. */
	T *dst = data;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(item);
}

/**
 * \brief Set the vector to exactly len new items.
 *
 * The vector becomes len elements in length. Destructors are called on any
 * existing elements in the vector. Default constructors are used to init the
 * new items.
 */
template<class T> void RtVector<T>::
		setAsNew(long len)
{
	/* Call All destructors. */
	T *pos = data;
	for ( long i = 0; i < tabLen; pos++, i++ )
		pos->~T();

	/* Adjust the allocated length. */
	if ( len < tabLen )
		downResize( len );
	else if ( len > tabLen )
		upResize( len );

	/* Set the new data length to exactly len. */
	tabLen = len;	
	
	/* Create items using default constructor. */
	T *dst = data;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();
}


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
template<class T> void RtVector<T>::
		replaceDup(long pos, const T &val, long len)
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
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(val);
}

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
template<class T> void RtVector<T>::
		replaceNew(long pos, long len)
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
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();
}

/**
 * \brief Remove len elements at position pos.
 *
 * Destructor is called on all elements removed. Elements to the right of pos
 * are shifted len spaces to the left to take up the free space. If pos is
 * greater than or equal to the length of the vector then undefined behavior
 * results. If pos is negative then it is treated as an offset relative to the
 * length of the vector.
 */
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

/**
 * \brief Insert len elements at position pos.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * The copy constructor is used to place the elements into this vector. If pos
 * is greater than the length of the vector then undefined behaviour results.
 * If pos is negative then it is treated as an offset relative to the length
 * of the vector.
 */
template<class T> void RtVector<T>::
		insert(long pos, const T *val, long len)
{
	/* If we are given a negative position to insert at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = tabLen + pos;
	
	/* Calculate the new length. */
	long newLen = tabLen + len;

	/* Up resize, we are growing. */
	upResize( newLen );

	/* Shift over data at insert spot if needed. */
	if ( len > 0 && pos < tabLen ) {
		memmove(data + pos + len, data + pos,
				sizeof(T)*(tabLen-pos));
	}

	/* Copy data in element by element. */
	T *dst = data + pos;
	const T *src = val;
	for ( long i = 0; i < len; i++, dst++, src++ )
		new(dst) T(*src);

	/* Set the new length. */
	tabLen = newLen;
}

/**
 * \brief Insert len copies of item at position pos.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * The copy constructor is used to place the element into this vector. If pos
 * is greater than the length of the vector then undefined behaviour results.
 * If pos is negative then it is treated as an offset relative to the length
 * of the vector.
 */
template<class T> void RtVector<T>::
		insertDup(long pos, const T &item, long len)
{
	/* If we are given a negative position to insert at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = tabLen + pos;
	
	/* Calculate the new length. */
	long newLen = tabLen + len;

	/* Up resize, we are growing. */
	upResize( newLen );

	/* Shift over data at insert spot if needed. */
	if ( len > 0 && pos < tabLen ) {
		memmove(data + pos + len, data + pos,
				sizeof(T)*(tabLen-pos));
	}

	/* Copy the data item in one at a time. */
	T *dst = data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T(item);

	/* Set the new length. */
	tabLen = newLen;
}

/**
 * \brief Insert len new elements using the default constructor.
 *
 * Elements in the vector from pos onward are shifted len spaces to the right.
 * Default constructors are used to init the new elements. If pos is off the
 * end of the vector then undefined behaviour results. If pos is negative then
 * it is treated as an offset relative to the length of the vector.
 */
template<class T> void RtVector<T>::
		insertNew(long pos, long len)
{
	/* If we are given a negative position to insert at then
	 * treat it as a position relative to the length. */
	if ( pos < 0 )
		pos = tabLen + pos;
	
	/* Calculate the new length. */
	long newLen = tabLen + len;

	/* Up resize, we are growing. */
	upResize( newLen );

	/* Shift over data at insert spot if needed. */
	if ( len > 0 && pos < tabLen ) {
		memmove(data + pos + len, data + pos,
				sizeof(T)*(tabLen-pos));
	}

	/* Init new data with default constructors. */
	T *dst = data + pos;
	for ( long i = 0; i < len; i++, dst++ )
		new(dst) T();

	/* Set the new length. */
	tabLen = newLen;
}


#endif 

