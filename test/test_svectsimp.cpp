/*
 * Copyright 2002 Adrian Thurston <thurston@colm.net>
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
