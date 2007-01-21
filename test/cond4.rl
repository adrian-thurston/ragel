/* 
 * @LANG: c++
 */

#include <iostream>
#include <string.h>
using std::cout;
using std::endl;

%%{
	machine foo;

	action c1 {(cout << "c1 ", true)}
	action c2 {(cout << "c2 ", true)}
	action c3 {(cout << "c3 ", true)}
	action c4 {(cout << "c4 ", true)}

	main := (
		10 .. 60 when c1 |
		20 .. 40 when c2 |
		30 .. 50 when c3 |
		32 .. 38 when c4 |
		0 .. 70 )* ${cout << "char: " << (int)*p << endl;};
}%%

%% write data noerror nofinal;

void test( char *str )
{
	int len = strlen( str );
	int cs = foo_start;
	char *p = str, *pe = str+len;
	%% write exec;
}

char data[] = { 5, 15, 25, 31, 35, 39, 45, 55, 65, 0 };

int main()
{
	test( data );
	return 0;
}

#ifdef _____OUTPUT_____
char: 5
c1 char: 15
c1 c2 char: 25
c1 c2 c3 char: 31
c1 c2 c3 c4 char: 35
c1 c2 c3 char: 39
c1 c3 char: 45
c1 char: 55
char: 65
#endif
