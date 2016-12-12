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

#ifndef _AAPL_SBSTMAP_H
#define _AAPL_SBSTMAP_H

#include "compare.h"
#include "svector.h"

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \brief Element for BstMap.
 *
 * Stores the key and value pair. 
 */
template <class Key, class Value> struct SBstMapEl
{
	SBstMapEl() {}
	SBstMapEl(const Key &key) : key(key) {}
	SBstMapEl(const Key &key, const Value &val) : key(key), value(val) {}

	/** \brief The key */
	Key key;

	/** \brief The value. */
	Value value;
};

#ifdef AAPL_NAMESPACE
}
#endif

/**
 * \addtogroup bst 
 * @{
 */

/** 
 * \class SBstMap
 * \brief Copy-on-write binary search table for key and value pairs.
 *
 * This is a map style binary search table that employs the copy-on-write
 * mechanism for table data. BstMap stores key and value pairs in each
 * element. The key and value can be any type. A compare class for the key
 * must be supplied.
 */

/*@}*/

#define BST_TEMPL_DECLARE class Key, class Value, \
		class Compare = CmpOrd<Key>, class Resize = ResizeExpn
#define BST_TEMPL_DEF class Key, class Value, class Compare, class Resize
#define BST_TEMPL_USE Key, Value, Compare, Resize
#define GET_KEY(el) ((el).key)
#define BstTable SBstMap
#define Vector SVector
#define Table STable
#define Element SBstMapEl<Key, Value>
#define BSTMAP
#define SHARED_BST

#include "bstcommon.h"

#undef BST_TEMPL_DECLARE
#undef BST_TEMPL_DEF
#undef BST_TEMPL_USE
#undef GET_KEY
#undef BstTable
#undef Vector
#undef Table
#undef Element
#undef BSTMAP
#undef SHARED_BST

/**
 * \fn SBstMap::insert(const Key &key, BstMapEl<Key, Value> **lastFound)
 * \brief Insert the given key.
 *
 * If the given key does not already exist in the table then a new element
 * having key is inserted. They key copy constructor and value default
 * constructor are used to place the pair in the table. If lastFound is given,
 * it is set to the new entry created. If the insert fails then lastFound is
 * set to the existing pair of the same key.
 *
 * \returns The new element created upon success, null upon failure.
 */

/**
 * \fn SBstMap::insertMulti(const Key &key)
 * \brief Insert the given key even if it exists already.
 *
 * If the key exists already then the new element having key is placed next
 * to some other pair of the same key. InsertMulti cannot fail. The key copy
 * constructor and the value default constructor are used to place the pair in
 * the table.
 *
 * \returns The new element created.
 */

#endif /* _AAPL_SBSTMAP_H */
