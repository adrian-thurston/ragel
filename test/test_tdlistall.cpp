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
