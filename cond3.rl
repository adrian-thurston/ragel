/* 
 * @LANG: c++
 */

#include <iostream>
#include <string.h>
using std::cout;
using std::endl;

%%{
	machine foo;

	action hit_5 {c == 5}
	action done { cout << "  done" << endl; }
	action inc {c++;}

	# The any* includes '\n' when hit_5 is true, so use guarded concatenation.
	main := (any @inc)* :> '\n' when hit_5 @done;
}%%

%% write data noerror;

void test( const char *str )
{
	int cs = foo_start;
	int c = 0;
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
	test( "12345\n" );  // success
	test( "\n2345\n" ); // success, first newline ignored
	test( "1234\n" );   // failure, didn't get 5 chars before newline.
	return 0;
}

#ifdef _____OUTPUT_____
run:
  done
  success

run:
  done
  success

run:
  failure

#endif
