/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class patact
{
char comm;
int top;
int stack[32];
const(char) * ts;
const(char) * te;
int act;
int value;

%%{
	machine patact;

	other := |* 
		[a-z]+ => {printf( "%.*s", "word\n" );};
		[0-9]+ => {printf( "%.*s", "num\n" );};
		[\n ] => {printf( "%.*s", "space\n" );};
	*|;

	exec_test := |* 
		[a-z]+ => {printf( "%.*s", "word (w/lbh)\n" );fexec te-1;fgoto other;};
		[a-z]+ ' foil' => {printf( "%.*s", "word (c/lbh)\n" );};
		[\n ] => {printf( "%.*s", "space\n" );};
		'22' => {printf( "%.*s", "num (w/switch)\n" );};
		[0-9]+ => {printf( "%.*s", "num (w/switch)\n" );fexec te-1;fgoto other;};
		[0-9]+ ' foil' => {printf( "%.*s", "num (c/switch)\n" );};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {printf( "%.*s", "in semi\n" );fgoto main;};
	*|;

	main := |* 
		[a-z]+ => {printf( "%.*s", "word (w/lbh)\n" );fhold;fgoto other;};
		[a-z]+ ' foil' => {printf( "%.*s", "word (c/lbh)\n" );};
		[\n ] => {printf( "%.*s", "space\n" );};
		'22' => {printf( "%.*s", "num (w/switch)\n" );};
		[0-9]+ => {printf( "%.*s", "num (w/switch)\n" );fhold;fgoto other;};
		[0-9]+ ' foil' => {printf( "%.*s", "num (c/switch)\n" );};
		';' => {printf( "%.*s", "going to semi\n" );fhold;fgoto semi;};
		'!' => {printf( "%.*s", "immdiate\n" );fgoto exec_test;};
	*|;
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
	const(char) *eof = pe;
	char _s[];

	%% write exec;
}

void finish( )
{
	if ( cs >= patact_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"abcd foix\n",
"abcd\nanother\n",
"123 foix\n",
"!abcd foix\n",
"!abcd\nanother\n",
"!123 foix\n",
";",
];

int inplen = 7;

}
int main()
{
	patact m = new patact();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

