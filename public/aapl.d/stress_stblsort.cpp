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

#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include "vector.h"
#include "mergesort.h"
#include "quicksort.h"
#include "insertsort.h"
#include "bubblesort.h"
#include "compare.h"

using namespace std;

#define TEST_SIZE 2000
void processArgs( int argc, char** argv );

struct TwoDouble
{
	double d1;
	double d2;
};

struct TdCmp1
{
	static inline int compare(TwoDouble &td1, TwoDouble &td2)
		{ return CmpOrd<double>::compare(td1.d1, td2.d1); }
};

struct TdCmp2
{
	static inline int compare(TwoDouble &td1, TwoDouble &td2)
		{ return CmpOrd<double>::compare(td1.d2, td2.d2); }
};

int main( int argc, char **argv )
{
	processArgs( argc, argv );
	srandom( time(0) );
	srand48( time(0) );

	static TwoDouble data1[TEST_SIZE];
	static TwoDouble data2[TEST_SIZE];
	int round = 0;

	cout << "round        0";
	cout.flush();

	while ( true ) {
		/* Choose the first double. It has a more restricted range than
		 * the second double so that we get lots of structs with the same
		 * first key. This help in testing the stable-sort. */
		switch ( random() % 3 ) {
		case 0:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i].d1 = data2[i].d1 = random() % 100;
			break;
		case 1:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i].d1 = data2[i].d1 = i % 100;
			break;
		case 2:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i].d1 = data2[i].d1 = (TEST_SIZE-i) % 100;
			break;
		}

		/* Choose the second double. */
		switch ( random() % 3 ) {
		case 0:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i].d2 = data2[i].d2 = random();
			break;
		case 1:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i].d2 = data2[i].d2 = i;
			break;
		case 2:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i].d2 = data2[i].d2 = TEST_SIZE-i;
			break;
		}

		MergeSort< TwoDouble, TdCmp2 > ms1;
		MergeSort< TwoDouble, TdCmp1 > ms2;
		BubbleSort< TwoDouble, TdCmp2 > bs1;
		BubbleSort< TwoDouble, TdCmp1 > bs2;

		ms1.sort( data1, TEST_SIZE );
		ms2.sort( data1, TEST_SIZE );
		bs1.sort( data2, TEST_SIZE );
		bs2.sort( data2, TEST_SIZE );
		
		assert( memcmp( data1, data2, sizeof(TwoDouble)*TEST_SIZE ) == 0 );

		/* Test the stable sort. */
		for ( int i = 1; i < TEST_SIZE; i++ ) {
			if ( data1[i-1].d1 == data1[i].d1 )
				assert( data1[i-1].d2 <= data1[i].d2 );
		}

		/* Test the stable sort. */
		for ( int i = 1; i < TEST_SIZE; i++ ) {
			if ( data2[i-1].d1 == data2[i].d1 )
				assert( data2[i-1].d2 <= data2[i].d2 );
		}

		cout << "\b\b\b\b\b\b\b\b" << setw(8) << round++;
		cout.flush();
	}
}
