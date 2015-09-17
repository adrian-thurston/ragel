/* 
 * @LANG: c++
 *
 * This is a conds test modified to exercise actions arguments.
 */

#include <iostream>
#include <string.h>
using std::cout;
using std::endl;

%%{
	machine foo;

	action c(v) { $v }

	action c1 {i}
	action c2 {j}

	action one {"one"}
	action pr(str) { cout << "  " $str << endl;}

	main := (
			[a-z] |
			('\n' when c(c1) @pr( one ) )
		)*
		('\n' when c({j}) @pr( {"two"} ) );
}%%

%% write data noerror;

void test( int i, int j, const char *str )
{
	int cs = foo_start;
	const char *p = str;
	const char *pe = str + strlen( str );

	cout << "run:" << endl;
	%% write exec;
	if ( cs >= foo_first_final )
		cout << "  success" << endl;
	else
		cout << "  failure" << endl;
	cout << endl;
}

int main()
{
	test( 0, 0, "hi\n\n" );
	test( 1, 0, "hi\n\n" );
	test( 0, 1, "hi\n" );
	test( 0, 1, "hi\n\n" );
	test( 1, 1, "hi\n" );
	test( 1, 1, "hi\n\n" );
	test( 1, 1, "hi\n\nx" );
	return 0;
}

##### OUTPUT #####
run:
  failure

run:
  one
  one
  failure

run:
  two
  success

run:
  two
  failure

run:
  one
  two
  success

run:
  one
  two
  one
  two
  success

run:
  one
  two
  one
  two
  failure

