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
int j;
int k;

%%{
	machine foo;

	action c1 {i != 0}
	action c2 {j != 0}
	action c3 {k != 0}
	action one {Console.Write( "  one\n" );}
	action two {Console.Write( "  two\n" );}
	action three {Console.Write( "  three\n" );}

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
"000abc\n",
"100abc\n",
"010abc\n",
"110abc\n",
"001abc\n",
"101abc\n",
"011abc\n",
"111abc\n",
};


static readonly int inplen = 8;

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
