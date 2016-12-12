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

#ifndef _AAPL_DLISTVAL_H
#define _AAPL_DLISTVAL_H

/**
 * \addtogroup dlist
 * @{
 */

/**
 * \class DListVal
 * \brief By-value doubly linked list.
 *
 * This class is a doubly linked list that does not require a list element
 * type to be declared. The user instead gives a type that is to be stored in
 * the list element. When inserting a new data item, the value is copied into
 * a newly allocated element. This list is inteded to behave and be utilized
 * like the list template found in the STL.
 *
 * DListVal is different from the other lists in that it allocates elements
 * itself. The raw element insert interface is still exposed for convenience,
 * however, the list assumes all elements in the list are allocated on the
 * heap and are to be managed by the list. The destructor WILL delete the
 * contents of the list. If the list is ever copied in from another list, the
 * existing contents are deleted first. This is in contrast to DList and
 * DListMel, which will never delete their contents to allow for statically
 * allocated elements.
 *
 * \include ex_dlistval.cpp
 */

/*@}*/

#define BASE_EL(name) name
#define DLMEL_TEMPDEF class T
#define DLMEL_TEMPUSE T
#define DList DListVal
#define Element DListValEl<T>
#define DOUBLELIST_VALUE

#include "dlcommon.h"

#undef BASE_EL
#undef DLMEL_TEMPDEF
#undef DLMEL_TEMPUSE
#undef DList
#undef Element
#undef DOUBLELIST_VALUE

#endif /* _AAPL_DLISTVAL_H */

