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
}



int main()
{
	testString1();
	return 0;
}
