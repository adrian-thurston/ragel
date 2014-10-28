/*
 *  Copyright 2002 Adrian Thurston <thurston@complang.org>
 */

#ifndef _AAPL_AVLISET_H
#define _AAPL_AVLISET_H

#include "compare.h"
#include "dlist.h"

/**
 * \addtogroup avlitree 
 * @{
 */

/**
 * \class AvliSet
 * \brief Linked Key-only oriented tree.
 *
 * AvliSet stores only keys in elements that are managed by the tree. AvliSet
 * requires that a Key type and a class containing a compare() routine
 * for Key be given. Items are inserted with just a key value.
 *
 * AvliSet assumes all elements in the tree are allocated on the heap and are
 * to be managed by the tree. This means that the class destructor will delete
 * the contents of the tree. A deep copy will cause existing elements to be
 * deleted first.
 *
 * \include ex_avliset.cpp
 */

/*@}*/

#define AVLTREE_SET
#define BASE_EL(name) name
#define BASEKEY(name) name
#define BASELIST DList< AvliSetEl<Key> >
#define AVLMEL_CLASSDEF class Key, class Compare = CmpOrd<Key>
#define AVLMEL_TEMPDEF class Key, class Compare
#define AVLMEL_TEMPUSE Key, Compare
#define AvlTree AvliSet
#define Element AvliSetEl<Key>
#define WALKABLE

#include "avlcommon.h"

#undef AVLTREE_SET
#undef BASE_EL
#undef BASEKEY
#undef BASELIST
#undef AVLMEL_CLASSDEF
#undef AVLMEL_TEMPDEF
#undef AVLMEL_TEMPUSE
#undef AvlTree
#undef Element
#undef WALKABLE

#endif /* _AAPL_AVLISET_H */
