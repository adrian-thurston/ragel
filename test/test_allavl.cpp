/*
 *  Copyright 2001 Adrian Thurston <thurston@cs.queensu.ca>
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

#include <stdio.h>

#include "avltree.h"
#include "avlmel.h"
#include "avlmelkey.h"
#include "avlmap.h"
#include "avlset.h"

#include "avlitree.h"
#include "avlimel.h"
#include "avlimelkey.h"
#include "avlimap.h"
#include "avliset.h"

/*
 * AvlTree
 */

struct TreeEl1 : public AvlTreeEl<TreeEl1>
{
	TreeEl1() : inTree(false) { }
	TreeEl1(const int key) : key(key), inTree(false) { }

	int getKey() { return key; }
	int key;
	bool inTree;
};

/* Instantiate the entire tree. */
template class AvlTree< TreeEl1, int >;

struct TreeEl2;
struct BaseEl2a : public AvlTreeEl< TreeEl2 > { };
struct BaseEl2b : public AvlTreeEl< TreeEl2 > { };

/*
 * AvlMel
 */
struct TreeEl2 : public BaseEl2a, public BaseEl2b
{
	TreeEl2() : inTree1(false), inTree2(false) { }
	TreeEl2(const int key) : key(key), inTree1(false), inTree2(false) { }

	const int &getKey() { return key; }

	int key;
	bool inTree1;
	bool inTree2;
};

template class AvlMel< TreeEl2, int, BaseEl2a >;
template class AvlMel< TreeEl2, int, BaseEl2b >;

/*
 * AvlMelKey
 */
struct TreeEl3;
struct BaseEl3a :
		public AvlTreeEl< TreeEl3 >
{
	int getKey() { return key; }
	int key;
};
struct BaseEl3b :
		public AvlTreeEl< TreeEl3 >
{
	char strKey[20];
	char *getKey() { return strKey; }
};

struct TreeEl3 :
		public BaseEl3a, 
		public BaseEl3b
{
	TreeEl3() : inTree1(false), inTree2(false) { }
	/* One for each tree. */
	TreeEl3(const int key) : inTree1(false), inTree2(false) { }
	TreeEl3(const char* key) : inTree1(false), inTree2(false) { }

	bool inTree1;
	bool inTree2;
};

template class AvlMelKey< TreeEl3, int, BaseEl3a, BaseEl3a >;
template class AvlMelKey< TreeEl3, char*, BaseEl3b, BaseEl3b, CmpStr >;

/*
 * AvlMap
 */
/* Instantiate the entire tree. */
template class AvlMap< int, int >;

/*
 * AvlSet
 */
/* Instantiate the entire tree. */
template class AvlSet< int >;

/*
 * AvliTree
 */

struct TreeEl6 : public AvliTreeEl<TreeEl6>
{
	TreeEl6() : inTree(false) { }
	TreeEl6(const int key) : key(key), inTree(false) { }

	int getKey() { return key; }
	int key;
	bool inTree;
};

/* Instantiate the entire tree. */
template class AvliTree< TreeEl6, int >;

struct TreeEl7;
struct BaseEl7a : public AvliTreeEl< TreeEl7 > { };
struct BaseEl7b : public AvliTreeEl< TreeEl7 > { };

/*
 * AvliMel
 */
struct TreeEl7 : public BaseEl7a, public BaseEl7b
{
	TreeEl7() : inTree1(false), inTree2(false) { }
	TreeEl7(const int key) : key(key), inTree1(false), inTree2(false) { }

	const int &getKey() { return key; }

	int key;
	bool inTree1;
	bool inTree2;
};

template class AvliMel< TreeEl7, int, BaseEl7a >;
template class AvliMel< TreeEl7, int, BaseEl7b >;

/*
 * AvliMelKey
 */
struct TreeEl8;
struct BaseEl8a :
		public AvliTreeEl< TreeEl8 >
{
	int getKey() { return key; }
	int key;
};
struct BaseEl8b :
		public AvliTreeEl< TreeEl8 >
{
	char strKey[20];
	char *getKey() { return strKey; }
};

struct TreeEl8 :
		public BaseEl8a, 
		public BaseEl8b
{
	TreeEl8() : inTree1(false), inTree2(false) { }
	/* One for each tree. */
	TreeEl8(const int key) : inTree1(false), inTree2(false) { }
	TreeEl8(const char* key) : inTree1(false), inTree2(false) { }

	bool inTree1;
	bool inTree2;
};

template class AvliMelKey< TreeEl8, int, BaseEl8a, BaseEl8a >;
template class AvliMelKey< TreeEl8, char*, BaseEl8b, BaseEl8b, CmpStr >;

/*
 * AvliMap
 */
/* Instantiate the entire tree. */
template class AvliMap< int, int >;

/*
 * AvliSet
 */
/* Instantiate the entire tree. */
template class AvliSet< int >;


int main()
{
	return 0;
}
