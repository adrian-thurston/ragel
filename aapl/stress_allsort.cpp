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

void processArgs( int argc, char** argv );

#define TEST_SIZE 2000
int main( int argc, char **argv )
{
	processArgs( argc, argv );
	srandom( time(0) );
	srand48( time(0) );

	static int data1[TEST_SIZE];
	static int data2[TEST_SIZE];
	static int data3[TEST_SIZE];
	static int data4[TEST_SIZE];
	int round = 0;

	cout << "round        0";
	cout.flush();

	while ( true ) {
		switch ( random() % 3 ) {
		case 0:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i] = data2[i] = data3[i] = data4[i] = random();
			break;
		case 1:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i] = data2[i] = data3[i] = data4[i] = i;
			break;
		case 2:
			for ( int i = 0; i < TEST_SIZE; i++ ) 
				data1[i] = data2[i] = data3[i] = data4[i] = TEST_SIZE-i;
			break;
		}

		InsertSort< int, CmpOrd<int> > is;
		BubbleSort< int, CmpOrd<int> > bs;
		QuickSort< int, CmpOrd<int> > qs;
		MergeSort< int, CmpOrd<int> > ms;

		qs.sort( data1, TEST_SIZE );
		ms.sort( data2, TEST_SIZE );
		is.sort( data3, TEST_SIZE );
		bs.sort( data4, TEST_SIZE );
		
		assert( memcmp( data1, data2, sizeof(int)*TEST_SIZE ) == 0 );
		assert( memcmp( data1, data3, sizeof(int)*TEST_SIZE) == 0 );
		assert( memcmp( data1, data4, sizeof(int)*TEST_SIZE) == 0 );

		cout << "\b\b\b\b\b\b\b\b" << setw(8) << round++;
		cout.flush();
	}
}

