/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class curs1
{
int return_to;

%%{
	machine curs1;

	unused := 'unused';

	one := 'one' @{printf( "%.*s", "one\n" );fnext *return_to;};

	two := 'two' @{printf( "%.*s", "two\n" );fnext *return_to;};

	main := 
		'1' @{return_to = fcurs;
fnext one;}
	|	'2' @{return_to = fcurs;
fnext two;}
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
	if ( cs >= curs1_first_final )
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
	curs1 m = new curs1();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

