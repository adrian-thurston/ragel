/*
 * @LANG: d
 * @GENERATED: true
 */

import std.stdio;
import std.string;

class atoi
{
int neg;
int value;

%%{
	machine atoi;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + cast( int ) ( fc - 48 )
;
}

	action finish {if ( neg != 0 )
{
	value = -1 * value;

} 
}
	action print {printf( "%d", value );
printf( "%.*s", "\n" );}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main := atoi '\n' @print;
}%%



%% write data;
int cs;
int blen;
char buffer[1024];

void init()
{
value = 0;
neg = 0;
	%% write init;
}
void exec( const(char) data[] )
{
	const(char) *p = data.ptr;
	const(char) *pe = data.ptr + data.length;
	char _s[];

	%% write exec;
}

void finish( )
{
	if ( cs >= atoi_first_final )
		writefln( "ACCEPT" );
	else
		writefln( "FAIL" );
}
static const char[][] inp = [
"1\n",
"12\n",
"222222\n",
"+2123\n",
"213 3213\n",
"-12321\n",
"--123\n",
"-99\n",
" -3000\n",
];

int inplen = 9;

}
int main()
{
	atoi m = new atoi();
	int i;
	for ( i = 0; i < m.inplen; i++ ) {
		m.init();
		m.exec( m.inp[i] );
		m.finish();
	}
	return 0;
}

