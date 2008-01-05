/* 
 * @LANG: c++
 */

#include <iostream>
#include <string.h>
using std::cout;
using std::endl;

%%{
	machine foo;
	write data noerror;
}%%

void test( const char *str )
{
	int cs = foo_start;
	int c = 0;
	const char *p = str;
	const char *pe = str + strlen( str );
	char last = '0';

	cout << "run:";
	%%{
		action d1 { cout << " d1"; }
		action see_five { cout << " see_five"; }

		see_five = ([0-9] when{c++ < 5} @d1)* '\n' @see_five;

		action in_sequence { cout << " in_sequence"; }
		action d2 { last = *p; cout << " d2"; }
		in_sequence = ( [0-9] when { *p == last+1 } @d2 )* '\n' @in_sequence;

		main := ( see_five | in_sequence ) ${cout << " |";};

		write exec;
	}%%
	if ( cs < foo_first_final )
		cout << " failure";
	cout << endl;
}

int main()
{
	test( "123456789012\n" );  // fails both
	test( "123456789\n" );  // fails five
	test( "1234\n" );  // fails five
	test( "13245\n" );  // fails sequence
	test( "12345\n" );  // succeeds in both
	return 0;
}

#ifdef _____OUTPUT_____
run: d1 d2 | d1 d2 | d1 d2 | d1 d2 | d1 d2 | d2 | d2 | d2 | d2 | failure
run: d1 d2 | d1 d2 | d1 d2 | d1 d2 | d1 d2 | d2 | d2 | d2 | d2 | in_sequence |
run: d1 d2 | d1 d2 | d1 d2 | d1 d2 | see_five in_sequence |
run: d1 d2 | d1 | d1 | d1 | d1 | see_five |
run: d1 d2 | d1 d2 | d1 d2 | d1 d2 | d1 d2 | see_five in_sequence |
#endif
