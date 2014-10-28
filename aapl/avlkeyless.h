/*
 *  Copyright 2002, 2003 Adrian Thurston <thurston@complang.org>
 */

#ifndef _AAPL_AVLKEYLESS_H
#define _AAPL_AVLKEYLESS_H

#include "compare.h"

/**
 * \addtogroup avltree 
 * @{
 */

/**
 * \class AvlKeyless
 * \brief AVL tree that has no insert/find/remove functions that take a key.
 *
 * AvlKeyless is an implementation of the AVL tree rebalancing functionality
 * only. It provides the common code for the tiny AVL tree implementations.
 */

/*@}*/

#define BASE_EL(name) name
#define AVLMEL_CLASSDEF class Element
#define AVLMEL_TEMPDEF class Element
#define AVLMEL_TEMPUSE Element
#define AvlTree AvlKeyless
#define AVL_KEYLESS

#include "avlcommon.h"

#undef BASE_EL
#undef AVLMEL_CLASSDEF
#undef AVLMEL_TEMPDEF
#undef AVLMEL_TEMPUSE
#undef AvlTree
#undef AVL_KEYLESS

#endif /* _AAPL_AVLKEYLESS_H */
