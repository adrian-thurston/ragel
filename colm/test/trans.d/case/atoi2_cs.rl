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
int neg;
int value;

%%{
	machine state_chart;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + ( int ) ( fc - 48 )
;
}

	action finish {if ( neg != 0 )
{
	value = -1 * value;

} 
}

	atoi = (
		start: (
			'-' @see_neg ->om_num | 
			'+' ->om_num |
			[0-9] @add_digit ->more_nums
		),

		# One or more nums.
		om_num: (
			[0-9] @add_digit ->more_nums
		),

		# Zero ore more nums.
		more_nums: (
			[0-9] @add_digit ->more_nums |
			'' -> final
		)
	) >begin %finish;

	action oneof {Console.Write( value );Console.Write( "\n" );}
	main := ( atoi '\n' @oneof )*;
}%%


%% write data;
int cs;

void init()
{
value = 0;
neg = 0;
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
	if ( cs >= state_chart_first_final )
		Console.WriteLine( "ACCEPT" );
	else
		Console.WriteLine( "FAIL" );
}

static readonly string[] inp = {
"1\n",
"12\n",
"222222\n",
"+2123\n",
"213 3213\n",
"-12321\n",
"--123\n",
"-99\n",
" -3000\n",
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
