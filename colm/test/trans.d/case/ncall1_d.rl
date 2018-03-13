/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class ncall1
{
int top;
int stack[32];
int target;

%%{
	machine ncall1;

	unused := 'unused';

	one := 'one' @{printf( "%.*s", "one\n" );fnret;};

	two := 'two' @{printf( "%.*s", "two\n" );fnret;};

	main := (
			'1' @{target = fentry(one);
fncall *target;}
		|	'2' @{target = fentry(two);
fncall *target;}
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
	if ( cs >= ncall1_first_final )
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
	ncall1 m = new ncall1();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

