/*
 *  Copyright 2001, 2003 Adrian Thurston <adriant@ragel.ca>
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
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include "avlmap.h"

#define AVLTREE_MAP
#include "avlverify.h"
#undef AVLTREE_MAP

/* Having the action change perion larger than the number of initial entries
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
void processArgs( int argc, char** argv );
void expandTab( char *dst, char *src );

/* Instantiate the entire tree. */
template class AvlMap< int, int >;
template class AvlMapVer< int, int >;

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
void printStats( int treeSize, AvlMapEl<int,int> *head )
{
	/* Print stats. */
	static char buf[OUTBUFSIZE] = { 0 };
	char tmpBuf[OUTBUFSIZE];

	if ( curRound % STATS_PERIOD == 0 ) {
		memset( buf, '\b', strlen(buf) );
		cout << buf;
		sprintf( tmpBuf, "%i\t%i\t%s\t%s\t%i\t%i\t%i\t", curRound, increment,
				action&0x1 ? "yes" : "no", action&0x2 ? "yes" : "no", 
				curIndex, treeSize, head ? head->height : 0 );
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


int main( int argc, char **argv )
{
	processArgs( argc, argv );
	srandom( time(0) );

	typedef AvlMapEl< int, int > TreeEl;
	AvlMapVer< int, int > tree;

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
			tree.insert( curIndex, curIndex );
		}

		/* Delete one? */
		if ( action&0x2 ) {
			newIndex();
			TreeEl *element = tree.detach( curIndex );
			if ( element != 0 )
				delete element;
		}

		/* Verify? */
		if ( curRound % VERIFY_PERIOD == 0 )
			tree.verifyIntegrity();

		/* Test the deep copy? */
		if ( curRound % COPY_PERIOD == 0 ) {
			AvlMapVer< int, int > copy(tree);
			copy.verifyIntegrity();
			copy.empty();
		}

	}	
	return 0;
}
