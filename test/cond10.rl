/*
 * @LANG: c++
 *
 * This test case exercises repetition of a machine that accepts zero-length
 * string. It is very ambiguous and not useful as a pattern. 
 */

#include <iostream>
#include <string.h>

using std::cout;
using std::endl;

%%{
	machine foo;
	alphtype char;

	action bs_b { bs_b( src, p ) }
	action bol { p == src }
	action eol { p+1 == eof }

	b = '' %when bs_b;
	B = '' %when !bs_b;
	bol = '' %when bol;
	eol = '' %when eol;

	# {1,25}
	action ini_4 { q_4 = 0; }
	action inc_4 { q_4++; }
	action min_4 { q_4 >= 1 }
	action max_4 { q_4 < 25 }

	# {1,5}
	action ini_5 { q_5 = 0; }
	action inc_5 { q_5++; }
	action min_5 { q_5 >= 1 }
	action max_5 { q_5 < 5 }

	# {100}
	action ini_6 { q_6 = 0; }
	action inc_6 { q_6++; }
	action min_6 { q_6 >= 100 }
	action max_6 { q_6 < 100 }

	R5306833741170350 =
		( '<'  47  's' 't' 'y' 'l' 'e' '>' '<' 
			(:condstar( (  ( (:condstar( (  [a-zA-Z0-9_]  ), ini_4, inc_4, min_4, max_4 ): ) 
			(:condstar( ( ' ' ), ini_5, inc_5, min_5, max_5 ): ) )  ), ini_6, inc_6, min_6, max_6 ): ) )  
	:> any @{ match = 1; };

	main := R5306833741170350;
}%%

%% write data;

void test( const char *str )
{
	int cs = foo_start;
	const char *p = str;
	const char *pe = str + strlen( str );
	int match = 0;

	long q_4 = 0, q_5 = 0, q_6 = 0;

	cout << "run:" << endl;
	%% write exec;
	if ( match )
		cout << "  success" << endl;
	else
		cout << "  failure" << endl;
	cout << endl;
}

int main()
{
	return 0;
}

##### OUTPUT #####
