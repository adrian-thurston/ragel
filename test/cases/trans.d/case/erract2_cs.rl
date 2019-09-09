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
	machine erract;

	action err_start {Console.Write( "err_start\n" );}
	action err_all {Console.Write( "err_all\n" );}
	action err_middle {Console.Write( "err_middle\n" );}
	action err_out {Console.Write( "err_out\n" );}

	action eof_start {Console.Write( "eof_start\n" );}
	action eof_all {Console.Write( "eof_all\n" );}
	action eof_middle {Console.Write( "eof_middle\n" );}
	action eof_out {Console.Write( "eof_out\n" );}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
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
	if ( cs >= erract_first_final )
		Console.WriteLine( "ACCEPT" );
	else
		Console.WriteLine( "FAIL" );
}

static readonly string[] inp = {
"",
"h",
"x",
"he",
"hx",
"hel",
"hex",
"hell",
"helx",
"hello",
"hellx",
"hello\n",
"hellox",
};


static readonly int inplen = 13;

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
