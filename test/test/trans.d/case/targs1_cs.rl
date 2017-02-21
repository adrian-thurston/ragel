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
int return_to;

%%{
	machine targs1;

	unused := 'unused';

	one := 'one' @{Console.Write( "one\n" );fnext *return_to;};

	two := 'two' @{Console.Write( "two\n" );fnext *return_to;};

	main := (
			'1' @{return_to = ftargs;
fnext one;}
		|	'2' @{return_to = ftargs;
fnext two;}
		|	'\n'
	)*;
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
	if ( cs >= targs1_first_final )
		Console.WriteLine( "ACCEPT" );
	else
		Console.WriteLine( "FAIL" );
}

static readonly string[] inp = {
"1one2two1one\n",
};


static readonly int inplen = 1;

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
