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

#ifndef _AAPL_AVLMAP_H
#define _AAPL_AVLMAP_H

#include "compare.h"

/**
 * \addtogroup avltree 
 * @{
 */

/**
 * \class AvlMap
 * \brief Key and value oriented AVL tree. 
 *
 * AvlMap stores key and value pairs in elements that managed by the tree. It
 * is intendend to be similar to map template found in the STL. AvlMap
 * requires that a Key type, a Value type, and a class containing a compare()
 * routine for Key be given. Items can be inserted with just a key or with a
 * key and value pair.
 *
 * AvlMap assumes all elements in the tree are allocated on the heap and are
 * to be managed by the tree. This means that the class destructor will delete
 * the contents of the tree. A deep copy will cause existing elements to be
 * deleted first.
 *
 * \include ex_avlmap.cpp
 */

/*@}*/

#define AVLTREE_MAP
#define BASE_EL(name) name
#define BASEKEY(name) name
#define AVLMEL_CLASSDEF class Key, class Value, class Compare = CmpOrd<Key>
#define AVLMEL_TEMPDEF class Key, class Value, class Compare
#define AVLMEL_TEMPUSE Key, Value, Compare
#define AvlTree AvlMap
#define Element AvlMapEl<Key,Value>

#include "avlcommon.h"

#undef AVLTREE_MAP
#undef BASE_EL
#undef BASEKEY
#undef AVLMEL_CLASSDEF
#undef AVLMEL_TEMPDEF
#undef AVLMEL_TEMPUSE
#undef AvlTree
#undef Element



#endif /* _AAPL_AVLMAP_H */
