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
		'a' => {printf( "%.*s", "on last     " );if ( p + 1 == te )
{
	printf( "%.*s", "yes" );
} 
printf( "%.*s", "\n" );};

		'b'+ => {printf( "%.*s", "on next     " );if ( p + 1 == te )
{
	printf( "%.*s", "yes" );
} 
printf( "%.*s", "\n" );};

		'c1' 'dxxx'? => {printf( "%.*s", "on lag      " );if ( p + 1 == te )
{
	printf( "%.*s", "yes" );
} 
printf( "%.*s", "\n" );};

		'd1' => {printf( "%.*s", "lm switch1  " );if ( p + 1 == te )
{
	printf( "%.*s", "yes" );
} 
printf( "%.*s", "\n" );};
		'd2' => {printf( "%.*s", "lm switch2  " );if ( p + 1 == te )
{
	printf( "%.*s", "yes" );
} 
printf( "%.*s", "\n" );};

		[d0-9]+ '.';

		'\n';
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
"abbc1d1d2\n",
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

