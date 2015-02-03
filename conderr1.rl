/* 
 * @LANG: c++
 */

#include <iostream>
#include <string.h>
using std::cout;
using std::endl;

%%{
	machine foo;

	action c {c}
	main := 
		( ( 'a' when c )+ ) $err{ cout << "  bail" << endl; }
		'\n';
}%%

%% write data noerror;

void test( int c, const char *str )
{
	int cs = foo_start;
	const char *p = str;
	const char *pe = str + strlen( str );
	const char *eof = pe;

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
	test( 1, "aa\n" );
	test( 1, "ab\n" );
	test( 0, "aa\n" );
	test( 0, "ab\n" );
	return 0;
}

##### OUTPUT #####
run:
  success

run:
  bail
  failure

run:
  bail
  failure

run:
  bail
  failure

