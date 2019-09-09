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

#include "util.h"

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
#define TAB_WIDTH 10

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


/* Replace the current stats line with new stats. For one tree. */
void printStats( int treeSize, TreeEl *root )
{
	/* Print stats. */
	static char buf[OUTBUFSIZE] = { 0 };
	char tmpBuf[OUTBUFSIZE];

	if ( curRound % STATS_PERIOD == 0 ) {
		memset( buf, '\b', strlen(buf) );
		cout << buf;

		sprintf( tmpBuf, "%i\t%i\t%s\t%s\t%i\t%i\t%li\t", curRound, increment,
				action&0x1 ? "yes" : "no", action&0x2 ? "yes" : "no", 
				curIndex, treeSize, root ? root->height : 0 );
		expandTab( buf, tmpBuf );
		cout << buf;
		cout.flush();
	}
}

/* Find a new curIndex to use. If the increment is 0 then get
 * a random curIndex. Otherwise use the increment. */
void newIndex()
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
	expandTab( buf, "round\tinc\tins\trem\tindex\tels\theight" );
	cout << buf << endl;
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

int main( int argc, char **argv )
{
	processArgs( argc, argv );
	srandom( time(0) );

	/* Make the tree and element. */
	AvlTreeVer< TreeEl, int > tree;
	TreeEl *allElements = new TreeEl[INITIAL_ENTRIES/2];
	for ( int element = 0; element < (INITIAL_ENTRIES/2); element++ )
		allElements[element].key = element;
	
	/* Make the shadow tree and element. */
	AvliTree< ShadowTreeEl, int > shadowTree;
	ShadowTreeEl *allShadowEls = new ShadowTreeEl[INITIAL_ENTRIES/2];
	for ( int element = 0; element < (INITIAL_ENTRIES/2); element++ )
		allShadowEls[element].key = element;
	
	printHeader();

	for ( curRound = 0; true; curRound++ ) {
		/* Do we change our action? */
		if ( curRound % ACTION_CHANGE_PERIOD == 0 ) {
			increment = random() % 2;
			if ( increment > 0 )
				increment = random() % INCREMENT_VARIATION;

			action = (random()%3) + 1;
		}

		/* Dump stats. */
		printStats( tree.treeSize, tree.root );

		/* Insert one? */
		if ( action&0x1 ) {
			newIndex();
			if ( curIndex < (INITIAL_ENTRIES/2) ) {
				/* Insert from the pool of existing element. */
				if ( ! allElements[curIndex].inTree ) {
					/* Do the insert for the primary tree. */
					tree.insert( allElements+curIndex );
					allElements[curIndex].inTree = true;

					/* Now insert same data for shadow tree. */
					shadowTree.insert( allShadowEls+curIndex );
					allShadowEls[curIndex].inTree = true;
				}
			}
			else {
				/* Insert a new element in both main and shadow. */
				tree.insert( curIndex );
				shadowTree.insert( curIndex );
			}
		}

		/* Delete one? */
		if ( action&0x2 ) {
			newIndex();
			if ( curIndex < (INITIAL_ENTRIES/2) ) {
				/* Delete from the pool of existing entries. */
				if ( allElements[curIndex].inTree ) {
					/* Detach for primary element. */
					tree.detach( allElements+curIndex );
					allElements[curIndex].inTree = false;

					/* Detach for shadow tree. */
					shadowTree.detach( allShadowEls+curIndex );
					allShadowEls[curIndex].inTree = false;
				}
			}
			else {
				/* Delete an element that was newed. */
				TreeEl *element = tree.detach( curIndex );
				if ( element != 0 )
					delete element;

				/* Delete for the shadow tree. */
				ShadowTreeEl *st_el = shadowTree.detach( curIndex );
				if ( st_el != 0 )
					delete st_el;
			}
		}

		/* Verify? */
		if ( curRound % VERIFY_PERIOD == 0 )
			tree.verifyIntegrity();

		/* Test the deep copy? */
		if ( curRound % COPY_PERIOD == 0 ) {
			AvlTreeVer< TreeEl, int > copy( tree );
			copy.verifyIntegrity();
			randomWalkTest(copy, shadowTree);
			copy.empty();
		}

		/* Walk both trees concurrently and verify matching contents. */
		if ( curRound % WALK_PERIOD == 0 ) {
			randomWalkTest(tree, shadowTree);
		}
	}	

	return 0;
}
