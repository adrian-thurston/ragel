/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class zlen1
{



%%{
	machine zlen1;
	main := zlen;
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
	char _s[];

	%% write exec;
}

void finish( )
{
	if ( cs >= zlen1_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"",
"x",
];

int inplen = 2;

}
int main()
{
	zlen1 m = new zlen1();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

