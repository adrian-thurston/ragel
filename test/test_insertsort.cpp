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

#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>
#include "vector.h"
#include "insertsort.h"
#include "compare.h"

using namespace std;

void testInsertSort1()
{
	Vector<int> v;
	InsertSort< int, CmpOrd<int> > is;

	int numItems = 1 + (random() % 100);
	for ( int i = 0; i < numItems; i++ )
		v.append(random() % 100);

	is.sort( v.data, v.tabLen );
	int *tel = v.data;
	int tlen = v.tabLen;
	for ( int i = 0; i < tlen; i++, tel++ )
		cout << *tel << endl;
}

struct Data
{
	Data() { }
	Data(const Data&) { cout << __PRETTY_FUNCTION__ << endl; }
	~Data() { }
	Data &operator=(const Data&) { cout << __PRETTY_FUNCTION__ << endl; return *this; }

	double data[4];

	static inline int compare( const Data &d1, const Data &d2 )
	{
		return memcmp( d1.data, d2.data, sizeof(d1.data) );
	}
};

#define TEST2_SIZE 1000
void testInsertSort2()
{
	Data data[TEST2_SIZE];
	InsertSort<Data, Data> is;

	for ( int i = 0; i < TEST2_SIZE; i++ ) {
		data[i].data[0] = drand48();
		data[i].data[1] = drand48();
		data[i].data[2] = drand48();
		data[i].data[3] = drand48();
	}

	is.sort( data, TEST2_SIZE );
}

#define TEST3_SIZE 20000
void testInsertSort3()
{
	static int data[TEST3_SIZE];
	InsertSort< int, CmpOrd<int> > is;

	for ( int i = 0; i < TEST3_SIZE; i++ ) 
		data[i] = random();

	is.sort( data, TEST3_SIZE );
}

int main()
{
	srandom( time(0) );
	srand48( time(0) );
	testInsertSort3();
	return 0;
}

