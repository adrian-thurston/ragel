/*
 *  Copyright 2002 Adrian Thurston <adriant@ragel.ca>
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

/* This header is not wrapped in ifndef becuase it is not intended to
 * be included by the user. */

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

#ifndef __AAPL_TDL_EL
#define __AAPL_TDL_EL
struct TDListEl
{
	TDListEl *prev, *next;
};
#endif /* __AAPL_TDL_EL */

#if defined( TDL_VALUE )
/**
 * \brief Double list element for TDListVal.
 *
 * TDListValEl stores the type T of TDListVal by value. 
 */
template <class T> struct TDListValEl : public TDListEl
{
	/**
	 * \brief Construct a TDListValEl with a given value.
	 *
	 * The only constructor available initializes the value element. This
	 * enforces that TDListVal elements are never created without having their
	 * value intialzed by the user. T's copy constructor is used to copy the
	 * value in.
	 */
	TDListValEl( const T &val ) : value(val) { }

	/**
	 * \brief Value stored by the list element.
	 *
	 * Value is always copied into new list elements using the copy
	 * constructor.
	 */
	T value;
};
#endif

/* Doubly Linked List */
template <TDL_TEMPDEF> class TDList : public DList<TDListEl>
{
private:
	typedef DList<TDListEl> BaseList;

public:

#ifdef DOUBLELIST_VALUE
	/**
	 * \brief Clear the double list contents.
	 *
	 * All elements are deleted.
	 */
	~TDList() { BaseList::empty(); }
#else
	/**
	 * \brief Abandon all elements in the list. 
	 *
	 * List elements are not deleted.
	 */
	~TDList() {}
#endif


	/* Assignment operator. This needs to be overridded to provide the correct
	 * return type. */
	TDList &operator=(const TDList &other) 
			{ BaseList::operator=( other ); return *this; }

#ifdef TDL_VALUE
	/**
	 * \brief Make a new element and prepend it to the front of the list.
	 *
	 * The item is copied into the new element using the copy constructor.
	 * Equivalent to list.addBefore(list.head, item).
	 */
	void prepend( const T &item );

	/**
	 * \brief Make a new element and append it to the end of the list.
	 *
	 * The item is copied into the new element using the copy constructor.
	 * Equivalent to list.addAfter(list.tail, item).
	 */
	void append( const T &item );

	/**
	 * \brief Make a new element and insert it immediately after an element in
	 * the list.
	 *
	 * The item is copied into the new element using the copy constructor. If
	 * prev_el is NULL then the new element is prepended to the front of the
	 * list. If prev_el is not already in the list then undefined behaviour
	 * results.  Equivalent to list.addAfter(prev_el, new TDListValEl(item)).
	 */
	void addAfter( Element *prev_el, const T &item );

	/**
	 * \brief Make a new element and insert it immediately before an element
	 * in the list. 
	 *
	 * The item is copied into the new element using the copy construcotor. If
	 * next_el is NULL then the new element is appended to the end of the
	 * list.  If next_el is not already in the list then undefined behaviour
	 * results.  Equivalent to list.addBefore(next_el, new TDListValEl(item)).
	 */
	void addBefore( Element *next_el, const T &item );
#endif

	/**
	 * \brief Prepend a single element to the front of the list.
	 *
	 * If new_el is already an element of some list, then undefined behaviour
	 * results. Equivalent to list.addBefore(list.head, new_el).
	 */
	void prepend( Element *new_el )
			{ BaseList::addBefore( head, BASECAST(new_el) ); }

	/**
	 * \brief Append a single element to the end of the list.
	 *
	 * If new_el is alreay an element of some list, then undefined behaviour
	 * results.  Equivalent to list.addAfter(list.tail, new_el).
	 */
	void append( Element *new_el )
			{ BaseList::addAfter( tail, BASECAST(new_el) ); }

	/**
	 * \brief Prepend an entire list to the beginning of this list.
	 *
	 * All items are moved, not copied. Afterwards, the other list is emtpy.
	 * All items are prepended at once, so this is an O(1) operation.
	 * Equivalent to list.addBefore(list.head, dl).
	 */
	void prepend( TDList &dl )
			{ BaseList::addBefore( head, dl ); }

	/**
	 * \brief Append an entire list to the end of the list.
	 *
	 * All items are moved, not copied. Afterwards, the other list is empty.
	 * All items are appened at once, so this is an O(1) operation.
	 * Equivalent to list.addAfter(list.tail, dl).
	 */
	void append( TDList &dl )
			{ BaseList::addAfter( tail, dl ); }

	void addAfter( Element *prev_el, Element *new_el )
			{ BaseList::addAfter( BASECAST(prev_el), BASECAST(new_el) ); }
	void addBefore( Element *next_el, Element *new_el)
			{ BaseList::addBefore( BASECAST(next_el), BASECAST(new_el) ); }

	void addAfter( Element *prev_el, TDList &dl )
			{ BaseList::addAfter( BASECAST(prev_el), dl ); }
	void addBefore( Element *next_el, TDList &dl )
			{ BaseList::addBefore( BASECAST(next_el), dl ); }

	/**
	 * \brief Detach the head of the list
	 *
	 * The element detached is not deleted. If there is no head of the list
	 * (the list is empty) then undefined behaviour results.  Equivalent to
	 * list.detach(list.head).
	 *
	 * \returns The element detached.
	 */
	Element *detachFirst()        
			{ return SUPERCAST(DList<TDListEl>::detach(head)); }

	/**
	 * \brief Detach the tail of the list
	 *
	 * The element detached is not deleted. If there is no tail of the list
	 * (the list is empty) then undefined behaviour results.  Equivalent to
	 * list.detach(list.tail).
	 *
	 * \returns The element detached.
	 */
	Element *detachLast()
			{ return SUPERCAST(BaseList::detach(tail)); }

 	/* Detaches an element from the list. Does not free any memory. */
	Element *detach( Element *el )
			{ return SUPERCAST(BaseList::detach(BASECAST(el))); }

	/**
	 * \brief Detach and delete the first element in the list.
	 *
	 * If there is no first element (the list is empty) then undefined
	 * behaviour results.  Equivalent to delete list.detach(list.head);
	 */
	void removeFirst()
			{ delete SUPERCAST(BaseList::detach( head )); }

	/**
	 * \brief Detach and delete the last element in the list.
	 *
	 * If there is no last element (the list is emtpy) then undefined
	 * behaviour results.  Equivalent to delete list.detach(list.tail);
	 */
	void removeLast()
			{ delete SUPERCAST(BaseList::detach( tail )); }

	/**
	 * \brief Detach and delete an element from the list.
	 *
	 * If the element is not in the list, then undefined behaviour results.
	 * Equivalent to delete list.detach(el);
	 */
	void remove( Element *el )
			{ delete SUPERCAST(BaseList::detach( BASECAST(el) )); }
	
	/* Forward this so a ref can be used. */
	struct Iter;

	/* Class for setting the iterator. */
	struct IterFirst { IterFirst( const TDList &l ) : l(l) { } const TDList &l; };
	struct IterLast { IterLast( const TDList &l ) : l(l) { } const TDList &l; };
	struct IterNext { IterNext( const Iter &i ) : i(i) { } const Iter &i; };
	struct IterPrev { IterPrev( const Iter &i ) : i(i) { } const Iter &i; };

	/* Double list iterator. */
	struct Iter
	{
		/* Default construct. */
		Iter() : ptr(0) { }

		/* Construct from a double list. */
		Iter( const TDList &dl )     : ptr(SUPERCAST(dl.head)) { }
		Iter( const IterFirst &dlf ) : ptr(SUPERCAST(dlf.l.head)) { }
		Iter( const IterLast &dll )  : ptr(SUPERCAST(dll.l.tail)) { }
		Iter( const IterNext &dln )  : ptr(SUPERCAST(dln.i.ptr->BASE_EL(next))) { }
		Iter( const IterPrev &dlp )  : ptr(SUPERCAST(dlp.i.ptr->BASE_EL(prev))) { }

		/* Assign from a double list. */
		Iter &operator=( const TDList &dl )    
				{ ptr = SUPERCAST(dl.head); return *this; }
		Iter &operator=( const IterFirst &af ) 
				{ ptr = SUPERCAST(af.l.head); return *this; }
		Iter &operator=( const IterLast &al )  
				{ ptr = SUPERCAST(al.l.tail); return *this; }
		Iter &operator=( const IterNext &an )  
				{ ptr = SUPERCAST(an.i.ptr->BASE_EL(next)); return *this; }
		Iter &operator=( const IterPrev &ap )  
				{ ptr = SUPERCAST(ap.i.ptr->BASE_EL(prev)); return *this; }

		/* At the end, beginning? */
		bool lte() const    { return ptr != 0; }
		bool end() const    { return ptr == 0; }
		bool gtb() const { return ptr != 0; }
		bool beg() const { return ptr == 0; }

		/* At the first, last element. */
		bool first() const { return ptr && ptr->BASE_EL(prev) == 0; }
		bool last() const  { return ptr && ptr->BASE_EL(next) == 0; }

#ifdef TDL_VALUE
		/* Cast, dereference, arrow ops. */
		operator T*() const         { return &ptr->value; }
		T &operator *() const       { return ptr->value; }
		T *operator->() const       { return &ptr->value; }
#else
		/* Cast, dereference, arrow ops. */
		operator Element*() const   { return ptr; }
		Element &operator *() const { return *ptr; }
		Element *operator->() const { return ptr; }
#endif

		/* Increment. */
		inline Element *operator++()      { return ptr = SUPERCAST(ptr->BASE_EL(next)); }
		inline Element *increment()       { return ptr = SUPERCAST(ptr->BASE_EL(next)); }
		inline Element *operator++(int);

		/* Decrement. */
		inline Element *operator--()      { return ptr = SUPERCAST(ptr->BASE_EL(prev)); }
		inline Element *decrement()       { return ptr = SUPERCAST(ptr->BASE_EL(prev)); }
		inline Element *operator--(int);

		/* Return the next, prev. Does not modify. */
		inline IterNext next() const { return IterNext(*this); }
		inline IterPrev prev() const { return IterPrev(*this); }

		/* The iterator is simply a pointer. */
		Element *ptr;
	};

	/* Return classes for setting first, last. */
	IterFirst first()  { return IterFirst(*this); }
	IterLast last()    { return IterLast(*this); }
};

#ifdef TDL_VALUE

/* Prepend a new item. Inlining this bloats the caller with new overhead. */
template <TDL_TEMPDEF> void TDList<TDL_TEMPUSE>::
		prepend( const T &item )
{
	BaseList::addBefore( head, BASECAST(new Element(item)) ); 
}

/* Append a new item. Inlining this bloats the caller with the new overhead. */
template <TDL_TEMPDEF> void TDList<TDL_TEMPUSE>::
		append( const T &item )
{
	BaseList::addAfter( tail, BASECAST(new Element(item)) );
}

/* Add a new item after a prev element. Inlining this bloats the caller with
 * the new overhead. */
template <TDL_TEMPDEF> void TDList<TDL_TEMPUSE>::
		addAfter( Element *prev_el, const T &item )
{
	BaseList::addAfter( prev_el, BASECAST(new Element(item)) );
}

/* Add a new item before a next element. Inlining this bloats the caller with
 * the new overhead. */
template <TDL_TEMPDEF> void TDList<TDL_TEMPUSE>::
		addBefore( Element *next_el, const T &item )
{
	BaseList::addBefore( next_el, BASECAST(new Element(item)) );
}

#endif /* TDL_VALUE */

/*
 * The larger iterator operators.
 */

/* Postfix ++ */
template <TDL_TEMPDEF> Element *TDList<TDL_TEMPUSE>::Iter::
		operator++(int)       
{
	Element *rtn = ptr; 
	ptr = SUPERCAST(ptr->BASE_EL(next));
	return rtn;
}

/* Postfix -- */
template <TDL_TEMPDEF> Element *TDList<TDL_TEMPUSE>::Iter::
		operator--(int)       
{
	Element *rtn = ptr;
	ptr = SUPERCAST(ptr->BASE_EL(prev));
	return rtn;
}

#ifdef AAPL_NAMESPACE
}
#endif
