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

/* SVector */
template < class T, class Resize = ResizeExpn > class SVector :
	public STable<T>, public Resize
{
private:
	typedef STable<T> BaseTable;

public:
	/**
	 * \brief Initialize an empty vector with no space allocated.  
	 *
	 * If a linear resizer is used, the step defaults to 256 units of T. For a
	 * runtime vector both up and down allocation schemes default to
	 * Exponential.
	 */
	SVector() { }

	/* Create a vector with an intial number of elements. */
	SVector( long size ) { setAsNew( size ); }

	/* Create a vector of specific size and allocation. */
	SVector( long size, long allocLen );

	/* Shallow copy. */
	SVector( const SVector &v );

	/* Shallow copy. */
	SVector(STabHead *head);

	/* Free all mem used by the vector. */
	~SVector() { empty(); }

	/* Delete all items. */
	void empty();

	/**
	 * \brief Deep copy another vector into this vector.
	 *
	 * Copies the entire contents of the other vector into this vector. Any
	 * existing contents are first deleted. Equivalent to setAs.
	 */
	void deepCopy( const SVector &v )     { setAs(v.data, v.length()); }

	/* Perform a shallow copy of another vector. */
	SVector &operator=( const SVector &v );

	/* Perform a shallow copy of another vector by the header. */
	SVector &operator=( STabHead *head );


	/*@{*/
	/* Insert one element at position pos. */
	void insert(long pos, const T &val)     { insert(pos, &val, 1); }

	/* Insert an array of values. */
	void insert(long pos, const T *val, long len);

	/* Insert the contents of another vector. */
	void insert(long pos, const SVector &v) { insert(pos, v.data, v.length()); }

	/* Insert len copies of val into the vector. */
	void insertDup(long pos, const T &val, long len);

	/* Insert one new item using the default constructor. */
	void insertNew(long pos)                { insertNew(pos, 1); }

	/* Insert len new items using default constructor. */
	void insertNew(long pos, long len);
	/*@}*/

	/*@{*/
	/* Delete one element. */
	void remove(long pos)                   { remove(pos, 1); }

	/* Delete a number of elements. */
	void remove(long pos, long len);
	/*@}*/

	/*@{*/
	/* Replace a single element. */
	void replace(long pos, const T &val)     { replace(pos, &val, 1); }

	/* Replace with an array of values. */
	void replace(long pos, const T *val, long len);

	/* Replace with the contents of another vector. */
	void replace(long pos, const SVector &v) { replace(pos, v.data, v.length()); }

	/* Replace len items with len copies of val. */
	void replaceDup(long pos, const T &val, long len);

	/* Replace one item at pos with a constructed object. */
	void replaceNew(long pos)                { replaceNew(pos, 1); }

	/* Replace len items at pos with newly constructed objects. */
	void replaceNew(long pos, long len);
	/*@}*/

	/*@{*/
	/* Set the vector to be val exactly. */
	void setAs(const T &val)             { setAs(&val, 1); }

	/* Set to the contents of an array. */
	void setAs(const T *val, long len);

	/* Set to the contents of another vector. */
	void setAs(const SVector &v)         { setAs(v.data, v.length()); }

	/* Set as len copies of item. */
	void setAsDup(const T &item, long len);

	/* Set as a newly constructed object using the default constructor. */
	void setAsNew()                      { setAsNew(1); }

	/* Set as newly constructed objects using the default constructor. */
	void setAsNew(long len);
	/*@}*/

	/*@{*/
	/* Append a single element. */
	void append(const T &val)                { replace(BaseTable::length(), &val, 1); }

	/* Append an array of elements. */
	void append(const T *val, long len)       { replace(BaseTable::length(), val, len); }

	/* Append to the end of the vector from another vector. */
	void append(const SVector &v)            { replace(BaseTable::length(), v.data, v.length()); }

	/* Append len copies of item. */
	void appendDup(const T &item, long len)   { replaceDup(BaseTable::length(), item, len); }

	/* Append a newly created item. Uses the copy constructor. */
	void appendNew()                         { replaceNew(BaseTable::length(), 1); }

	/* Append newly created items. Uses the copy constructor. */
	void appendNew(long len)                  { replaceNew(BaseTable::length(), len); }
	/*@}*/
	
	/*@{*/
	/* Prepend a single element. */
	void prepend(const T &val)               { insert(0, &val, 1); }

	/* Prepend an array of elements. */
	void prepend(const T *val, long len)      { insert(0, val, len); }

	/* Prepend to the front of the vector from another vector. */
	void prepend(const SVector &v)           { insert(0, v.data, v.length()); }

	/* Prepend len copies of item. */
	void prependDup(const T &item, long len)  { insertDup(0, item, len); }

	/* Prepend a newly created item. Uses the copy constructor to init. */
	void prependNew()                        { insertNew(0, 1); }

	/* Prepend newly created items. Uses the copy constructor to init. */
	void prependNew(long len)                 { insertNew(0, len); }
	/*@}*/

	/* Convenience access. */
	T &operator[](int i) const { return BaseTable::data[i]; }
	long size() const           { return BaseTable::length(); }

	/* Various classes for setting the iterator */
	struct Iter;
	struct IterFirst { IterFirst( const SVector &v ) : v(v) { } const SVector &v; };
	struct IterLast { IterLast( const SVector &v ) : v(v) { } const SVector &v; };
	struct IterNext { IterNext( const Iter &i ) : i(i) { } const Iter &i; };
	struct IterPrev { IterPrev( const Iter &i ) : i(i) { } const Iter &i; };

	/** 
	 * \brief Shared Vector Iterator. 
	 * \ingroup iterators
	 */
	struct Iter
	{
		/* Construct, assign. */
		Iter() : ptr(0), ptrBeg(0), ptrEnd(0) { }

		/* Construct. */
		Iter( const SVector &v );
		Iter( const IterFirst &vf );
		Iter( const IterLast &vl );
		inline Iter( const IterNext &vn );
		inline Iter( const IterPrev &vp );

		/* Assign. */
		Iter &operator=( const SVector &v );
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

		/** \brief Move to previous item. */
		T *operator--()       { return --ptr; }

		/** \brief Move to previous item. */
		T *operator--(int)    { return ptr--; }

		/** \brief Move to previous item. */
		T *decrement()        { return --ptr; }

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

	void setAsCommon(long len);
	long replaceCommon(long pos, long len);
	long insertCommon(long pos, long len);

	void upResize(long len);
	void upResizeDup(long len);
	void upResizeFromEmpty(long len);
	void downResize(long len);
	void downResizeDup(long len);
};


/* Create a vector with an intial number of elements and size. */
template <class T, class Resize> SVector<T, Resize>::
		SVector( long size, long allocLen )
{
	/* Allocate the space if we are given a positive allocLen. */
	if ( allocLen > 0 ) {
		/* Allocate the data needed. */
		STabHead *head = (STabHead*) malloc( sizeof(STabHead) + 
				sizeof(T) * allocLen );
		if ( head == 0 )
			throw std::bad_alloc();

		/* Set up the header and save the data pointer. */
		head->refCount = 1;
		head->allocLen = allocLen;
		head->tabLen = 0;
		BaseTable::data = (T*) (head + 1);
	}

	/* Grow to the size specified. If we did not have enough space
	 * allocated that is ok. Table will be grown to the right size. */
	setAsNew( size );
}

/**
 * \brief Perform a shallow copy of the vector.
 *
 * Takes a reference to the contents of the other vector.
 */
template <class T, class Resize> SVector<T, Resize>::
		SVector(const SVector<T, Resize> &v)
{
	/* Take a reference to other, if any data is allocated. */
	if ( v.data == 0 )
		BaseTable::data = 0;
	else {
		/* Get the source header, up the refcount and ref it. */
		STabHead *srcHead = ((STabHead*) v.data) - 1;
		srcHead->refCount += 1;
		BaseTable::data = (T*) (srcHead + 1);
	}
}

/**
 * \brief Perform a shallow copy of the vector from only the header.
 *
 * Takes a reference to the contents specified by the header.
 */
template <class T, class Resize> SVector<T, Resize>::
		SVector(STabHead *head)
{
	/* Take a reference to other, if the header is no-null. */
	if ( head == 0 )
		BaseTable::data = 0;
	else {
		head->refCount += 1;
		BaseTable::data = (T*) (head + 1);
	}
}


/**
 * \brief Shallow copy another vector into this vector.
 *
 * Takes a reference to the other vector. The contents of this vector are
 * first emptied. 
 *
 * \returns A reference to this.
 */
template <class T, class Resize> SVector<T, Resize> &
		SVector<T, Resize>:: operator=( const SVector &v )
{
	/* First clean out the current contents. */
	empty();

	/* Take a reference to other, if any data is allocated. */
	if ( v.data == 0 )
		BaseTable::data = 0;
	else {
		/* Get the source header, up the refcount and ref it. */
		STabHead *srcHead = ((STabHead*) v.data) - 1;
		srcHead->refCount += 1;
		BaseTable::data = (T*) (srcHead + 1);
	}
	return *this;
}

/**
 * \brief Shallow copy another vector into this vector from only the header.
 *
 * Takes a reference to the other header vector. The contents of this vector
 * are first emptied. 
 *
 * \returns A reference to this.
 */
template <class T, class Resize> SVector<T, Resize> &
		SVector<T, Resize>::operator=( STabHead *head )
{
	/* First clean out the current contents. */
	empty();

	/* Take a reference to other, if the header is no-null. */
	if ( head == 0 )
		BaseTable::data = 0;
	else {
		head->refCount += 1;
		BaseTable::data = (T*) (head + 1);
	}
	return *this;
}

/* Init a vector iterator with just a vector. */
template <class T, class Resize> SVector<T, Resize>::
		Iter::Iter( const SVector &v ) 
{
	long length;
	if ( v.data == 0 || (length=(((STabHead*)v.data)-1)->tabLen) == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = v.data;
		ptrBeg = v.data-1;
		ptrEnd = v.data+length;
	}
}

/* Init a vector iterator with the first of a vector. */
template <class T, class Resize> SVector<T, Resize>::
		Iter::Iter( const IterFirst &vf ) 
{
	long length;
	if ( vf.v.data == 0 || (length=(((STabHead*)vf.v.data)-1)->tabLen) == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = vf.v.data;
		ptrBeg = vf.v.data-1;
		ptrEnd = vf.v.data+length;
	}
}

/* Init a vector iterator with the last of a vector. */
template <class T, class Resize> SVector<T, Resize>::
		Iter::Iter( const IterLast &vl ) 
{
	long length;
	if ( vl.v.data == 0 || (length=(((STabHead*)vl.v.data)-1)->tabLen) == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = vl.v.data+length-1;
		ptrBeg = vl.v.data-1;
		ptrEnd = vl.v.data+length;
	}
}

/* Init a vector iterator with the next of some other iterator. */
template <class T, class Resize> SVector<T, Resize>::
		Iter::Iter( const IterNext &vn ) 
:
	ptr(vn.i.ptr+1), 
	ptrBeg(vn.i.ptrBeg),
	ptrEnd(vn.i.ptrEnd)
{
}

/* Init a vector iterator with the prev of some other iterator. */
template <class T, class Resize> SVector<T, Resize>::
		Iter::Iter( const IterPrev &vp ) 
:
	ptr(vp.i.ptr-1),
	ptrBeg(vp.i.ptrBeg),
	ptrEnd(vp.i.ptrEnd)
{
}

/* Set a vector iterator with some vector. */
template <class T, class Resize> typename SVector<T, Resize>::Iter &
		SVector<T, Resize>::Iter::operator=( const SVector &v )    
{
	long length;
	if ( v.data == 0 || (length=(((STabHead*)v.data)-1)->tabLen) == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = v.data; 
		ptrBeg = v.data-1; 
		ptrEnd = v.data+length; 
	}
	return *this;
}

/* Set a vector iterator with the first element in a vector. */
template <class T, class Resize> typename SVector<T, Resize>::Iter &
		SVector<T, Resize>::Iter::operator=( const IterFirst &vf )    
{
	long length;
	if ( vf.v.data == 0 || (length=(((STabHead*)vf.v.data)-1)->tabLen) == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = vf.v.data; 
		ptrBeg = vf.v.data-1; 
		ptrEnd = vf.v.data+length; 
	}
	return *this;
}

/* Set a vector iterator with the last element in a vector. */
template <class T, class Resize> typename SVector<T, Resize>::Iter &
		SVector<T, Resize>::Iter::operator=( const IterLast &vl )    
{
	long length;
	if ( vl.v.data == 0 || (length=(((STabHead*)vl.v.data)-1)->tabLen) == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = vl.v.data+length-1; 
		ptrBeg = vl.v.data-1; 
		ptrEnd = vl.v.data+length; 
	}
	return *this;
}

/* Set a vector iterator with the next of some other iterator. */
template <class T, class Resize> typename SVector<T, Resize>::Iter &
		SVector<T, Resize>::Iter::operator=( const IterNext &vn )    
{
	ptr = vn.i.ptr+1; 
	ptrBeg = vn.i.ptrBeg;
	ptrEnd = vn.i.ptrEnd;
	return *this;
}

/* Set a vector iterator with the prev of some other iterator. */
template <class T, class Resize> typename SVector<T, Resize>::Iter &
		SVector<T, Resize>::Iter::operator=( const IterPrev &vp )    
{
	ptr = vp.i.ptr-1; 
	ptrBeg = vp.i.ptrBeg;
	ptrEnd = vp.i.ptrEnd;
	return *this;
}

/* Up resize the data for len elements using Resize::upResize to tell us the
 * new length. Reads and writes allocLen. Does not read or write length.
 * Assumes that there is some data allocated already. */
template <class T, class Resize> void SVector<T, Resize>::
		upResize(long len)
{
	/* Get the current header. */
	STabHead *head = ((STabHead*)BaseTable::data) - 1;

	/* Ask the resizer what the new length will be. */
	long newLen = Resize::upResize(head->allocLen, len);

	/* Did the data grow? */
	if ( newLen > head->allocLen ) {
		head->allocLen = newLen;

		/* Table exists already, resize it up. */
		head = (STabHead*) realloc( head, sizeof(STabHead) + 
				sizeof(T) * newLen );
		if ( head == 0 )
			throw std::bad_alloc();

		/* Save the data pointer. */
		BaseTable::data = (T*) (head + 1);
	}
}

/* Allocates a new buffer for an up resize that requires a duplication of the
 * data. Uses Resize::upResize to get the allocation length.  Reads and writes
 * allocLen. This upResize does write the new length.  Assumes that there is
 * some data allocated already. */
template <class T, class Resize> void SVector<T, Resize>::
		upResizeDup(long len)
{
	/* Get the current header. */
	STabHead *head = ((STabHead*)BaseTable::data) - 1;

	/* Ask the resizer what the new length will be. */
	long newLen = Resize::upResize(head->allocLen, len);

	/* Dereferencing the existing data, decrement the refcount. */
	head->refCount -= 1;

	/* Table exists already, resize it up. */
	head = (STabHead*) malloc( sizeof(STabHead) + sizeof(T) * newLen );
	if ( head == 0 )
		throw std::bad_alloc();

	head->refCount = 1;
	head->allocLen = newLen;
	head->tabLen = len;

	/* Save the data pointer. */
	BaseTable::data = (T*) (head + 1);
}

/* Up resize the data for len elements using Resize::upResize to tell us the
 * new length. Reads and writes allocLen. This upresize DOES write length.
 * Assumes that no data is allocated. */
template <class T, class Resize> void SVector<T, Resize>::
		upResizeFromEmpty(long len)
{
	/* There is no table yet. If the len is zero, then there is no need to
	 * create a table. */
	if ( len > 0 ) {
		/* Ask the resizer what the new length will be. */
		long newLen = Resize::upResize(0, len);

		/* If len is greater than zero then we are always allocating the table. */
		STabHead *head = (STabHead*) malloc( sizeof(STabHead) + 
				sizeof(T) * newLen );
		if ( head == 0 )
			throw std::bad_alloc();

		/* Set up the header and save the data pointer. Note that we set the
		 * length here. This differs from the other upResizes. */
		head->refCount = 1;
		head->allocLen = newLen;
		head->tabLen = len;
		BaseTable::data = (T*) (head + 1);
	}
}

/* Down resize the data for len elements using Resize::downResize to determine
 * the new length. Reads and writes allocLen. Does not read or write length. */
template <class T, class Resize> void SVector<T, Resize>::
		downResize(long len)
{
	/* If there is already no length, then there is nothing we can do. */
	if ( BaseTable::data != 0 ) {
		/* Get the current header. */
		STabHead *head = ((STabHead*)BaseTable::data) - 1;

		/* Ask the resizer what the new length will be. */
		long newLen = Resize::downResize( head->allocLen, len );

		/* Did the data shrink? */
		if ( newLen < head->allocLen ) {
			if ( newLen == 0 ) {
				/* Simply free the data. */
				free( head );
				BaseTable::data = 0;
			}
			else {
				/* Save the new allocated length. */
				head->allocLen = newLen;

				/* Not shrinking to size zero, realloc it to the smaller size. */
				head = (STabHead*) realloc( head, sizeof(STabHead) + 
						sizeof(T) * newLen );
				if ( head == 0 )
					throw std::bad_alloc();
				
				/* Save the new data ptr. */
				BaseTable::data = (T*) (head + 1);
			}
		}
	}
}

/* Allocate a new buffer for a down resize and duplication of the array.  The
 * new array will be len long and allocation size will be determined using
 * Resize::downResize with the old array's allocLen. Does not actually copy
 * any data. Reads and writes allocLen and writes the new len. */
template <class T, class Resize> void SVector<T, Resize>::
		downResizeDup(long len)
{
	/* If there is already no length, then there is nothing we can do. */
	if ( BaseTable::data != 0 ) {
		/* Get the current header. */
		STabHead *head = ((STabHead*)BaseTable::data) - 1;

		/* Ask the resizer what the new length will be. */
		long newLen = Resize::downResize( head->allocLen, len );

		/* Detaching from the existing head, decrement the refcount. */
		head->refCount -= 1;

		/* Not shrinking to size zero, malloc it to the smaller size. */
		head = (STabHead*) malloc( sizeof(STabHead) + sizeof(T) * newLen );
		if ( head == 0 )
			throw std::bad_alloc();

		/* Save the new allocated length. */
		head->refCount = 1;
		head->allocLen = newLen;
		head->tabLen = len;

		/* Save the data pointer. */
		BaseTable::data = (T*) (head + 1);
	}
}

#ifdef AAPL_NAMESPACE
}
#endif
