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
int value;

%%{
	machine patact;

	other := |* 
		[a-z]+ => {Console.Write( "word\n" );};
		[0-9]+ => {Console.Write( "num\n" );};
		[\n ] => {Console.Write( "space\n" );};
	*|;

	exec_test := |* 
		[a-z]+ => {Console.Write( "word (w/lbh)\n" );fexec te-1;fgoto other;};
		[a-z]+ ' foil' => {Console.Write( "word (c/lbh)\n" );};
		[\n ] => {Console.Write( "space\n" );};
		'22' => {Console.Write( "num (w/switch)\n" );};
		[0-9]+ => {Console.Write( "num (w/switch)\n" );fexec te-1;fgoto other;};
		[0-9]+ ' foil' => {Console.Write( "num (c/switch)\n" );};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {Console.Write( "in semi\n" );fgoto main;};
	*|;

	main := |* 
		[a-z]+ => {Console.Write( "word (w/lbh)\n" );fhold;fgoto other;};
		[a-z]+ ' foil' => {Console.Write( "word (c/lbh)\n" );};
		[\n ] => {Console.Write( "space\n" );};
		'22' => {Console.Write( "num (w/switch)\n" );};
		[0-9]+ => {Console.Write( "num (w/switch)\n" );fhold;fgoto other;};
		[0-9]+ ' foil' => {Console.Write( "num (c/switch)\n" );};
		';' => {Console.Write( "going to semi\n" );fhold;fgoto semi;};
		'!' => {Console.Write( "immdiate\n" );fgoto exec_test;};
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
	if ( cs >= patact_first_final )
		Console.WriteLine( "ACCEPT" );
	else
		Console.WriteLine( "FAIL" );
}

static readonly string[] inp = {
"abcd foix\n",
"abcd\nanother\n",
"123 foix\n",
"!abcd foix\n",
"!abcd\nanother\n",
"!123 foix\n",
";",
};


static readonly int inplen = 7;

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
