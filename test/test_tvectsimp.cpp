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
#include "tvectsimp.h"
#include "vectsimp.h"
#include "vector.h"

using namespace std;

struct POD1 { POD1(int i, int j) : i(i),j(j) {} int i, j; };
struct POD2 { POD2(int i, int j) : i(i),j(j) {} int i, j; };
struct POD3 { POD3(int i, int j) : i(i),j(j) {} int i, j; };
struct POD4 { POD4(int i, int j) : i(i),j(j) {} int i, j; };
struct POD5 { POD5(int i, int j) : i(i),j(j) {} int i, j; };
struct POD6 { POD6(int i, int j) : i(i),j(j) {} int i, j; };

#define TINY_VECT

#ifdef TINY_VECT
typedef TVectSimp<POD1> Vect1;
typedef TVectSimp<POD2> Vect2;
typedef TVectSimp<POD3> Vect3;
typedef TVectSimp<POD4> Vect4;
typedef TVectSimp<POD5> Vect5;
typedef TVectSimp<POD6> Vect6;
#else
typedef VectSimp<POD1> Vect1;
typedef VectSimp<POD2> Vect2;
typedef VectSimp<POD3> Vect3;
typedef VectSimp<POD4> Vect4;
typedef VectSimp<POD5> Vect5;
typedef VectSimp<POD6> Vect6;
#endif

void testTVectSimp1()
{
	POD1 pod(1,2);
	Vect1 b;
	for (int i = 0; i < 1000000; i++ ) {
		pod.i = 1;
		b.append(pod);
	}
}

void testTVectSimp2()
{
	POD2 pod(3,4);
	Vect2 b;
	for (int i = 0; i < 1000000; i++ ) {
		pod.i = 1;
		b.append(pod);
	}
}

void testTVectSimp3()
{
	POD3 pod(5,6);
	Vect3 b;
	for (int i = 0; i < 1000000; i++ ) {
		pod.i = 1;
		b.append(pod);
	}
}

void testTVectSimp4()
{
	POD4 pod(7,8);
	Vect4 b;
	for (int i = 0; i < 1000000; i++ ) {
		pod.i = 1;
		b.append(pod);
	}
}

void testTVectSimp5()
{
	POD5 pod(9,10);
	Vect5 b;
	for (int i = 0; i < 1000000; i++ ) {
		pod.i = 1;
		b.append(pod);
	}
}

void testTVectSimp6()
{
	POD6 pod(11,12);
	Vect6 b;
	for (int i = 0; i < 1000000; i++ ) {
		pod.i = 1;
		b.append(pod);
	}
}

int main()
{
	testTVectSimp1();
	testTVectSimp2();
	testTVectSimp3();
	testTVectSimp4();
	testTVectSimp5();
	testTVectSimp6();
	return 0;
}
