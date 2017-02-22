/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class GotoCallRet
{
char comm;
int top;
int stack[32];
const(char) * ts;
const(char) * te;
int act;
int val;

%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{printf( "%.*s", "handle " );fhold;if ( val == 1 )
{
	fnext *fentry(one);
} 
if ( val == 2 )
{
	fnext *fentry(two);
} 
if ( val == 3 )
{
	fnext main;
} 
};

	one := |*
		'{' => {printf( "%.*s", "{ " );fcall *fentry(one);};
		"[" => {printf( "%.*s", "[ " );fcall *fentry(two);};
		"}" sp* => {printf( "%.*s", "} " );fret;};
		[a-z]+ => {printf( "%.*s", "word " );val = 1;
fgoto *fentry(handle);};
		' ' => {printf( "%.*s", "space " );};
	*|;

	two := |*
		'{' => {printf( "%.*s", "{ " );fcall *fentry(one);};
		"[" => {printf( "%.*s", "[ " );fcall *fentry(two);};
		']' sp* => {printf( "%.*s", "] " );fret;};
		[a-z]+ => {printf( "%.*s", "word " );val = 2;
fgoto *fentry(handle);};
		' ' => {printf( "%.*s", "space " );};
	*|;

	main := |* 
		'{' => {printf( "%.*s", "{ " );fcall one;};
		"[" => {printf( "%.*s", "[ " );fcall two;};
		[a-z]+ => {printf( "%.*s", "word " );val = 3;
fgoto handle;};
		[a-z] ' foil' => {printf( "%.*s", "this is the foil" );};
		' ' => {printf( "%.*s", "space " );};
		'\n';
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
	if ( cs >= GotoCallRet_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"{a{b[c d]d}c}\n",
"[a{b[c d]d}c}\n",
"[a[b]c]d{ef{g{h}i}j}l\n",
"{{[]}}\n",
"a b c\n",
"{a b c}\n",
"[a b c]\n",
"{]\n",
"{{}\n",
"[[[[[[]]]]]]\n",
"[[[[[[]]}]]]\n",
];

int inplen = 11;

}
int main()
{
	GotoCallRet m = new GotoCallRet();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

