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
int i;
int c;

%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = ( int ) ( fc )
;
Console.Write( "item: " );Console.Write( c );Console.Write( "\n" );}

	count = [0-9] @{i = ( int ) ( fc - 48 )
;
Console.Write( "count: " );Console.Write( i );Console.Write( "\n" );};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
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
	if ( cs >= foo_first_final )
		Console.WriteLine( "ACCEPT" );
	else
		Console.WriteLine( "FAIL" );
}

static readonly string[] inp = {
"00\n",
"019\n",
"190\n",
"1719\n",
"1040000\n",
"104000a\n",
"104000\n",
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
