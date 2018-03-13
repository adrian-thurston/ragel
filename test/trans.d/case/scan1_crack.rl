//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int
 ts;
int
 te;
int act;
int token;

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => {cout.format( "on last     " );
if ( p + 1 == te )
{
	cout.format( "yes" );

} 
cout.format( "\n" );
};

		'b'+ => {cout.format( "on next     " );
if ( p + 1 == te )
{
	cout.format( "yes" );

} 
cout.format( "\n" );
};

		'c1' 'dxxx'? => {cout.format( "on lag      " );
if ( p + 1 == te )
{
	cout.format( "yes" );

} 
cout.format( "\n" );
};

		'd1' => {cout.format( "lm switch1  " );
if ( p + 1 == te )
{
	cout.format( "yes" );

} 
cout.format( "\n" );
};
		'd2' => {cout.format( "lm switch2  " );
if ( p + 1 == te )
{
	cout.format( "yes" );

} 
cout.format( "\n" );
};

		[d0-9]+ '.';

		'\n';
	*|;
}%%



%% write data;

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;
	int eof = pe;

	%% write init;
	%% write exec;

	if ( cs >= scanner_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "abbc1d1d2\n" );
}

main();
