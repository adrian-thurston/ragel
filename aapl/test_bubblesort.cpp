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

#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>
#include "vector.h"
#include "bubblesort.h"
#include "compare.h"

using namespace std;

void testBubbleSort1()
{
	Vector<int> v;
	BubbleSort< int, CmpOrd<int> > bubbleSort;

	int numItems = 1 + (random() % 100);
	for ( int i = 0; i < numItems; i++ )
		v.append(random() % 100);


	bubbleSort.sort( v.data, v.tabLen );
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
void testBubbleSort2()
{
	Data data[TEST2_SIZE];
	BubbleSort<Data, Data> bubbleSort;

	for ( int i = 0; i < TEST2_SIZE; i++ ) {
		data[i].data[0] = drand48();
		data[i].data[1] = drand48();
		data[i].data[2] = drand48();
		data[i].data[3] = drand48();
	}

	bubbleSort.sort( data, TEST2_SIZE );
}

#define TEST3_SIZE 20000
void testBubbleSort3()
{
	static int data[TEST3_SIZE];
	BubbleSort< int, CmpOrd<int> > bubbleSort;

	for ( int i = 0; i < TEST3_SIZE; i++ ) 
		data[i] = random();

	bubbleSort.sort( data, TEST3_SIZE );
}

int main()
{
	srandom( time(0) );
	srand48( time(0) );
	testBubbleSort1();
	return 0;
}

