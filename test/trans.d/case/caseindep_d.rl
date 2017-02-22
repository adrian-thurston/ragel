/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class indep
{


%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
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
	if ( cs >= indep_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"1hello\n",
"1HELLO\n",
"1HeLLo\n",
"2hello\n",
"2HELLO\n",
"2HeLLo\n",
"3hello\n",
"3HELLO\n",
"3HeLLo\n",
"4hello\n",
"4HELLO\n",
"4HeLLo\n",
"5hello\n",
"5HELLO\n",
"5HeLLo\n",
"6hello\n",
"6HELLO\n",
"6HeLLo\n",
];

int inplen = 18;

}
int main()
{
	indep m = new indep();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

