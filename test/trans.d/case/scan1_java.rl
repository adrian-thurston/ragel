/*
 * @LANG: java
 * @GENERATED: true
 */


class scan1_java
{
int
 ts ;
int
 te ;
int act ;
int token ;

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => {System.out.print( "on last     " );
if ( p + 1 == te )
{
	System.out.print( "yes" );

} 
System.out.print( "\n" );
};

		'b'+ => {System.out.print( "on next     " );
if ( p + 1 == te )
{
	System.out.print( "yes" );

} 
System.out.print( "\n" );
};

		'c1' 'dxxx'? => {System.out.print( "on lag      " );
if ( p + 1 == te )
{
	System.out.print( "yes" );

} 
System.out.print( "\n" );
};

		'd1' => {System.out.print( "lm switch1  " );
if ( p + 1 == te )
{
	System.out.print( "yes" );

} 
System.out.print( "\n" );
};
		'd2' => {System.out.print( "lm switch2  " );
if ( p + 1 == te )
{
	System.out.print( "yes" );

} 
System.out.print( "\n" );
};

		[d0-9]+ '.';

		'\n';
	*|;
}%%



%% write data;
int cs;

void init()
{
	%% write init;
}

void exec( char data[], int len )
{
	char buffer [] = new char[1024];
	int blen = 0;
	int p = 0;
	int pe = len;

	int eof = len;
	String _s;
	%% write exec;
}

void finish( )
{
	if ( cs >= scanner_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"abbc1d1d2\n",
};

static final int inplen = 1;

public static void main (String[] args)
{
	scan1_java machine = new scan1_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
