/*
 * Copyright 2002 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _AAPL_AVLITREE_H
#define _AAPL_AVLITREE_H

#include "compare.h"
#include "dlistmel.h"

/**
 * \addtogroup avlitree
 * @{
 */

/**
 * \class AvliTree
 * \brief Linked AVL tree.
 *
 * AvliTree is the standard linked by-structure AVL tree. To use this
 * structure the user must define an element type and give it the necessary
 * properties. At the very least it must have a getKey() function that will be
 * used to compare the relative ordering of elements and tree management data
 * necessary for the AVL algorithm. An element type can acquire the management
 * data by inheriting the AvliTreeEl class.
 *
 * AvliTree does not presume to manage the allocation of elements in the tree.
 * The destructor will not delete the items in the tree, instead the elements
 * must be explicitly de-allocated by the user if necessary and when it is
 * safe to do so. The empty() routine will traverse the tree and delete all
 * items. 
 *
 * Since the tree does not manage the elements, it can contain elements that
 * are allocated statically or that are part of another data structure.
 *
 * \include ex_avlitree.cpp
 */

/*@}*/

#define BASE_EL(name) name
#define BASEKEY(name) name
#define BASELIST DListMel< Element, AvliTreeEl<Element> >
#define AVLMEL_CLASSDEF class Element, class Key, class Compare = CmpOrd<Key>
#define AVLMEL_TEMPDEF class Element, class Key, class Compare
#define AVLMEL_TEMPUSE Element, Key, Compare
#define AvlTree AvliTree
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

#endif /* _AAPL_AVLITREE_H */
