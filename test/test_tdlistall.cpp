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
#include "tdlist.h"
#include "tdlistmel.h"
#include "tdlistval.h"
#include "dlist.h"
#include "dlistmel.h"
#include "dlistval.h"

using namespace std;

/* 
 * TDList
 */
struct ListEl1 : public TDListEl { int i; };
typedef TDList<ListEl1> List1;

/*
 * TDListMel
 */
struct ListEl2_A : public TDListEl { };
struct ListEl2_B : public TDListEl { };
struct ListEl2 : public ListEl2_A, public ListEl2_B { int i; };
typedef TDListMel<ListEl2, ListEl2_A> List2;

/*
 * TDListVal
 */
typedef TDListVal<int> List3;


/* Instantiate all lists. */
template class TDListMel<ListEl2, ListEl2_A>;
template class TDListVal<int>;
template class TDList<ListEl1>;

int main()
{
	return 0;
}
