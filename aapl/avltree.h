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

#ifndef _AAPL_AVLTREE_H
#define _AAPL_AVLTREE_H

#include "compare.h"

/**
 * \addtogroup avltree 
 * @{
 */

/**
 * \class AvlTree
 * \brief Basic AVL tree.
 *
 * AvlTree is the standard by-structure AVL tree. A element class and a key type
 * must be given. Elements can be inserted by specifying a key only or by
 * explicitly creating the element and then inserting it. The element type must have
 * the appropriate AVL element pointers. This can be achieved by inheriting from
 * the AvlTreeEl class.
 *
 * AvlTree does not explicitly manage memory for element. Elements inserted into
 * the tree can be allocated by the user or can be allocated by the tree. In
 * any case, the tree does not assume ownership of the element. The destructor
 * will not delete element. A deep copy will cause existing elements to be
 * abandoned.
 *
 * \include ex_avltree.cpp
 */

/*@}*/

#define BASE_EL(name) name
#define BASEKEY(name) name
#define AVLMEL_CLASSDEF class Element, class Key, class Compare = CmpOrd<Key>
#define AVLMEL_TEMPDEF class Element, class Key, class Compare
#define AVLMEL_TEMPUSE Element, Key, Compare
#define AvlTree AvlTree

#include "avlcommon.h"

#undef BASE_EL
#undef BASEKEY
#undef AVLMEL_CLASSDEF
#undef AVLMEL_TEMPDEF
#undef AVLMEL_TEMPUSE
#undef AvlTree

#endif /* _AAPL_AVLTREE_H */
