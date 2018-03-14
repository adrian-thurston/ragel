/*
 * Copyright 2001 Adrian Thurston <thurston@colm.net>
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

#include "bsttable.h"
#include "bstmap.h"
#include "bstset.h"

using namespace std;

struct MyElement  : public CmpOrd<int>
{
	MyElement() {}
	MyElement(int key) : key(key) { }
	const int &getKey() const { return key; }
	int key;
};

template class BstTable<MyElement, int>;

void testBstTable1()
{
	BstMap<int, int, CmpOrd<int> > table;

	table.insert(1, 3);
	table.insert(3, 1);
	table.insert(5, 0);
	table.insert(7, 1);
	table.insert(4, 8);
	table.insert(2, 0);
	table.insert(6, 1);

	table.remove(2);
	table.remove(3);
	table.remove(6);
	table.remove(4);
	table.remove(5);
	table.remove(7);

	BstMapEl<int, int> *tab = table.data;
	int len = table.tabLen;
	for (int i = 0; i < len; i++, tab++)
		printf("(%i) maps to %i\n", tab->key, tab->value);
}

struct CompoundKey
{
	int Key1;
	int Key2;
};

struct CompoundKeyCompare
{
	inline static int compare(const CompoundKey &key1, const CompoundKey &key2)
	{
		if ( key1.Key1 < key2.Key1 )
			return -1;
		else if ( key1.Key1 > key2.Key1 )
			return 1;
		else
		{
			if ( key1.Key2 < key2.Key2 )
				return -1;
			else if ( key1.Key2 > key2.Key2 )
				return 1;
			else
				return 0;
		}
	}
};

void testBstTable2()
{
	BstMap<CompoundKey, int, CompoundKeyCompare > table;

	CompoundKey k;

	k.Key1 = 1; k.Key2 = 2;
	table.insert(k, 10);

	k.Key1 = 1; k.Key2 = 3;
	table.insert(k, 10);

	k.Key1 = 2; k.Key2 = 2;
	table.insert(k, 10);

	k.Key1 = 0; k.Key2 = 2;
	table.insert(k, 10);

	k.Key1 = 2; k.Key2 = -10;
	table.insert(k, 10);

	BstMapEl<CompoundKey, int> *tab = table.data;
	int len = table.tabLen;
	for (int i = 0; i < len; i++, tab++)
		printf("(%i, %i) maps to %i\n", tab->key.Key1, tab->key.Key2, tab->value);
}

struct KeyStruct
{
	/* Constructors. */
	KeyStruct() : key(0) { }
	KeyStruct(int i) : key(i)
		{ cout << "KeyStruct(" << key << ")" << endl; }
	KeyStruct(const KeyStruct &o) : key(o.key)
		{ cout << "KeyStruct(KeyStruct &)" << endl; }

	/* Destructors. */
	~KeyStruct() { cout << "~KeyStruct = {" << key << "}" << endl; }

	int key;
};

struct KeyStructCompare
{
	static inline int compare(const KeyStruct &k1, const KeyStruct &k2)
		{ return CmpOrd<int>::compare(k1.key, k2.key); }
};

int testBstTable3()
{
	BstMap<KeyStruct, int, KeyStructCompare > tbl;

	cout << "ins res = " << tbl.insert(KeyStruct(0), 1) << endl;
	cout << "ins res = " << tbl.insert(KeyStruct(1), 1) << endl;
	return 0;
}

int testBstTable4()
{
	BstMap<int, int, CmpOrd<int> > tbl;

	tbl.insertMulti(1, 1);
	tbl.insertMulti(4, 1);
	tbl.insertMulti(2, 1);
	tbl.insertMulti(1, 1);
	tbl.insertMulti(1, 1);
	tbl.insertMulti(1, 3);
	tbl.insertMulti(3, 1);
	tbl.insertMulti(1, 0);
	tbl.insertMulti(7, 1);
	tbl.insertMulti(4, 8);
	tbl.insertMulti(1, 0);
	tbl.insertMulti(6, 1);

	BstMapEl<int, int> *low, *high;

	tbl.findMulti( 1, low, high );
	cout << high - low + 1 << endl;

	cout << tbl.removeMulti( 1 ) << endl;
	cout << tbl.findMulti( 1, low, high ) << endl;
	return 0;
}

int testBstTable5()
{
	cout << "TEST 5" << endl;

	BstTable<MyElement, int> table1, table2;
	table1.insert( 1 );
	table1.insert( 2 );
	table1.insert( 3 );
	table1.insertMulti( 2 );

	table2.insert( 3 );
	table2.insert( table1 );

	for ( int i = 0; i < table2.tabLen; i++ )
		cout << table2.data[i].key << endl;

	table2.insertMulti( table1 );
	for ( int i = 0; i < table2.tabLen; i++ )
		cout << table2.data[i].key << endl;
	return 0;
}

void testBstTable6()
{
	cout << "TEST 6" << endl;
	BstTable<MyElement, int> table1( MyElement(1) );
	cout << table1[0].key << endl;
}

int main()
{
	testBstTable1();
	testBstTable2();
	testBstTable3();
	testBstTable4();
	testBstTable5();
	testBstTable6();
	return 0;
}
