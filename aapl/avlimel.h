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

#ifndef _AAPL_AVLIMEL_H
#define _AAPL_AVLIMEL_H

#include "compare.h"
#include "dlistmel.h"

/**
 * \addtogroup avltree 
 * @{
 */

/**
 * \class AvliMel
 * \brief Linked AVL tree for element appearing in multiple trees.
 *
 * AvliMel allows for a element to simultaneously be an element in multiple trees
 * without the two trees interferring with one another. This is achieved by
 * multiple inheritence. The element must use the same key for each tree. The
 * AvliMel class requires that you specify to it how to resolve the ambiguities
 * between the multiple AvlTreeEl classes that the element will inherit from. This
 * is done with the BaseEl parameter.
 *
 * AvliMel implicitly connects element with linked list pointers, allowing the
 * element to be walked in order using next and previous pointers.
 *
 * AvliMel does not explicitly manage memory for element. Elements inserted into
 * the tree are typically allocated by the user. Since element can have static
 * allocation, the tree does not assume ownership of the element. The destructor
 * will not delete element. A deep copy will cause existing elements to be
 * abandoned.
 *
 * \include ex_avlimel.cpp
 */

/*@}*/

#define BASE_EL(name) BaseEl::name
#define BASEKEY(name) name
#define BASELIST DListMel< Element, BaseEl >
#define AVLMEL_CLASSDEF class Element, class Key, \
		class BaseEl, class Compare = CmpOrd<Key>
#define AVLMEL_TEMPDEF class Element, class Key, \
		class BaseEl, class Compare
#define AVLMEL_TEMPUSE Element, Key, BaseEl, Compare
#define AvlTree AvliMel
#define WALKABLE

#include "avlcommon.h"

#undef BASE_EL
#undef BASEKEY
#undef BASELIST
#undef AVLMEL_CLASSDEF
#undef AVLMEL_TEMPDEF
#undef AVLMEL_TEMPUSE
#undef AvlTree
#undef WALKABLE

#endif /* _AAPL_AVLIMEL_H */
