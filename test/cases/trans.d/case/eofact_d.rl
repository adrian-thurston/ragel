/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class eofact
{



%%{
	machine eofact;

	action a1 {printf( "%.*s", "a1\n" );}
	action a2 {printf( "%.*s", "a2\n" );}
	action a3 {printf( "%.*s", "a3\n" );}
	action a4 {printf( "%.*s", "a4\n" );}


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

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
	if ( cs >= eofact_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"",
"h",
"hell",
"hello",
"hello\n",
"t",
"ther",
"there",
"friend",
];

int inplen = 9;

}
int main()
{
	eofact m = new eofact();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

