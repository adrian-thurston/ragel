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
#include "vectsimp.h"
#include "vector.h"

using namespace std;

template class VectSimp<char>;

void testVectSimp1()
{
	VectSimp< char > buffer;
	while ( ! cin.eof() ) {
		char c;
		cin >> c;
		buffer.append(c);
	}
}

void testVectSimp2()
{
	VectSimp<char> b;
	for (int i = 0; i < 1000000; i++ )
		b.append("age writes code ", 12);
}

int main()
{
	testVectSimp2();
	return 0;
}
