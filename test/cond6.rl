/* 
 * @LANG: c++
 */

/* Balanced parenthesis with conditions. */

#include <iostream>
#include <string.h>
using std::cout;
using std::endl;

%%{
	machine cond;
	write data noerror;
}%%

void test( const char *str )
{
	int cs = cond_start, n = 0;
	const char *p = str;
	const char *pe = str + strlen( str );

	%%{
		comment = '(' @{n=0;} 
			( '('@{n++;} | ')'@{n--;} | [^()] )*
		:> ')' when{!n};

		main := ' '* comment ' '* '\n' @{cout << "success";};

		write exec;
	}%%
	if ( cs < cond_first_final )
		cout << "failure";
	cout << endl;
}

int main()
{
	test( "( ( )\n" );
	test( "()()\n" );
	test( "(((\n" );
	test( "((()\n" );
	test( "((())\n" );
	test( "()\n" );
	test( "((()))\n" );
	test( "(()())\n" );
	test( "((())()(((()))))\n" );
	return 0;
}

#ifdef _____OUTPUT_____
failure
failure
failure
failure
failure
success
success
success
success
#endif
