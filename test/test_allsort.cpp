/*
 *  Copyright 2002 Adrian Thurston <thurston@cs.queensu.ca>
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

