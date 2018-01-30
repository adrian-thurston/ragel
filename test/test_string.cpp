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
#include "astring.h"

using namespace std;

template class StrTmpl<char>;

void testString1()
{
	String str1 = "hello";
	String str2 = "my";
	String str3 = "friend";

	String fmt( 5, "%.7f", 1.3/2.7 );
	String final = str1 + " there " + str2 + " " + str3;

	cout << String( 50, "%.7f", 1.3/2.7 ) << endl;
	cout << final << endl;
	cout << fmt << " " << fmt.length() << endl;

	String str4 = "";
	StringStream ss( str4 );

	ss << 0.0000000011111111 << endl;
	cout << str4;
	cout.flush();
}

int main()
{
	testString1();
	return 0;
}
