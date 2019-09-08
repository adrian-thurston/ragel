#include <iostream>
#include "vector.h"

using namespace std;

class Element
{
public:
	Element(const Element &o);
	Element(int data);
	~Element();
	int data;
};

Element::Element(const Element &o) 
: 
	data(o.data)
{ 
	cout << "Copy Constructor" << endl;
}

Element::Element(int data) : data(data) 
{
	cout << "Initialization Constructor" << endl;
}

Element::~Element()
{
	cout << "Destructor" << endl;
}

int main()
{
    Vector<Element> v;
    v.append( Element(2) );

    /* Insert 5 duplicate elements at pos 0. */
    v.prependDup( Element(1), 5 );      

    return 0;
}
