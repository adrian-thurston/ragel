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

#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "vectsimp.h"
#include "svectsimp.h"

using namespace std;

template class SVectSimp<char>;

void testVectSimp1()
{
	SVectSimp< char > buffer;
	while ( ! cin.eof() ) {
		char c;
		cin >> c;
		buffer.append(c);
	}
}

void testSVectSimp2()
{
	SVectSimp<char> b;
	for (int i = 0; i < 1000000; i++ )
		b.append("age writes code ", 12);
}

/* Data structure whos constructors and destructor count callings. */
struct TheData
{
	TheData() : i(0) { }
	TheData(int i) : i(i) { }
	TheData(const TheData &d) : i(1) { }
	~TheData() { }

	int i;
};

bool operator==(const TheData &td1, const TheData &td2)
	{ return td1.i == td2.i; }

#define LEN_THRESH 1000
#define STATS_PERIOD 211
#define OUTBUFSIZE 1000
//#define NUM_ROUNDS 1e9
#define NUM_ROUNDS 100000
int curRound = 0;

void expandTab( char *dst, char *src );

/* Print a statistics header. */
void printHeader()
{
	char buf[OUTBUFSIZE];
	expandTab( buf, "round\tlength" );
	cout << buf << endl;
}


/* Replace the current stats line with new stats. For one tree. */
void printStats( VectSimp<TheData> &v1 )
{
	/* Print stats. */
	static char buf[OUTBUFSIZE] = { 0 };
	char tmpBuf[OUTBUFSIZE];

	memset( buf, '\b', strlen(buf) );
	cout << buf;
	sprintf( tmpBuf, "%i\t%li\t", curRound, v1.length() );
	expandTab( buf, tmpBuf );
	cout << buf;
	cout.flush();
}

int main()
{
	testSVectSimp2();
	return 0;
}
