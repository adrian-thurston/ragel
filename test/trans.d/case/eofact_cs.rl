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



%%{
	machine eofact;

	action a1 {Console.Write( "a1\n" );}
	action a2 {Console.Write( "a2\n" );}
	action a3 {Console.Write( "a3\n" );}
	action a4 {Console.Write( "a4\n" );}


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

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
	if ( cs >= eofact_first_final )
		Console.WriteLine( "ACCEPT" );
	else
		Console.WriteLine( "FAIL" );
}

static readonly string[] inp = {
"",
"h",
"hell",
"hello",
"hello\n",
"t",
"ther",
"there",
"friend",
};


static readonly int inplen = 9;

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
