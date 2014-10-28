/*
 *  Copyright 2001 Adrian Thurston <thurston@complang.org>
 */

#ifndef _AAPL_DLIST_H
#define _AAPL_DLIST_H

#define BASE_EL(name) name
#define DLMEL_TEMPDEF class Element
#define DLMEL_TEMPUSE Element
#define DList DList

/**
 * \addtogroup dlist
 * @{
 */

/**
 * \class DList
 * \brief Basic doubly linked list.
 *
 * DList is the standard by-structure list type. This class requires the
 * programmer to declare a list element type that has the necessary next and
 * previous pointers in it. This can be achieved by inheriting from the
 * DListEl class or by simply adding next and previous pointers directly into
 * the list element class.
 *
 * DList does not assume ownership of elements in the list. If the elements
 * are known to reside on the heap, the provided empty() routine can be used to
 * delete all elements, however the destructor will not call this routine, it
 * will simply abandon all the elements. It is up to the programmer to
 * explicitly de-allocate items when necessary.
 *
 * \include ex_dlist.cpp
 */

/*@}*/

#include "dlcommon.h"

#undef BASE_EL
#undef DLMEL_TEMPDEF
#undef DLMEL_TEMPUSE
#undef DList

#endif /* _AAPL_DLIST_H */

