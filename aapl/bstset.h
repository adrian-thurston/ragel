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

#ifndef _AAPL_BSTSET_H
#define _AAPL_BSTSET_H

/**
 * \addtogroup bst 
 * @{
 */

/** 
 * \class BstSet
 * \brief Binary search table for types that are the key.
 *
 * BstSet is suitable for types that comprise the entire key. Rather than look
 * into the element to retrieve the key, the element is the key. A class that
 * contains a comparison routine for the key must be given.
 */

/*@}*/

#include "compare.h"
#include "vector.h"

#define BST_TEMPL_DECLARE class Key, class Compare = CmpOrd<Key>, \
		class Resize = ResizeExpn
#define BST_TEMPL_DEF class Key, class Compare, class Resize
#define BST_TEMPL_USE Key, Compare, Resize
#define GET_KEY(el) (el)
#define BstTable BstSet
#define Element Key
#define BSTSET

#include "bstcommon.h"

#undef BST_TEMPL_DECLARE
#undef BST_TEMPL_DEF
#undef BST_TEMPL_USE
#undef GET_KEY
#undef BstTable
#undef Element
#undef BSTSET

/**
 * \fn BstSet::insert(const Key &key, Key **lastFound)
 * \brief Insert the given key.
 *
 * If the given key does not already exist in the table then it is inserted.
 * The key's copy constructor is used to place the item in the table. If
 * lastFound is given, it is set to the new entry created. If the insert fails
 * then lastFound is set to the existing key of the same value.
 *
 * \returns The new element created upon success, null upon failure.
 */

/**
 * \fn BstSet::insertMulti(const Key &key)
 * \brief Insert the given key even if it exists already.
 *
 * If the key exists already then it is placed next to some other key of the
 * same value. InsertMulti cannot fail. The key's copy constructor is used to
 * place the item in the table.
 *
 * \returns The new element created.
 */

#endif /* _AAPL_BSTSET_H */
