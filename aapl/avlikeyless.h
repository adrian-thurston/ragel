/*
 *  Copyright 2002, 2003 Adrian Thurston <thurston@complang.org>
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
