/*
 *  Copyright 2001 Adrian Thurston <adriant@ragel.ca>
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

#include <assert.h>

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

#ifdef WALKABLE
/* This is used by TAvlTree, TAvlMel and TAvlMelKey so it
 * must be protected by global ifdefs. */
#ifndef __AAPL_TAVLI_EL__
#define __AAPL_TAVLI_EL__

/* Tree element properties for linked Tiny AVL trees. */
struct TAvliTreeEl 
{
	/**
	 * \brief Tree pointers connecting element in a tree.
	 */
	TAvliTreeEl *left, *right, *parent;

	/**
	 * \brief Linked list pointers.
	 */
	TAvliTreeEl *prev, *next;

	/**
	 * \brief Height of the tree rooted at this element.
	 *
	 * Height is required by the AVL balancing algorithm.
	 */
	long height;
};
#endif /* __AAPL_AVLI_EL__ */

#else /* not WALKABLE */

/* This is used by All the non walkable trees so it must be
 * protected by a global ifdef. */
#ifndef __AAPL_TAVL_EL__
#define __AAPL_TAVL_EL__
/**
 * \brief Tree element properties for linked AVL trees.
 *
 * TAvlTreeEl needs to be inherited by classes that intend to be element in an
 * TAvlTree. 
 */
struct TAvlTreeEl
{
	/**
	 * \brief Tree pointers connecting element in a tree.
	 */
	TAvlTreeEl *left, *right, *parent;

	/**
	 * \brief Height of the tree rooted at this element.
	 *
	 * Height is required by the AVL balancing algorithm.
	 */
	long height;
};
#endif /* __AAPL_TAVL_EL__ */
#endif /* def WALKABLE */


#if defined( AVLTREE_MAP )

#ifdef WALKABLE

/**
 * \brief Tree element for TAvliMap
 *
 * Stores the key and value pair.
 */
template <class Key, class Value> struct TAvliMapEl : public TAvliTreeEl
{
	TAvliMapEl(const Key &key) 
		: key(key) { }
	TAvliMapEl(const Key &key, const Value &value) 
		: key(key), value(value) { }

	const Key &getKey() { return key; }

	/** \brief The key. */
	Key key;

	/** \brief The value. */
	Value value;
};

#else /* not WALKABLE */

/**
 * \brief Tree element for TAvlMap
 *
 * Stores the key and value pair.
 */
template <class Key, class Value> struct TAvlMapEl : public TAvlTreeEl
{
	TAvlMapEl(const Key &key) 
		: key(key) { }
	TAvlMapEl(const Key &key, const Value &value) 
		: key(key), value(value) { }

	const Key &getKey() { return key; }

	/** \brief The key. */
	Key key;

	/** \brief The value. */
	Value value;
};
#endif /* def WALKABLE */

#elif defined( AVLTREE_SET )

#ifdef WALKABLE
/**
 * \brief Tree element for TAvliSet
 *
 * Stores the key.
 */
template <class Key> struct TAvliSetEl : public TAvliTreeEl
{
	TAvliSetEl(const Key &key) : key(key) { }

	const Key &getKey() { return key; }

	/** \brief The key. */
	Key key;
};

#else /* not WALKABLE */

/**
 * \brief Tree element for TAvlSet
 *
 * Stores the key.
 */
template <class Key> struct TAvlSetEl : public TAvlTreeEl
{
	TAvlSetEl(const Key &key) : key(key) { }

	const Key &getKey() { return key; }

	/** \brief The key. */
	Key key;
};
#endif /* def WALKABLE */

#endif /* AVLTREE_SET */

/* Common TAvlTree Class */
template < AVLMEL_CLASSDEF > class TAvlTree 
		: public Compare,
#ifdef WALKABLE
		public AvliKeyless<TAvliTreeEl>
#else
		public AvlKeyless<TAvlTreeEl>
#endif
{
private:
	typedef AvlKeyless<TAvlTreeEl> BaseTree;

public:
	/**
	 * \brief Create an empty tree.
	 */
	TAvlTree() { }

	/** 
	 * \brief Perform a deep copy of the tree. 
	 *
	 * Each element is duplicated for the new tree. Copy constructors are used to
	 * create the new element.
	 */
	TAvlTree(const TAvlTree &other) : BaseTree(other) { }

	/* Perform a deep copy of another tree into this tree. This is overridden
	 * to provide the correct return type. */
	TAvlTree &operator=( const TAvlTree &tree )
			{ BaseTree::operator=( tree ); return *this; }

	/* Insert a element into the tree. */
	Element *insert( Element *element, Element **lastFound = 0 );
	Element *insert( const Key &key, Element **lastFound = 0 );
#if defined( AVLTREE_MAP )
	Element *insert( const Key &key, const Value &val,
			Element **lastFound = 0 );
#endif

	/* Find a element in the tree. Returns the element if 
	 * key exists, false otherwise. */
	Element *find( const Key &key ) const;

	/* Detach a element from the tree. */
	Element *detach( const Key &key );

	/* Detach and delete a element from the tree. */
	bool remove( const Key &key );

	/* Detach a element from the tree. */
	Element *detach( Element *element ) 
			{ return SUPERCAST(BaseTree::detach( BASECAST(element)) ); }

	/* Detach and delete a element from the tree. */
	void remove( Element *element );

	/* Forward this so a ref can be used. */
	struct Iter;

	/* Various classes for setting the iterator */
	struct IterFirst { IterFirst( const TAvlTree &t ) : t(t) { } const TAvlTree &t; };
	struct IterLast { IterLast( const TAvlTree &t ) : t(t) { } const TAvlTree &t; };
	struct IterNext { IterNext( const Iter &i ) : i(i) { } const Iter &i; };
	struct IterPrev { IterPrev( const Iter &i ) : i(i) { } const Iter &i; };

#ifdef WALKABLE
	/* TAvlTree iterator. */
	struct Iter
	{
		/* Default construct. */
		Iter() : ptr(0) { }

		/* Construct from an avl tree and iterator-setting classes. */
		Iter( const TAvlTree &t )   : ptr(SUPERCAST(t.head)) { }
		Iter( const IterFirst &af ) : ptr(SUPERCAST(af.t.head)) { }
		Iter( const IterLast &al )  : ptr(SUPERCAST(al.t.tail)) { }
		Iter( const IterNext &an )  : ptr(findNext(an.i.ptr)) { }
		Iter( const IterPrev &ap )  : ptr(findPrev(ap.i.ptr)) { }
		
		/* Assign from a tree and iterator-setting classes. */
		Iter &operator=( const TAvlTree &tree )
				{ ptr = SUPERCAST(tree.head); return *this; }
		Iter &operator=( const IterFirst &af )
				{ ptr = SUPERCAST(af.t.head); return *this; }
		Iter &operator=( const IterLast &al )
				{ ptr = SUPERCAST(al.t.tail); return *this; }
		Iter &operator=( const IterNext &an )
				{ ptr = findNext(BASECAST(an.i.ptr)); return *this; }
		Iter &operator=( const IterPrev &ap )
				{ ptr = findPrev(BASECAST(ap.i.ptr)); return *this; }

		/* At the end, beginning? */
		bool lte() const { return ptr != 0; }
		bool end() const { return ptr == 0; }
		bool gtb() const { return ptr != 0; }
		bool beg() const { return ptr == 0; }

		/* At the first, last element. */
		bool first() const { return ptr && ptr->BASE_EL(prev) == 0; }
		bool last() const { return ptr && ptr->BASE_EL(next) == 0; }

#ifdef AVLTREE_SET
		/* Cast, dereference, arrow ops. */
		operator Key*() const       { return &ptr->key; }
		Key &operator *() const     { return ptr->key; }
		Key *operator->() const     { return &ptr->key; }
#else
		/* Cast, dereference, arrow ops. */
		operator Element*() const      { return ptr; }
		Element &operator *() const    { return *ptr; }
		Element *operator->() const    { return ptr; }
#endif

		/* Forward increment. */
		inline Element *operator++();
		inline Element *operator++(int);
		inline Element *increment();

		/* Reverse increment. */
		inline Element *operator--();
		inline Element *operator--(int);
		inline Element *decrement();

		/* Return the next, prev. Does not modify. */
		IterNext next() const { return IterNext( *this ); }
		IterPrev prev() const { return IterPrev( *this ); }

	private:
		static Element *findPrev( Element *element ) { return SUPERCAST(element->BASE_EL(prev)); }
		static Element *findNext( Element *element ) { return SUPERCAST(element->BASE_EL(next)); }

	public:
		/* The iterator is simply a pointer. */
		Element *ptr;
	};

#else

	/* TAvlTree iterator. */
	struct Iter
	{
		/* Default construct. */
		Iter() : ptr(0), tree(0) { }

		/* Construct from a tree and iterator-setting classes. */
		Iter( const TAvlTree &t )   : ptr(SUPERCAST(t.head)), tree(&t) { }
		Iter( const IterFirst &af ) : ptr(SUPERCAST(af.t.head)), tree(&af.t) { }
		Iter( const IterLast &al )  : ptr(SUPERCAST(al.t.tail)), tree(&al.t) { }
		Iter( const IterNext &an )  : ptr(findNext(an.i.ptr)), tree(an.i.tree) { }
		Iter( const IterPrev &ap )  : ptr(findPrev(ap.i.ptr)), tree(ap.i.tree) { }
		
		/* Assign from a tree and iterator-setting classes. */
		Iter &operator=( const TAvlTree &t )    
				{ ptr = SUPERCAST(t.head); tree = &t; return *this; }
		Iter &operator=( const IterFirst &af ) 
				{ ptr = SUPERCAST(af.t.head); tree = &af.t; return *this; }
		Iter &operator=( const IterLast &al )  
				{ ptr = SUPERCAST(al.t.tail); tree = &al.t; return *this; }
		Iter &operator=( const IterNext &an )  
				{ ptr = findNext(an.i.ptr); tree = an.i.tree; return *this; }
		Iter &operator=( const IterPrev &ap )  
				{ ptr = findPrev(ap.i.ptr); tree = ap.i.tree; return *this; }

		/* At the end, beginning? */
		bool lte() const { return ptr != 0; }
		bool end() const { return ptr == 0; }
		bool gtb() const { return ptr != 0; }
		bool beg() const { return ptr == 0; }

		/* At the first, last element. Note that ptr cannot be set without
		 * tree being set. */
		bool first() const { return ptr && BASECAST(ptr) == tree->head; }
		bool last() const { return ptr && BASECAST(ptr) == tree->tail; }

		/* Cast, dereference, arrow ops. */
		operator Element*() const      { return ptr; }
#ifdef AVLTREE_SET
		operator Key*() const       { return &ptr->key; }
		Key &operator *() const     { return ptr->key; }
		Key *operator->() const     { return &ptr->key; }
#else
		Element &operator *() const    { return *ptr; }
		Element *operator->() const    { return ptr; }
#endif

		/* Forward increment. */
		inline Element *operator++();
		inline Element *operator++(int);
		inline Element *increment();

		/* Reverse increment. */
		inline Element *operator--();
		inline Element *operator--(int);
		inline Element *decrement();

		/* Return the next, prev. Does not modify. */
		IterNext next() const { return IterNext( *this ); }
		IterPrev prev() const { return IterPrev( *this ); }

	private:
		static Element *findPrev( Element *element );
		static Element *findNext( Element *element );

	public:
		/* The iterator is simply a pointer. */
		Element *ptr;

		/* If the list is not walkable then keep a pointer to the tree so we
		 * can test head and tail. */
		const TAvlTree *tree;
	};
#endif

	/* Return classes for setting first, last. */
	IterFirst first()  { return IterFirst( *this ); }
	IterLast last()    { return IterLast( *this ); }
};

/*
 * Iterator operators.
 */

/* Prefix ++ */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::Iter::
		operator++()       
{
	return ptr = findNext( ptr );
}

/* Postfix ++ */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::Iter::
		operator++(int)       
{
	Element *rtn = ptr; 
	ptr = findNext( ptr );
	return rtn;
}

/* increment */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::Iter::
		increment()
{
	return ptr = findNext( ptr );
}

/* Prefix -- */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::Iter::
		operator--()       
{
	return ptr = findPrev( ptr );
}

/* Postfix -- */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::Iter::
		operator--(int)       
{
	Element *rtn = ptr;
	ptr = findPrev( ptr );
	return rtn;
}

/* decrement */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::Iter::
		decrement()
{
	return ptr = findPrev( ptr );
}

#ifndef WALKABLE

/* Move ahead one. */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::Iter::
		findNext( Element *element )
{
	/* Try to go right once then infinite left. */
	if ( element->BASE_EL(right) != 0 ) {
		element = SUPERCAST(element->BASE_EL(right));
		while ( element->BASE_EL(left) != 0 )
			element = SUPERCAST(element->BASE_EL(left));
	}
	else {
		/* Go up to parent until we were just a left child. */
		while ( true ) {
			Element *last = element;
			element = SUPERCAST(element->BASE_EL(parent));
			if ( element == 0 || element->BASE_EL(left) == last )
				break;
		}
	}
	return element;
}

/* Move back one. */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::Iter::
		findPrev( Element *element )
{
	/* Try to go left once then infinite right. */
	if ( element->BASE_EL(left) != 0 ) {
		element = element->BASE_EL(left);
		while ( element->BASE_EL(right) != 0 )
			element = SUPERCAST(element->BASE_EL(right));
	}
	else {
		/* Go up to parent until we were just a left child. */
		while ( true ) {
			Element *last = element;
			element = SUPERCAST(element->BASE_EL(parent));
			if ( element == 0 || element->BASE_EL(right) == last )
				break;
		}
	}
	return element;
}

#endif


/**
 * \brief Insert an existing element into the tree. 
 *
 * If the insert succeeds and lastFound is given then it is set to the element
 * inserted. If the insert fails then lastFound is set to the existing element in
 * the tree that has the same key as element. If the element's avl pointers are
 * already in use then undefined behaviour results.
 * 
 * \returns The element inserted upon success, null upon failure.
 */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::
		insert( Element *element, Element **lastFound )
{
	long keyRelation;
	Element *curEl = SUPERCAST(root), *parentEl = 0;
	Element *lastLess = 0;

	while (true) {
		if ( curEl == 0 ) {
			/* We are at an external element and did not find the key we were
			 * looking for. Attach underneath the leaf and rebalance. */
			attachRebal( element, parentEl, lastLess );

			if ( lastFound != 0 )
				*lastFound = element;
			return element;
		}

		keyRelation = compare( element->BASEKEY(getKey()), 
				curEl->BASEKEY(getKey()) );

		/* Do we go left? */
		if ( keyRelation < 0 ) {
			parentEl = lastLess = curEl;
			curEl = SUPERCAST(curEl->BASE_EL(left));
		}
		/* Do we go right? */
		else if ( keyRelation > 0 ) {
			parentEl = curEl;
			curEl = SUPERCAST(curEl->BASE_EL(right));
		}
		/* We have hit the target. */
		else {
			if ( lastFound != 0 )
				*lastFound = curEl;
			return 0;
		}
	}
}

/**
 * \brief Insert a new element into the tree with given key.
 *
 * If the key is not already in the tree then a new element is made using the
 * Element(const Key &key) constructor and the insert succeeds. If lastFound is
 * given then it is set to the element inserted. If the insert fails then
 * lastFound is set to the existing element in the tree that has the same key as
 * element.
 * 
 * \returns The new element upon success, null upon failure.
 */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::
		insert( const Key &key, Element **lastFound )
{
	long keyRelation;
	Element *curEl = SUPERCAST(root), *parentEl = 0;
	Element *lastLess = 0;

	while (true) {
		if ( curEl == 0 ) {
			/* We are at an external element and did not find the key we were
			 * looking for. Create the new element, attach underneath the leaf and
			 * rebalance. */
			Element *element = new Element( key );
			attachRebal( element, parentEl, lastLess );

			if ( lastFound != 0 )
				*lastFound = element;
			return element;
		}

		keyRelation = compare( key, curEl->BASEKEY(getKey()) );

		/* Do we go left? */
		if ( keyRelation < 0 ) {
			parentEl = lastLess = curEl;
			curEl = SUPERCAST(curEl->BASE_EL(left));
		}
		/* Do we go right? */
		else if ( keyRelation > 0 ) {
			parentEl = curEl;
			curEl = SUPERCAST(curEl->BASE_EL(right));
		}
		/* We have hit the target. */
		else {
			if ( lastFound != 0 )
				*lastFound = curEl;
			return 0;
		}
	}
}

#if defined( AVLTREE_MAP )
/**
 * \brief Insert a new element into the tree with key and value. 
 *
 * If the key is not already in the tree then a new element is constructed and
 * the insert succeeds. If lastFound is given then it is set to the element
 * inserted. If the insert fails then lastFound is set to the existing element in
 * the tree that has the same key as element. This insert routine is only
 * available in TAvlMap because it is the only class that knows about a Value
 * type.
 * 
 * \returns The new element upon success, null upon failure.
 */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::
		insert( const Key &key, const Value &val, Element **lastFound )
{
	long keyRelation;
	Element *curEl = SUPERCAST(root), *parentEl = 0;
	Element *lastLess = 0;

	while (true) {
		if ( curEl == 0 ) {
			/* We are at an external element and did not find the key we were
			 * looking for. Create the new element, attach underneath the leaf and
			 * rebalance. */
			Element *element = new Element( key, val );
			attachRebal( element, parentEl, lastLess );

			if ( lastFound != 0 )
				*lastFound = element;
			return element;
		}

		keyRelation = compare( key, curEl->getKey() );

		/* Do we go left? */
		if ( keyRelation < 0 ) {
			parentEl = lastLess = curEl;
			curEl = SUPERCAST(curEl->BASE_EL(left));
		}
		/* Do we go right? */
		else if ( keyRelation > 0 ) {
			parentEl = curEl;
			curEl = SUPERCAST(curEl->BASE_EL(right));
		}
		/* We have hit the target. */
		else {
			if ( lastFound != 0 )
				*lastFound = curEl;
			return 0;
		}
	}
}
#endif


/**
 * \brief Find a element in the tree with the given key.
 *
 * \returns The element if key exists, null if the key does not exist.
 */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::
		find(const Key &key) const
{
	Element *curEl = SUPERCAST(root);
	long keyRelation;

	while (curEl) {
		keyRelation = compare( key, curEl->BASEKEY(getKey()) );

		/* Do we go left? */
		if ( keyRelation < 0 )
			curEl = SUPERCAST(curEl->BASE_EL(left));
		/* Do we go right? */
		else if ( keyRelation > 0 )
			curEl = SUPERCAST(curEl->BASE_EL(right));
		/* We have hit the target. */
		else {
			return curEl;
		}
	}
	return 0;
}

/**
 * \brief Find a element, then detach it from the tree. 
 * 
 * The element is not deleted.
 *
 * \returns The element detached if the key is found, othewise returns null.
 */
template <AVLMEL_TEMPDEF> Element *TAvlTree<AVLMEL_TEMPUSE>::
		detach(const Key &key)
{
	Element *element = find( key );
	if ( element != 0 )
		BaseTree::detach( BASECAST(element) );

	return element;
}

/**
 * \brief Find, detach and delete a element from the tree. 
 *
 * \returns True if the element was found and deleted, false otherwise.
 */
template <AVLMEL_TEMPDEF> bool TAvlTree<AVLMEL_TEMPUSE>::
		remove(const Key &key)
{
	/* Assume not found. */
	bool retVal = false;

	/* Look for the key. */
	Element *element = find( key );
	if ( element != 0 ) {
		/* If found, detach the element and delete. */
		BaseTree::detach( BASECAST(element) );
		delete element;
		retVal = true;
	}

	return retVal;
}

/**
 * \brief Detach and delete a element from the tree. 
 *
 * If the element is not in the tree then undefined behaviour results.
 */
template <AVLMEL_TEMPDEF> void TAvlTree<AVLMEL_TEMPUSE>::
		remove( Element *element )
{
	/* Detach and delete. */
	BaseTree::detach( BASECAST(element) );
	delete element;
}

#ifdef AAPL_NAMESPACE
}
#endif
