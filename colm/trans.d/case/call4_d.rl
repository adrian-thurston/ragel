/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class call4
{
int target;
int top;
int stack[32];

%%{
	machine call4;

	unused := 'unused';

	one := 'one' @{printf( "%.*s", "one\n" );fret;};

	two := 'two' @{printf( "%.*s", "two\n" );fret;};

	main := (
			'1' @{target = fentry(one);
fcall *target;}
		|	'2' @{target = fentry(two);
fcall *target;}
		|	'\n'
	)*;
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
	if ( cs >= call4_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"1one2two1one\n",
];

int inplen = 1;

}
int main()
{
	call4 m = new call4();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

