/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class foo
{
int i;
int j;
int k;

%%{
	machine foo;

	action c1 {i != 0}
	action c2 {j != 0}
	action c3 {k != 0}
	action one {printf( "%.*s", "  one\n" );}
	action two {printf( "%.*s", "  two\n" );}
	action three {printf( "%.*s", "  three\n" );}

	action seti {if ( fc == 48 )
{
	i = 0;

} 
else {
	i = 1;

}
}
	action setj {if ( fc == 48 )
{
	j = 0;

} 
else {
	j = 1;

}
}
	action setk {if ( fc == 48 )
{
	k = 0;

} 
else {
	k = 1;

}
}

	action break {fnbreak;}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
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
"000abc\n",
"100abc\n",
"010abc\n",
"110abc\n",
"001abc\n",
"101abc\n",
"011abc\n",
"111abc\n",
];

int inplen = 8;

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

