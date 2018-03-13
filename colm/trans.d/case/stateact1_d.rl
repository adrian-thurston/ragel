/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class state_act
{



%%{
	machine state_act;

	action a1 {printf( "%.*s", "a1\n" );}
	action a2 {printf( "%.*s", "a2\n" );}
	action b1 {printf( "%.*s", "b1\n" );}
	action b2 {printf( "%.*s", "b2\n" );}
	action c1 {printf( "%.*s", "c1\n" );}
	action c2 {printf( "%.*s", "c2\n" );}
	action next_again {fnext again;}

	hi = 'hi';
	line = again: 
			hi 
				>to b1 
				>from b2 
			'\n' 
				>to c1 
				>from c2 
				@next_again;
		 
	main := line*
			>to a1 
			>from a2;
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
	if ( cs >= state_act_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"hi\nhi\n",
];

int inplen = 1;

}
int main()
{
	state_act m = new state_act();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

