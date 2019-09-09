/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class foo
{
int i;
int c;

%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = cast( int ) ( fc )
;
printf( "%.*s", "item: " );printf( "%d", c );
printf( "%.*s", "\n" );}

	count = [0-9] @{i = cast( int ) ( fc - 48 )
;
printf( "%.*s", "count: " );printf( "%d", i );
printf( "%.*s", "\n" );};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
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
	if ( cs >= foo_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"00\n",
"019\n",
"190\n",
"1719\n",
"1040000\n",
"104000a\n",
"104000\n",
];

int inplen = 7;

}
int main()
{
	foo m = new foo();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

