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

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

#if defined( AVLTREE_MEL )
#	define BASE_EL(name) BaseEl::name
#	define BASEKEY(name) name
#	define AVL_CLASSDEF class Element, class Key, \
		class BaseEl, class Compare = CmpOrd<Key>
#	define AVL_TEMPDEF class Element, class Key, \
		class BaseEl, class Compare
#	define AVL_TEMPUSE Element, Key, BaseEl, Compare
#	if defined( WALKABLE )
#		define AvlTreeVer AvliMelVer
#		define AvlTree AvliMel
#		define BASELIST DListMel< Element, BaseEl >
#	else
#		define AvlTreeVer AvlMelVer
#		define AvlTree AvlMel
#	endif
#elif defined( AVLTREE_MELKEY )
#	define BASE_EL(name) BaseEl::name
#	define BASEKEY(name) BaseKey::name
#	define AVL_CLASSDEF class Element, class Key, class BaseEl, \
		class BaseKey, class Compare = CmpOrd<Key>
#	define AVL_TEMPDEF class Element, class Key, class BaseEl, \
		class BaseKey, class Compare
#	define AVL_TEMPUSE Element, Key, BaseEl, BaseKey, Compare
#	if defined( WALKABLE )
#		define AvlTreeVer AvliMelKeyVer
#		define AvlTree AvliMelKey
#		define BASELIST DListMel< Element, BaseEl >
#	else
#		define AvlTreeVer AvlMelKeyVer
#		define AvlTree AvlMelKey
#	endif
#elif defined( AVLTREE_SINGULAR )
#	define BASE_EL(name) name
#	define BASEKEY(name) name
#	define AVL_CLASSDEF class Element, class Key, class Compare = CmpOrd<Key>
#	define AVL_TEMPDEF class Element, class Key, class Compare
#	define AVL_TEMPUSE Element, Key, Compare
#	if defined( WALKABLE )
#		define AvlTreeVer AvliTreeVer
#		define AvlTree AvliTree
#		define BASELIST DListMel< Element, AvliTreeEl<Element> >
#	else
#		define AvlTreeVer AvlTreeVer
#		define AvlTree AvlTree
#	endif
#elif defined( AVLTREE_MAP )
#	define BASE_EL(name) name
#	define BASEKEY(name) name
#	define AVL_CLASSDEF class Key, class Value, class Compare = CmpOrd<Key>
#	define AVL_TEMPDEF class Key, class Value, class Compare
#	define AVL_TEMPUSE Key, Value, Compare
#	if defined( WALKABLE )
#		define AvlTreeVer AvliMapVer
#		define AvlTree AvliMap
#		define Element AvliMapEl<Key,Value>
#		define BASELIST DList< AvliMapEl<Key,Value> >
#	else
#		define AvlTreeVer AvlMapVer
#		define AvlTree AvlMap
#		define Element AvlMapEl<Key,Value>
#	endif
#elif defined( AVLTREE_SET )
#	define BASE_EL(name) name
#	define BASEKEY(name) name
#	define AVL_CLASSDEF class Key, class Compare = CmpOrd<Key>
#	define AVL_TEMPDEF class Key, class Compare
#	define AVL_TEMPUSE Key, Compare
#	if defined( WALKABLE )
#		define AvlTreeVer AvliSetVer
#		define AvlTree AvliSet
#		define Element AvliSetEl<Key>
#		define BASELIST DList< AvliSetEl<Key> >
#	else
#		define AvlTreeVer AvlSetVer
#		define AvlTree AvlSet
#		define Element AvlSetEl<Key>
#	endif
#else
#	error "tree type not specified: this is not a user header"
#endif


/* AvlTree with verification routines. */
template < AVL_CLASSDEF > class AvlTreeVer 
		: public AvlTree<AVL_TEMPUSE>
{
public:
	typedef AvlTree<AVL_TEMPUSE> BaseTree;
#ifdef WALKABLE
	typedef BASELIST BaseList;
#else
	typedef AvlTree<AVL_TEMPUSE> BaseList;
#endif

	/* For debugging/testing. Verify the tree properties. */
	void verifyIntegrity() const;

	/* For debugging/testing. Verify the tree properties. */
	void verifyIntegrity(Element *element, Element *parent) const;
};

/* For debugging/testing. Verify the tree properties. */
template <AVL_TEMPDEF> void AvlTreeVer<AVL_TEMPUSE>::
		verifyIntegrity() const
{
	Element *cur, *last;

	/* Verify the properties of the element. */
	verifyIntegrity(BaseTree::root, 0);

	/* Make sure the element are connected in proper order. */
#ifdef WALKABLE
	assert( BaseList::listLen == BaseTree::treeSize );
	/* Walk the list of element, assert the ordering. */
	if ( BaseTree::treeSize > 0 ) {
		Element *prev = BaseList::head, *cur = BaseList::head->BASE_EL(next);
		while ( cur != 0 ) {
			assert( compare( prev->BASEKEY(getKey()), 
				cur->BASEKEY(getKey()) ) < 0 );

			/* If there is no parent, then the element should be the root. */
			if ( cur->BASE_EL(parent) == 0 )
				assert( BaseTree::root == cur );

			prev = cur;
			cur = cur->BASE_EL(next);
		}
	}
#endif

	/* Verify that the head is indeed the head. */
	last = 0, cur = BaseList::head;
	while ( cur != 0 ) {
		assert( cur->BASE_EL(left) == last );
		last = cur;
		cur = cur->BASE_EL(parent);
	}

	/* Verify that the tail is indeed the tail. */
	last = 0, cur = BaseList::tail;
	while ( cur != 0 ) {
		assert( cur->BASE_EL(right) == last );
		last = cur;
		cur = cur->BASE_EL(parent);
	}
}

/* Verify the balance property of the avl tree Verify binary tree propery as
 * well as parent/child pointers are correct. */
template <AVL_TEMPDEF> void AvlTreeVer<AVL_TEMPUSE>::
		verifyIntegrity(Element *element, Element *parent) const
{
	int lheight, rheight, balanceProp;
	if ( element == 0 )
		return;

	lheight = element->BASE_EL(left) ? 
			element->BASE_EL(left)->BASE_EL(height) : 0;
	rheight = element->BASE_EL(right) ? 
			element->BASE_EL(right)->BASE_EL(height) : 0;

	balanceProp = lheight - rheight;

	assert( -1 <= balanceProp && balanceProp <= 1 );
	assert( element->BASE_EL(parent) == parent);

	if ( element->BASE_EL(left) ) {
		assert( compare( element->BASEKEY(getKey()), 
				element->BASE_EL(left)->BASEKEY(getKey()) ) > 0 );
	}

	if ( element->BASE_EL(right) ) {
		assert( compare( element->BASEKEY(getKey()),
				element->BASE_EL(right)->BASEKEY(getKey()) ) < 0 );
	}

	verifyIntegrity(element->BASE_EL(left), element);
	verifyIntegrity(element->BASE_EL(right), element);
	return;
}

/* namespace Aapl */
#ifdef AAPL_NAMESPACE
}
#endif

