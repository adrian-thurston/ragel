/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class goto1
{
int target;

%%{
	machine goto1;

	unused := 'unused';

	one := 'one' @{printf( "%.*s", "one\n" );target = fentry(main);
fgoto *target;};

	two := 'two' @{printf( "%.*s", "two\n" );target = fentry(main);
fgoto *target;};

	main := 
		'1' @{target = fentry(one);
fgoto *target;}
	|	'2' @{target = fentry(two);
fgoto *target;}
	|	'\n';
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
	if ( cs >= goto1_first_final )
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
	goto1 m = new goto1();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

