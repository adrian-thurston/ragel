/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class next1
{
int target;

%%{
	machine next1;

	unused := 'unused';

	one := 'one' @{printf( "%.*s", "one\n" );target = fentry(main);
fnext *target;};

	two := 'two' @{printf( "%.*s", "two\n" );target = fentry(main);
fnext *target;};

	main := 
		'1' @{target = fentry(one);
fnext *target;}
	|	'2' @{target = fentry(two);
fnext *target;}
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
	if ( cs >= next1_first_final )
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
	next1 m = new next1();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

