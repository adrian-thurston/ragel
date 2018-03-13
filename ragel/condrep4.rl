/* 
 * @LANG: c++
 */

#include <iostream>
#include <string.h>
using std::cout;
using std::endl;

%%{
	machine foo;


	action init { c = 0; }
	action inc { c += 1; }
	action min { ({ c >= min; }) }
	action max { ({ c < max; }) }

	main :=
		:condstar( any, init, inc, min, max ): :> '.' 0;

}%%

%% write data noerror;

void test( int min, int max, const char *str )
{
	int cs;
	const char *p = str;
	const char *pe = str + strlen( str ) + 1;
	int c = 0, seen = 0;

	cout << "run " << str << " " << min << " " << max << ":";

	%% write init;
	%% write exec;

	cout << " " << seen;

	if ( cs >= foo_first_final )
		cout << " success" << endl;
	else
		cout << " failure" << endl;
}

int main()
{
	test( 0, 0, "" );
	test( 0, 1, "" );
	test( 1, 1, "" );
	test( 1, 2, "" );
	test( 2, 2, "" );
	test( 2, 3, "" );

	test( 0, 0, "." );
	test( 0, 1, "." );
	test( 1, 1, "." );
	test( 1, 2, "." );
	test( 2, 2, "." );
	test( 2, 3, "." );

	test( 0, 0, ".." );
	test( 0, 1, ".." );
	test( 1, 1, ".." );
	test( 1, 2, ".." );
	test( 2, 2, ".." );
	test( 2, 3, ".." );

	test( 0, 0, "..." );
	test( 0, 1, "..." );
	test( 1, 1, "..." );
	test( 1, 2, "..." );
	test( 2, 2, "..." );
	test( 2, 3, "..." );

	test( 0, 0, "...." );
	test( 0, 1, "...." );
	test( 1, 1, "...." );
	test( 1, 2, "...." );
	test( 2, 2, "...." );
	test( 2, 3, "...." );
	return 0;
}

##### OUTPUT #####
run  0 0: 0 failure
run  0 1: 0 failure
run  1 1: 0 failure
run  1 2: 0 failure
run  2 2: 0 failure
run  2 3: 0 failure
run . 0 0: 0 success
run . 0 1: 0 success
run . 1 1: 0 failure
run . 1 2: 0 failure
run . 2 2: 0 failure
run . 2 3: 0 failure
run .. 0 0: 0 failure
run .. 0 1: 0 failure
run .. 1 1: 0 success
run .. 1 2: 0 success
run .. 2 2: 0 failure
run .. 2 3: 0 failure
run ... 0 0: 0 failure
run ... 0 1: 0 failure
run ... 1 1: 0 failure
run ... 1 2: 0 failure
run ... 2 2: 0 success
run ... 2 3: 0 success
run .... 0 0: 0 failure
run .... 0 1: 0 failure
run .... 1 1: 0 failure
run .... 1 2: 0 failure
run .... 2 2: 0 failure
run .... 2 3: 0 failure
