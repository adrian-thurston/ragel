#include <iostream>
#include "astring.h"

using namespace std;

int main()
{
	String s = "hello";
	s += " there my friend";
	cout << s.data << endl;
}
