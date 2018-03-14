/*
 * Copyright 2001, 2002 Adrian Thurston <thurston@colm.net>
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

#include "bstset.h"
#include "avlset.h"
using namespace std;

template class BstSet< int, CmpOrd<int> >;
template class AvlSet< int, CmpOrd<int> >;

int main()
{
	BstSet< int, CmpOrd<int> > set1;
	AvlSet< int, CmpOrd<int> > set2;

	set1.insert(0);
	set2.insert(0);

	set1.insert(0);
	set2.insert(0);

	set1.insert(2);
	set2.insert(2);

	BstSet< int, CmpOrd<int> >::Iter it1 = set1;
	AvlSet< int, CmpOrd<int> >::Iter it2 = set2;
	for ( ; it1.lte(); it1++, it2++ )
		cout << *it1 << " " << it2->key << endl;

	for ( int i = 0; i < 20000; i++ )
		set1.insert( i );

	int num = 0;
	for ( int i = 0; i < 20000; i++ ) {
		BstSet< int, CmpOrd<int> >::Iter iter = set1;
		for ( ; !iter.end(); iter++ )
			num += *iter;
	}
	cout << num << endl;

	return 0;
}
