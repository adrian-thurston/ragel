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

using std::cout;
using std::endl;

#include "bstmap.h"

typedef BstMap< const char *, int, CmpStr > Map;

void testBstMap1()
{
	cout << "TEST 1" << endl;
	Map m1( "hi there", 1 );
	Map m2( "friend" );

	cout << m1.data[0].key << endl;
	cout << m2.data[0].key << endl;
}

void testBstMap2()
{
	cout << "TEST 2" << endl;
	Map table1( "hi" );
	cout << table1[0].key << endl;

	Map table2( "there", 1  );
	cout << table2[0].key << endl;
	cout << table2[0].value << endl;
}


int main()
{
	testBstMap1();
	testBstMap2();
	return 0;
}
