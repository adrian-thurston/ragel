/*
 * @LANG: c++
 */

#include <iostream>
#include <string.h>
using namespace std;

extern char buf[];

struct Scanner
{
	int cs, act;
	char *ts, *te;

	// Initialize the machine. Invokes any init statement blocks. Returns 0
	// if the machine begins in a non-accepting state and 1 if the machine
	// begins in an accepting state.
	void init( );

	// Execute the machine on a block of data. Returns -1 if after processing
	// the data, the machine is in the error state and can never accept, 0 if
	// the machine is in a non-accepting state and 1 if the machine is in an
	// accepting state.
	int execute( char *data, int len );

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( );
};

%%{
	machine Scanner;

	action to_act { 
		cout << "to:   fc = ";
		if ( fc == '\'' )
			cout << (int)fc;
		else
			cout << fc;
		cout << " ts = " << ( ts == 0 ? -1 : ts-buf ) << endl;
	} 
	action from_act {
		cout << "from: fc = ";
		if ( fc == '\'' )
			cout << (int)fc;
		else
			cout << fc;
		cout << " ts = " << ( ts == 0 ? -1 : ts-buf ) << endl;
	}

	c_comm := ( any* $0 '*/' @1 @{ fgoto main; } ) $~to_act $*from_act;
	cxx_comm := ( any* $0 '\n' @1 @{ fgoto main; } ) $~to_act $*from_act;

	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | /\\./ )* "'" ) $~ to_act $* from_act;
	( 'L'? '"' ( [^"\\\n] | /\\./ )* '"' ) $~ to_act $* from_act;

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) $~ to_act $* from_act;

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) $~ to_act $* from_act;
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]{0,3} ) $~ to_act $* from_act;

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]{0,2} ) $~ to_act $* from_act;

	# Integer hex. Leading 0 buffered by float.
	( '0x' [0-9a-fA-F]+ [ulUL]{0,2} ) $~ to_act $* from_act;

	# Three char compounds, first item already buffered. */
	( '...' ) $~ to_act $* from_act;

	# Single char symbols.
	( punct - [_"'] ) $~ to_act $* from_act;

	# Comments and whitespace.
	( '/*' ) $~ to_act $* from_act { fgoto c_comm; };
	( '//' ) $~ to_act $* from_act { fgoto cxx_comm; };

	( any - 33..126 )+ $~ to_act $* from_act;

	*|;
}%%

%% write data;

void Scanner::init( )
{
	%% write init;
}

int Scanner::execute( char *data, int len )
{
	char *p = data;
	char *pe = data + len;
	char *eof = pe;

	%% write exec;

	return 0;
}

int Scanner::finish( )
{
	if ( cs == Scanner_error )
		return -1;
	if ( cs >= Scanner_first_final )
		return 1;
	return 0;
}

void test( )
{
	int len = strlen( buf );
	Scanner scanner;

	scanner.init();
	scanner.execute( buf, len );
	if ( scanner.cs == Scanner_error ) {
		/* Machine failed before finding a token. */
		cout << "PARSE ERROR" << endl;
	}
	scanner.finish();
}

char buf[4096];

int main()
{
	strcpy( buf, 
		"a b 0.98 /*\n"
		"9 */'\\''//hi\n"
		"there\n"
	);
	test();
	return 0;
}

#ifdef _____OUTPUT_____
from: fc = a ts = 0
to:   fc = a ts = 0
from: fc =   ts = 0
to:   fc = a ts = -1
from: fc =   ts = 1
to:   fc =   ts = 1
from: fc = b ts = 1
to:   fc =   ts = -1
from: fc = b ts = 2
to:   fc = b ts = 2
from: fc =   ts = 2
to:   fc = b ts = -1
from: fc =   ts = 3
to:   fc =   ts = 3
from: fc = 0 ts = 3
to:   fc =   ts = -1
from: fc = 0 ts = 4
to:   fc = 0 ts = 4
from: fc = . ts = 4
to:   fc = . ts = 4
from: fc = 9 ts = 4
to:   fc = 9 ts = 4
from: fc = 8 ts = 4
to:   fc = 8 ts = 4
from: fc =   ts = 4
to:   fc = 8 ts = -1
from: fc =   ts = 8
to:   fc =   ts = 8
from: fc = / ts = 8
to:   fc =   ts = -1
from: fc = / ts = 9
to:   fc = / ts = 9
from: fc = * ts = 9
to:   fc = * ts = -1
from: fc = 
 ts = -1
to:   fc = 
 ts = -1
from: fc = 9 ts = -1
to:   fc = 9 ts = -1
from: fc =   ts = -1
to:   fc =   ts = -1
from: fc = * ts = -1
to:   fc = * ts = -1
from: fc = / ts = -1
to:   fc = / ts = -1
from: fc = 39 ts = 16
to:   fc = 39 ts = 16
from: fc = \ ts = 16
to:   fc = \ ts = 16
from: fc = 39 ts = 16
to:   fc = 39 ts = 16
from: fc = 39 ts = 16
to:   fc = 39 ts = -1
from: fc = / ts = 20
to:   fc = / ts = 20
from: fc = / ts = 20
to:   fc = / ts = -1
from: fc = h ts = -1
to:   fc = h ts = -1
from: fc = i ts = -1
to:   fc = i ts = -1
from: fc = 
 ts = -1
to:   fc = 
 ts = -1
from: fc = t ts = 25
to:   fc = t ts = 25
from: fc = h ts = 25
to:   fc = h ts = 25
from: fc = e ts = 25
to:   fc = e ts = 25
from: fc = r ts = 25
to:   fc = r ts = 25
from: fc = e ts = 25
to:   fc = e ts = 25
from: fc = 
 ts = 25
to:   fc = e ts = -1
from: fc = 
 ts = 30
to:   fc = 
 ts = 30
to:   fc = 
 ts = -1
#endif
