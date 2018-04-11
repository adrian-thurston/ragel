/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class scanner
{
const(char) * ts;
const(char) * te;
int act;
int token;

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
	  'a' => {printf( "%.*s", "pat1\n" );};

	  [ab]+ . 'c' => {printf( "%.*s", "pat2\n" );};

	  any;
	*|;
}%%



%% write data;
int cs;
int blen;
char buffer[1024];

void init()
{
	%% write init;
}
void exec( const(char) data[] )
{
	const(char) *p = data.ptr;
	const(char) *pe = data.ptr + data.length;
	const(char) *eof = pe;
	char _s[];

	%% write exec;
}

void finish( )
{
	if ( cs >= scanner_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"ba a",
];

int inplen = 1;

}
int main()
{
	scanner m = new scanner();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

