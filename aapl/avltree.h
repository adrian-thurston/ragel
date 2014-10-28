/*
 *  Copyright 2002 Adrian Thurston <thurston@complang.org>
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
 * AvlTree is the standard by-structure AVL tree. To use this structure the
 * user must define an element type and give it the necessary properties. At
 * the very least it must have a getKey() function that will be used to
 * compare the relative ordering of elements and tree management data
 * necessary for the AVL algorithm. An element type can acquire the management
 * data by inheriting the AvlTreeEl class.
 *
 * AvlTree does not presume to manage the allocation of elements in the tree.
 * The destructor will not delete the items in the tree, instead the elements
 * must be explicitly de-allocated by the user if necessary and when it is
 * safe to do so. The empty() routine will traverse the tree and delete all
 * items. 
 *
 * Since the tree does not manage the elements, it can contain elements that
 * are allocated statically or that are part of another data structure.
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
