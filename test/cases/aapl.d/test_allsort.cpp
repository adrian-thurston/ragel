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
#include <iomanip>
#include <vector>
#include "vector.h"
#include "mergesort.h"
#include "quicksort.h"
#include "insertsort.h"
#include "bubblesort.h"
#include "compare.h"

using namespace std;

struct TwoDouble
{
	double d1;
	double d2;
};

struct TdCmpData
{
	TdCmpData() : toInt(false) { }

	inline int compare( TwoDouble &td1, TwoDouble &td2 )
	{
		return toInt ? 
			CmpOrd<double>::compare(td1.d1, td2.d1) :
			CmpOrd<double>::compare(td1.d1, td2.d1);
	}
	bool toInt;
};


int testNonStaticCompare()
{
	static TwoDouble data[3];
	data[0].d1 = 2;
	data[1].d1 = 1.5;
	data[2].d1 = 1;

	BubbleSort< TwoDouble, TdCmpData > bs;
	MergeSort< TwoDouble, TdCmpData > ms;
	InsertSort< TwoDouble, TdCmpData > is;
	QuickSort< TwoDouble, TdCmpData > qs;

	bs.toInt = true;
	bs.sort( data, 3 );
	cout << data[0].d1 << " " << data[1].d1 << " " << data[2].d1 << endl;

	ms.toInt = true;
	ms.sort( data, 3 );
	cout << data[0].d1 << " " << data[1].d1 << " " << data[2].d1 << endl;

	is.toInt = true;
	is.sort( data, 3 );
	cout << data[0].d1 << " " << data[1].d1 << " " << data[2].d1 << endl;

	qs.toInt = false;
	qs.sort( data, 3 );
	cout << data[0].d1 << " " << data[1].d1 << " " << data[2].d1 << endl;

	return 0;
}

int main()
{
	testNonStaticCompare();
	return 0;
}

