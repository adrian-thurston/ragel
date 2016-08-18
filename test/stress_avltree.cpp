/*
 *  Copyright 2002, 2003 Adrian Thurston <thurston@cs.queensu.ca>
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
#include <iostream>
#include <stdio.h>

#include "avltree.h"

#define AVLTREE_SINGULAR
#include "avlverify.h"
#undef AVLTREE_SINGULAR

#include "util.h"

/* Having the action change period larger than the number of initial entries
 * means that it will take longer to get to all the cases, but all the
 * cases will be more thoroughly tested. The tree will be more likely to
 * empty out completely and fill up with all of the entries. */
#define INITIAL_ENTRIES 64831
#define ACTION_CHANGE_PERIOD 120233
#define VERIFY_PERIOD 1119
#define COPY_PERIOD 1351
#define INCREMENT_VARIATION 10
#define STATS_PERIOD 211
#define OUTBUFSIZE 100

using namespace std;

/* Test element. */
struct TreeEl : public AvlTreeEl<TreeEl>
{
	TreeEl() : inTree(false) { }
	TreeEl(const int key) : key(key), inTree(false) { }

	int getKey() { return key; }
	int key;
	bool inTree;
};

/* Instantiate all the code. */
template class AvlTree< TreeEl, int >;
template class AvlTreeVer< TreeEl, int >;

int increment = 0;
int curIndex = 0;
int action = 1;
int curRound = 0;


/* Write out stats. */
void writeStats( int treeSize, TreeEl *root )
{
	static char buf[OUTBUFSIZE] = { 0 };
	char tmpBuf[OUTBUFSIZE];

	if ( curRound % STATS_PERIOD == 0 ) {
		/* Replace the contents of the buffer with backspaces and clear the line. */
		memset( buf, '\b', strlen(buf) );
		cout << buf;

		/* Write the new stats line. */
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

int main( int argc, char **argv )
{
	processArgs( argc, argv );
	srandom( time(0) );

	/* Make the tree and element. */
	AvlTreeVer< TreeEl, int > tree;
	TreeEl *allElements = new TreeEl[INITIAL_ENTRIES/2];
	for ( int element = 0; element < (INITIAL_ENTRIES/2); element++ )
		allElements[element].key = element;
	
	char buf[OUTBUFSIZE];
	expandTab( buf, "round\tinc\tins\trem\tindex\tels\theight" );
	cout << buf << endl;

	for ( curRound = 0; true; curRound++ ) {
		/* Do we change our action? */
		if ( curRound % ACTION_CHANGE_PERIOD == 0 ) {
			increment = random() % 2;
			if ( increment > 0 )
				increment = random() % INCREMENT_VARIATION;

			action = (random()%3) + 1;
		}

		/* Maybe show some stats. */
		writeStats( tree.treeSize, tree.root );

		/* Insert one? */
		if ( action&0x1 ) {
			newIndex();
			if ( curIndex < (INITIAL_ENTRIES/2) ) {
				/* Insert from the pool of existing element. */
				if ( ! allElements[curIndex].inTree ) {
					tree.insert( allElements+curIndex );
					allElements[curIndex].inTree = true;
				}
			}
			else {
				/* Insert a new element the version of insert that will create
				 * the node for us. */
				tree.insert( curIndex );
			}
		}

		/* Delete one? */
		if ( action&0x2 ) {
			newIndex();
			if ( curIndex < (INITIAL_ENTRIES/2) ) {
				/* Delete from the pool of existing entries. */
				if ( allElements[curIndex].inTree ) {
					tree.detach( allElements+curIndex );
					allElements[curIndex].inTree = false;
				}
			}
			else {
				/* Delete an element that was newed. */
				TreeEl *element = tree.detach( curIndex );
				if ( element != 0 )
					delete element;
			}
		}

		/* Verify? */
		if ( curRound % VERIFY_PERIOD == 0 ) {
			tree.verifyIntegrity();
			for ( int element = 0; element < (INITIAL_ENTRIES/2); element++ ) {
				TreeEl *res = tree.find( allElements[element].key );
				assert( allElements[element].inTree == (res != 0) );
			}
		}

		/* Test the deep copy? */
		if ( curRound % COPY_PERIOD == 0 ) {
			AvlTreeVer< TreeEl, int > copy( tree );
			copy.verifyIntegrity();
			copy.empty();
		}
	}	
	return 0;
}
