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

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "avltree.h"
#include "avlitree.h"
#include "avlset.h"
#include "avliset.h"

#define AVLTREE_SINGULAR
#include "avlverify.h"
#undef AVLTREE_SINGULAR

using namespace std;

/* Having the action change perion larger than the number of initial entries
 * means that it will take longer to get to all the cases, but all the
 * cases will be more thoroughly tested. The tree will be more likely to
 * empty out completely and fill up with all of the entries. */
#define INITIAL_ENTRIES 64831
#define ACTION_CHANGE_PERIOD 120233
#define VERIFY_PERIOD 1119
#define COPY_PERIOD 1351
#define WALK_PERIOD 113
#define INCREMENT_VARIATION 10
#define STATS_PERIOD 211
#define OUTBUFSIZE 100

/* Test element. */
struct TreeEl : public AvlTreeEl<TreeEl>
{
	TreeEl() : inTree(false) { }
	TreeEl(const int key) : key(key), inTree(false) { }

	int getKey() { return key; }
	int key;
	bool inTree;
};

/* Test element. */
struct ShadowTreeEl : public AvliTreeEl<ShadowTreeEl>
{
	ShadowTreeEl() : inTree(false) { }
	ShadowTreeEl(const int key) : key(key), inTree(false) { }

	int getKey() { return key; }
	int key;
	bool inTree;
};

/* Instantiate the entire tree. */
template class AvlTree< TreeEl, int >;
template class AvlTreeVer< TreeEl, int >;

/* This is the shadow tree that we will use to test the iterator against. It
 * maintains the next/prev pointers and so we assume it to be correct. */
template class AvliTree< ShadowTreeEl, int >;

int increment = 0;
int curIndex = 0;
int action = 1;
int curRound = 0;

void ExpandTab(char *buf, char *dstBuf)
{
	int pos = 10;
	char *src = buf;
	char *dst = dstBuf;

	while ( *src != 0 ) {
		if ( *src == '\t' ) {
			*dst++ = ' ';
			while ( dst - dstBuf < pos )
				*dst++ = ' ';
			pos += 10;
		}
		else
			*dst++ = *src;
		src++;
	}
	*dst = 0;
}

/* Replace the current stats line with new stats. For one tree. */
void PrintStats( int treeSize, TreeEl *root )
{
	/* Print stats. */
	char buf1[OUTBUFSIZE];
	char buf2[OUTBUFSIZE];

	if ( curRound % STATS_PERIOD == 0 ) {
		memset( buf1, '\b', OUTBUFSIZE );
		fwrite( buf1, 1, OUTBUFSIZE, stdout );
		sprintf(buf1, "%i\t%i\t%s\t%s\t%i\t%i\t%li\t", curRound, increment,
				action&0x1 ? "yes" : "no", action&0x2 ? "yes" : "no", 
				curIndex, treeSize, root ? root->height : 0 );
		ExpandTab(buf1, buf2);
		fputs(buf2, stdout);
		fflush(stdout);
	}
}

/* Find a new curIndex to use. If the increment is 0 then get
 * a random curIndex. Otherwise use the increment. */
void NewIndex()
{
	if ( increment == 0 )
		curIndex = random() % INITIAL_ENTRIES;
	else
		curIndex = (curIndex + increment) % INITIAL_ENTRIES;
}

/* Print the header to the stats. For one tree. */
void printHeader()
{
	char buf[OUTBUFSIZE];
	ExpandTab( "round\tinc\tins\trem\tindex\tels\theight\n", buf);
	fputs(buf, stdout);
}

std::ostream &operator <<(std::ostream &out, TreeEl &element)
{
	out << element.key;
	return out;
}

void randomWalkTest( AvlTreeVer<TreeEl, int> &tree, 
		AvliTree< ShadowTreeEl, int > &shadowTree )
{
	/* Randomly choose a walk type. */
	int wt = random() % 4;
	if ( wt == 0 ) {
		/* Walk forward. */
		ShadowTreeEl *st_el = shadowTree.head;
		AvlTree<TreeEl, int>::Iter it = tree.first();
		for ( ; it.lte(); it++, st_el = st_el->next )
			assert ( it->key == st_el->key );

		/* If one is done, the other should be too. */
		assert ( st_el == 0 );
	}
	else if ( wt == 1 ) {
		/* Walk backwards. */
		ShadowTreeEl *st_el = shadowTree.tail;
		AvlTree<TreeEl, int>::Iter it = tree.last();
		for ( ; it.gtb(); it--, st_el = st_el->prev )
			assert ( it->key == st_el->key );

		/* If one is done, the other should be too. */
		assert ( st_el == 0 );
	}
	else if ( wt >= 2 ) {
		/* Walk to the middle then wiggle around some. */
		ShadowTreeEl *st_el = shadowTree.head;
		AvlTree<TreeEl, int>::Iter it = tree.first();
		for ( int i = 0; i < tree.treeSize/2; i++ ) {
			assert ( it->key == st_el->key );
			it.increment();
			st_el = st_el->next;
		}

		/* Wiggle around the size of the tree times. */
		for ( int i = 0; i < tree.treeSize; i++ ) {
			/* How far to go with this wiggle? */
			int dist = random() % 10;

			if ( wt == 2 ) {
				/* Go forward some. */
				for ( int j = 0; j < dist && it.gtb() && it.lte(); j++ )
				{
					assert ( it->key == st_el->key );
					it.increment();
					st_el = st_el->next;
				}
			}
			else {
				/* Go backwards some. */
				for ( int j = 0; j < dist && it.gtb() && it.lte(); j++ )
				{
					assert ( it->key == st_el->key );
					it.decrement();
					st_el = st_el->prev;
				}
			}

			if ( it.beg() || it.end() ) {
				/* If one is done, the other should be too. */
				assert ( st_el == 0 );
				break;
			}
		}
	}
}

typedef AvlSet< int > Tree;
typedef AvliSet< int > TreeI;

void testIterNotWalkable()
{
	Tree tree;

	cout << "NOT WALKABLE" << endl;
	cout << "Iterator Default Constructor" << endl;

	/* Default constructed iterator. */
	Tree::Iter defIt;
	cout << defIt.lte() << " " << defIt.end() << " " << defIt.gtb() << " " << 
			defIt.beg() << " " << defIt.first() << " " << defIt.last() << endl;

	cout << "Zero Items" << endl;

	/* Iterator constructed from empty tree. */
	Tree::Iter i1(tree);
	cout << i1.lte() << " " << i1.end() << " " << i1.gtb() << " " << 
			i1.beg() << " " << i1.first() << " " << i1.last() << endl;
	Tree::Iter i2(tree.first());
	cout << i2.lte() << " " << i2.end() << " " << i2.gtb() << " " << 
			i2.beg() << " " << i2.first() << " " << i2.last() << endl;
	Tree::Iter i3(tree.last());
	cout << i3.lte() << " " << i3.end() << " " << i3.gtb() << " " << 
			i3.beg() << " " << i3.first() << " " << i3.last() << endl;
	
	/* Iterator assigned from an empty tree. */
	i1 = tree;
	cout << i1.lte() << " " << i1.end() << " " << i1.gtb() << " " << 
			i1.beg() << " " << i1.first() << " " << i1.last() << endl;
	i2 = tree.first();
	cout << i2.lte() << " " << i2.end() << " " << i2.gtb() << " " << 
			i2.beg() << " " << i2.first() << " " << i2.last() << endl;
	i3 = tree.last();
	cout << i3.lte() << " " << i3.end() << " " << i3.gtb() << " " << 
			i3.beg() << " " << i3.first() << " " << i3.last() << endl;

	cout << "One Item" << endl;
	tree.insert( 1 );

	/* Iterator constructed from an a tree with one item. */
	Tree::Iter i4(tree);
	cout << i4.lte() << " " << i4.end() << " " << i4.gtb() << " " << 
			i4.beg() << " " << i4.first() << " " << i4.last() << endl;
	Tree::Iter i5(tree.first());
	cout << i5.lte() << " " << i5.end() << " " << i5.gtb() << " " << 
			i5.beg() << " " << i5.first() << " " << i5.last() << endl;
	Tree::Iter i6(tree.last());
	cout << i6.lte() << " " << i6.end() << " " << i6.gtb() << " " << 
			i6.beg() << " " << i6.first() << " " << i6.last() << endl;

	/* Iterator assigned from an a tree with one item. */
	i4 = tree;
	cout << i4.lte() << " " << i4.end() << " " << i4.gtb() << " " << 
			i4.beg() << " " << i4.first() << " " << i4.last() << endl;
	i5 = tree.first();
	cout << i5.lte() << " " << i5.end() << " " << i5.gtb() << " " << 
			i5.beg() << " " << i5.first() << " " << i5.last() << endl;
	i6 = tree.last();
	cout << i6.lte() << " " << i6.end() << " " << i6.gtb() << " " << 
			i6.beg() << " " << i6.first() << " " << i6.last() << endl;

	cout << "Two Items" << endl;
	tree.insert( 2 );

	/* Iterator constructed from an a tree with two items. */
	Tree::Iter i7(tree);
	cout << i7.lte() << " " << i7.end() << " " << i7.gtb() << " " << 
			i7.beg() << " " << i7.first() << " " << i7.last() << endl;
	Tree::Iter i8(tree.first());
	cout << i8.lte() << " " << i8.end() << " " << i8.gtb() << " " << 
			i8.beg() << " " << i8.first() << " " << i8.last() << endl;
	Tree::Iter i9(tree.last());
	cout << i9.lte() << " " << i9.end() << " " << i9.gtb() << " " << 
			i9.beg() << " " << i9.first() << " " << i9.last() << endl;

	/* Iterator assigned from an a tree with two items. */
	i7 = tree;
	cout << i7.lte() << " " << i7.end() << " " << i7.gtb() << " " << 
			i7.beg() << " " << i7.first() << " " << i7.last() << endl;
	i8 = tree.first();
	cout << i8.lte() << " " << i8.end() << " " << i8.gtb() << " " << 
			i8.beg() << " " << i8.first() << " " << i8.last() << endl;
	i9 = tree.last();
	cout << i9.lte() << " " << i9.end() << " " << i9.gtb() << " " << 
			i9.beg() << " " << i9.first() << " " << i9.last() << endl;

	tree.empty();
	tree.insert( 1 );
	tree.insert( 2 );
	tree.insert( 3 );
	tree.insert( 4 );

	cout << "test1" << endl;
	Tree::Iter it1 = tree;
	for ( ; it1.lte(); it1++ )
		cout << it1->key << endl;

	cout << "test2" << endl;
	Tree::Iter it2 = tree.first();
	for ( ; it2.lte(); it2++ )
		cout << it2->key << endl;

	cout << "test3" << endl;
	Tree::Iter it3 = tree.last();
	for ( ; it3.gtb(); it3-- )
		cout << it3->key << endl;

	cout << "test4" << endl;
	it1 = tree;
	for ( ; !it1.end(); it1++ )
		cout << it1->key << endl;

	cout << "test5" << endl;
	it2 = tree.first();
	for ( ; !it2.end(); it2++ )
		cout << it2->key << endl;

	cout << "test6" << endl;
	it3 = tree.last();
	for ( ; !it3.beg(); it3-- )
		cout << it3->key << endl;
}

void testIterWalkable()
{
	TreeI tree;

	cout << "WALKABLE" << endl;
	cout << "Iterator Default Constructor" << endl;

	/* Default constructed iterator. */
	TreeI::Iter defIt;
	cout << defIt.lte() << " " << defIt.end() << " " << defIt.gtb() << " " << 
			defIt.beg() << " " << defIt.first() << " " << defIt.last() << endl;

	cout << "Zero Items" << endl;

	/* Iterator constructed from empty tree. */
	TreeI::Iter i1(tree);
	cout << i1.lte() << " " << i1.end() << " " << i1.gtb() << " " << 
			i1.beg() << " " << i1.first() << " " << i1.last() << endl;
	TreeI::Iter i2(tree.first());
	cout << i2.lte() << " " << i2.end() << " " << i2.gtb() << " " << 
			i2.beg() << " " << i2.first() << " " << i2.last() << endl;
	TreeI::Iter i3(tree.last());
	cout << i3.lte() << " " << i3.end() << " " << i3.gtb() << " " << 
			i3.beg() << " " << i3.first() << " " << i3.last() << endl;
	
	/* Iterator assigned from an empty tree. */
	i1 = tree;
	cout << i1.lte() << " " << i1.end() << " " << i1.gtb() << " " << 
			i1.beg() << " " << i1.first() << " " << i1.last() << endl;
	i2 = tree.first();
	cout << i2.lte() << " " << i2.end() << " " << i2.gtb() << " " << 
			i2.beg() << " " << i2.first() << " " << i2.last() << endl;
	i3 = tree.last();
	cout << i3.lte() << " " << i3.end() << " " << i3.gtb() << " " << 
			i3.beg() << " " << i3.first() << " " << i3.last() << endl;

	cout << "One Item" << endl;
	tree.insert( 1 );

	/* Iterator constructed from an a tree with one item. */
	TreeI::Iter i4(tree);
	cout << i4.lte() << " " << i4.end() << " " << i4.gtb() << " " << 
			i4.beg() << " " << i4.first() << " " << i4.last() << endl;
	TreeI::Iter i5(tree.first());
	cout << i5.lte() << " " << i5.end() << " " << i5.gtb() << " " << 
			i5.beg() << " " << i5.first() << " " << i5.last() << endl;
	TreeI::Iter i6(tree.last());
	cout << i6.lte() << " " << i6.end() << " " << i6.gtb() << " " << 
			i6.beg() << " " << i6.first() << " " << i6.last() << endl;

	/* Iterator assigned from an a tree with one item. */
	i4 = tree;
	cout << i4.lte() << " " << i4.end() << " " << i4.gtb() << " " << 
			i4.beg() << " " << i4.first() << " " << i4.last() << endl;
	i5 = tree.first();
	cout << i5.lte() << " " << i5.end() << " " << i5.gtb() << " " << 
			i5.beg() << " " << i5.first() << " " << i5.last() << endl;
	i6 = tree.last();
	cout << i6.lte() << " " << i6.end() << " " << i6.gtb() << " " << 
			i6.beg() << " " << i6.first() << " " << i6.last() << endl;

	cout << "Two Items" << endl;
	tree.insert( 2 );

	/* Iterator constructed from an a tree with two items. */
	TreeI::Iter i7(tree);
	cout << i7.lte() << " " << i7.end() << " " << i7.gtb() << " " << 
			i7.beg() << " " << i7.first() << " " << i7.last() << endl;
	TreeI::Iter i8(tree.first());
	cout << i8.lte() << " " << i8.end() << " " << i8.gtb() << " " << 
			i8.beg() << " " << i8.first() << " " << i8.last() << endl;
	TreeI::Iter i9(tree.last());
	cout << i9.lte() << " " << i9.end() << " " << i9.gtb() << " " << 
			i9.beg() << " " << i9.first() << " " << i9.last() << endl;

	/* Iterator assigned from an a tree with two items. */
	i7 = tree;
	cout << i7.lte() << " " << i7.end() << " " << i7.gtb() << " " << 
			i7.beg() << " " << i7.first() << " " << i7.last() << endl;
	i8 = tree.first();
	cout << i8.lte() << " " << i8.end() << " " << i8.gtb() << " " << 
			i8.beg() << " " << i8.first() << " " << i8.last() << endl;
	i9 = tree.last();
	cout << i9.lte() << " " << i9.end() << " " << i9.gtb() << " " << 
			i9.beg() << " " << i9.first() << " " << i9.last() << endl;

	tree.insert( 4 );
	tree.insert( 3 );
	tree.insert( 2 );
	tree.insert( 1 );

	cout << "test1" << endl;
	TreeI::Iter it1 = tree;
	for ( ; it1.lte(); it1++ )
		cout << it1->key << endl;

	cout << "test2" << endl;
	TreeI::Iter it2 = tree.first();
	for ( ; it2.lte(); it2++ )
		cout << it2->key << endl;

	cout << "test3" << endl;
	TreeI::Iter it3 = tree.last();
	for ( ; it3.gtb(); it3-- )
		cout << it3->key << endl;

	cout << "test4" << endl;
	it1 = tree;
	for ( ; !it1.end(); it1++ )
		cout << it1->key << endl;

	cout << "test5" << endl;
	it2 = tree.first();
	for ( ; !it2.end(); it2++ )
		cout << it2->key << endl;

	cout << "test6" << endl;
	it3 = tree.last();
	for ( ; !it3.beg(); it3-- )
		cout << it3->key << endl;
}


int main()
{
	testIterWalkable();
	testIterNotWalkable();
	return 0;
}
