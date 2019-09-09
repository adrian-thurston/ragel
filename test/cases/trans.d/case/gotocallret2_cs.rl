/*
 * @LANG: csharp
 * @GENERATED: true
 */

using System;
// Disables lots of warnings that appear in the test suite
#pragma warning disable 0168, 0169, 0219, 0162, 0414
namespace Test {
class Test
{
char comm;
int top;
int [] stack = new int [32];
int ts;
int te;
int act;
int val;

%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{Console.Write( "handle " );fhold;if ( val == 1 )
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
		'{' => {Console.Write( "{ " );fcall *fentry(one);};
		"[" => {Console.Write( "[ " );fcall *fentry(two);};
		"}" sp* => {Console.Write( "} " );fret;};
		[a-z]+ => {Console.Write( "word " );val = 1;
fgoto *fentry(handle);};
		' ' => {Console.Write( "space " );};
	*|;

	two := |*
		'{' => {Console.Write( "{ " );fcall *fentry(one);};
		"[" => {Console.Write( "[ " );fcall *fentry(two);};
		']' sp* => {Console.Write( "] " );fret;};
		[a-z]+ => {Console.Write( "word " );val = 2;
fgoto *fentry(handle);};
		' ' => {Console.Write( "space " );};
	*|;

	main := |* 
		'{' => {Console.Write( "{ " );fcall one;};
		"[" => {Console.Write( "[ " );fcall two;};
		[a-z]+ => {Console.Write( "word " );val = 3;
fgoto handle;};
		[a-z] ' foil' => {Console.Write( "this is the foil" );};
		' ' => {Console.Write( "space " );};
		'\n';
	*|;
}%%


%% write data;
int cs;

void init()
{
	%% write init;
}

void exec( char[] data, int len )
{
	int p = 0;
	int pe = len;
	int eof = len;
	string _s;
	char [] buffer = new char [1024];
	int blen = 0;
	%% write exec;
}

void finish( )
{
	if ( cs >= GotoCallRet_first_final )
		Console.WriteLine( "ACCEPT" );
	else
		Console.WriteLine( "FAIL" );
}

static readonly string[] inp = {
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
};


static readonly int inplen = 11;

public static void Main (string[] args)
{
	Test machine = new Test();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].ToCharArray(), inp[i].Length );
		machine.finish();
	}
}
}
}
