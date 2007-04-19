#include <iostream>
#include "shrstr.h"

using namespace std;

int main()
{
	String s = "hello";
	s += " there my friend";
	cout << s.data << endl;
}
