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

