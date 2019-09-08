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

#ifndef _AAPL_AVLIBASIC_H
#define _AAPL_AVLIBASIC_H

#include "compare.h"

/**
 * \addtogroup avlitree 
 * @{
 */

/**
 * \class AvliBasic
 * \brief Linked AVL Tree in which the entire element structure is the key.
 *
 * AvliBasic is a linked AVL tree that does not distinguish between the
 * element that it contains and the key. The entire element structure is the
 * key that is used to compare the relative ordering of elements. This is
 * similar to the BstSet structure.
 *
 * AvliBasic does not assume ownership of elements in the tree. Items must be
 * explicitly de-allocated.
 */

/*@}*/

#define BASE_EL(name) name
#define BASEKEY(name) name
#define AVLMEL_CLASSDEF class Element, class Compare
#define AVLMEL_TEMPDEF class Element, class Compare
#define AVLMEL_TEMPUSE Element, Compare
#define AvlTree AvliBasic
#define AVL_BASIC
#define WALKABLE

#include "avlcommon.h"

#undef BASE_EL
#undef BASEKEY
#undef AVLMEL_CLASSDEF
#undef AVLMEL_TEMPDEF
#undef AVLMEL_TEMPUSE
#undef AvlTree
#undef AVL_BASIC
#undef WALKABLE

#endif /* _AAPL_AVLIBASIC_H */
