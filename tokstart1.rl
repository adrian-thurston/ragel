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
	char *tokstart, *tokend;

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
		cout << " tokstart = " << ( tokstart == 0 ? -1 : tokstart-buf ) << endl;
	} 
	action from_act {
		cout << "from: fc = ";
		if ( fc == '\'' )
			cout << (int)fc;
		else
			cout << fc;
		cout << " tokstart = " << ( tokstart == 0 ? -1 : tokstart-buf ) << endl;
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

	%% write exec;

	int have = 0;
	if ( tokstart != 0 ) {
		have = pe - tokstart;
		memmove( data, tokstart, have );
	}
	return have;
}

int Scanner::finish( )
{
	%% write eof;
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
from: fc = a tokstart = 0
to:   fc = a tokstart = 0
from: fc =   tokstart = 0
to:   fc = a tokstart = -1
from: fc =   tokstart = 1
to:   fc =   tokstart = 1
from: fc = b tokstart = 1
to:   fc =   tokstart = -1
from: fc = b tokstart = 2
to:   fc = b tokstart = 2
from: fc =   tokstart = 2
to:   fc = b tokstart = -1
from: fc =   tokstart = 3
to:   fc =   tokstart = 3
from: fc = 0 tokstart = 3
to:   fc =   tokstart = -1
from: fc = 0 tokstart = 4
to:   fc = 0 tokstart = 4
from: fc = . tokstart = 4
to:   fc = . tokstart = 4
from: fc = 9 tokstart = 4
to:   fc = 9 tokstart = 4
from: fc = 8 tokstart = 4
to:   fc = 8 tokstart = 4
from: fc =   tokstart = 4
to:   fc = 8 tokstart = -1
from: fc =   tokstart = 8
to:   fc =   tokstart = 8
from: fc = / tokstart = 8
to:   fc =   tokstart = -1
from: fc = / tokstart = 9
to:   fc = / tokstart = 9
from: fc = * tokstart = 9
to:   fc = * tokstart = -1
from: fc = 
 tokstart = -1
to:   fc = 
 tokstart = -1
from: fc = 9 tokstart = -1
to:   fc = 9 tokstart = -1
from: fc =   tokstart = -1
to:   fc =   tokstart = -1
from: fc = * tokstart = -1
to:   fc = * tokstart = -1
from: fc = / tokstart = -1
to:   fc = / tokstart = -1
from: fc = 39 tokstart = 16
to:   fc = 39 tokstart = 16
from: fc = \ tokstart = 16
to:   fc = \ tokstart = 16
from: fc = 39 tokstart = 16
to:   fc = 39 tokstart = 16
from: fc = 39 tokstart = 16
to:   fc = 39 tokstart = -1
from: fc = / tokstart = 20
to:   fc = / tokstart = 20
from: fc = / tokstart = 20
to:   fc = / tokstart = -1
from: fc = h tokstart = -1
to:   fc = h tokstart = -1
from: fc = i tokstart = -1
to:   fc = i tokstart = -1
from: fc = 
 tokstart = -1
to:   fc = 
 tokstart = -1
from: fc = t tokstart = 25
to:   fc = t tokstart = 25
from: fc = h tokstart = 25
to:   fc = h tokstart = 25
from: fc = e tokstart = 25
to:   fc = e tokstart = 25
from: fc = r tokstart = 25
to:   fc = r tokstart = 25
from: fc = e tokstart = 25
to:   fc = e tokstart = 25
from: fc = 
 tokstart = 25
to:   fc = e tokstart = -1
from: fc = 
 tokstart = 30
to:   fc = 
 tokstart = 30
#endif
