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
 * This class is a by-value list that does not require a list element type to
 * be declared. When adding list items, the user can supply the data type and
 * the list will implicitly new up a list element. This behaviour is intended
 * to be similar to a standard list template implementation.
 *
 * DListVal is different from the other lists in that it assumes all elements
 * in the list are allocated on the heap and are to be managed by the list.
 * This means that the class destructor will delete the contents of the list.
 * If the list is ever copied in from another list, the existing contents are
 * deleted first. The other lists will never delete contents to allow for
 * statically allocated elements.
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

