/*
 *  Copyright 2003 Adrian Thurston <adriant@ragel.ca>
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
#include "tsvectsimp.h"
#include "svectsimp.h"
#include "svector.h"

using namespace std;

struct POD1 { POD1(int i, int j) : i(i),j(j) {} int i, j; };
struct POD2 { POD2(int i, int j) : i(i),j(j) {} int i, j; };
struct POD3 { POD3(int i, int j) : i(i),j(j) {} int i, j; };
struct POD4 { POD4(int i, int j) : i(i),j(j) {} int i, j; };
struct POD5 { POD5(int i, int j) : i(i),j(j) {} int i, j; };
struct POD6 { POD6(int i, int j) : i(i),j(j) {} int i, j; };

#define TINY_VECT

#ifdef TINY_VECT
typedef TSVectSimp<POD1> Vect1;
typedef TSVectSimp<POD2> Vect2;
typedef TSVectSimp<POD3> Vect3;
typedef TSVectSimp<POD4> Vect4;
typedef TSVectSimp<POD5> Vect5;
typedef TSVectSimp<POD6> Vect6;
#else
typedef SVectSimp<POD1> Vect1;
typedef SVectSimp<POD2> Vect2;
typedef SVectSimp<POD3> Vect3;
typedef SVectSimp<POD4> Vect4;
typedef SVectSimp<POD5> Vect5;
typedef SVectSimp<POD6> Vect6;
#endif

//template TSVectSimp<POD1>;
//template TSVectSimp<POD2>;
//template TSVectSimp<POD3>;
//template TSVectSimp<POD4>;
//template TSVectSimp<POD5>;
//template TSVectSimp<POD6>;

void testTSVectSimp1()
{
	POD1 pod(1,2);
	Vect1 b;
	for (int i = 0; i < 1000000; i++ )
		b.append(pod);
	for ( Vect1::Iter i = b.first(); i.lte(); i++ )
		i->i = i->j + 1;
}

void testTSVectSimp2()
{
	POD2 pod(3,4);
	Vect2 b;
	for (int i = 0; i < 1000000; i++ )
		b.append(pod);
	for ( Vect2::Iter i = b.first(); i.lte(); i++ )
		i->i = i->j + 1;
}

void testTSVectSimp3()
{
	POD3 pod(5,6);
	Vect3 b;
	for (int i = 0; i < 1000000; i++ )
		b.append(pod);
	for ( Vect3::Iter i = b.first(); i.lte(); i++ )
		i->i = i->j + 1;
}

void testTSVectSimp4()
{
	POD4 pod(7,8);
	Vect4 b;
	for (int i = 0; i < 1000000; i++ )
		b.append(pod);
	for ( Vect4::Iter i = b.first(); i.lte(); i++ )
		i->i = i->j + 1;
}

void testTSVectSimp5()
{
	POD5 pod(9,10);
	Vect5 b;
	for (int i = 0; i < 1000000; i++ )
		b.append(pod);
	for ( Vect5::Iter i = b.first(); i.lte(); i++ )
		i->i = i->j + 1;
}

void testTSVectSimp6()
{
	POD6 pod(11,12);
	Vect6 b;
	for (int i = 0; i < 1000000; i++ )
		b.append(pod);
	for ( Vect6::Iter i = b.first(); i.lte(); i++ )
		i->i = i->j + 1;
}

int main()
{
	testTSVectSimp1();
	testTSVectSimp2();
	testTSVectSimp3();
	testTSVectSimp4();
	testTSVectSimp5();
	testTSVectSimp6();
	return 0;
}
