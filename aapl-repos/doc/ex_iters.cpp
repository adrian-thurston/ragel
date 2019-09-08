#include <iostream>
#include "vector.h"

struct Value
{
	Value( const int data ) 
		: data(data) {}
	int data;
	operator int() { return data; }
};

typedef Vector<Value> MyVect;

int main()
{
    MyVect vector;
    vector.append( Value(1) );
    vector.append( Value(2) );
    vector.append( Value(3) );

	/* Move forward through the vector, get data directly. */
	for ( MyVect::Iter i = vector; i.lte(); i++ )
		std::cout << i->data << std::endl;

	/* Move backwards through the vector, use conversion from (Value&) to int. */
	for ( MyVect::Iter i = vector.last(); i.gtb(); i-- )
		std::cout << *i << std::endl;
	
	/* Find the value before value 3, iter is cast to (Value*). */
	Value *last = vector.data + 2;
	MyVect::Iter i = vector;
	while ( i != last )
		i.increment();
	std::cout << *(--i) << std::endl;
	
    return 0;
}
