/*
 *  Copyright 2001-2003 Adrian Thurston <adriant@ragel.ca>
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

#include "deque.h"
#include "dequesimp.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

using namespace std;

/* test with small chunks. */
struct MyBlockSize
{
	int getBlockSize( ) { return 2; }
};

/* Deque element */
struct DequeEl1
{
	DequeEl1() {
		id = nextId++;
		cout << "DequeEl1(" << id << ")" << endl;
	}
	DequeEl1( const DequeEl1 &other ) : id(other.id) {
		std::cout << "CopyDequeEl1(" << id << ")" << endl;
	}
	~DequeEl1() {
		std::cout << "~DequeEl1(" << id << ")" << endl;
	}

	static int nextId;
	int id;
};

int DequeEl1::nextId = 1;

struct DequeEl2
{
	int id;
};

template class Deque<DequeEl1>;
template class DequeSimp<DequeEl2>;

#define NUM 5

void testDeque()
{
	Deque<DequeEl1> deque;

	/* Init some elements. */
	DequeEl1 els[NUM];
	deque.append( els, NUM );
	deque.append( els, NUM );

	for ( Deque<DequeEl1>::Iter el = deque.first(); el.lte(); el++ )
		cout << el->id << endl;

	for ( Deque<DequeEl1>::Iter el = deque.last(); el.gtb(); el-- )
		cout << el->id << endl;
}

int main()
{
	testDeque();
	return 0;
}
