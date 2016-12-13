/*
 * Copyright 2002, 2003 Adrian Thurston <thurston@colm.net>
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

#ifndef _AAPL_AVLIKEYLESS_H
#define _AAPL_AVLIKEYLESS_H

#include "compare.h"
#include "dlistmel.h"

/**
 * \addtogroup avlitree
 * @{
 */

/**
 * \class AvliKeyless
 * \brief Linked AVL tree that has no insert/find/remove functions that take a
 * key.
 *
 * AvliKeyless is an implementation of the AVL tree rebalancing functionality
 * only. It provides the common code for the tiny AVL tree implementations.
 */

/*@}*/

#define BASE_EL(name) name
#define BASELIST DListMel< Element, AvliTreeEl<Element> >
#define AVLMEL_CLASSDEF class Element
#define AVLMEL_TEMPDEF class Element
#define AVLMEL_TEMPUSE Element
#define AvlTree AvliKeyless
#define WALKABLE
#define AVL_KEYLESS

#include "avlcommon.h"

#undef BASE_EL
#undef BASELIST
#undef AVLMEL_CLASSDEF
#undef AVLMEL_TEMPDEF
#undef AVLMEL_TEMPUSE
#undef AvlTree
#undef WALKABLE
#undef AVL_KEYLESS

#endif /* _AAPL_AVLIKEYLESS_H */
