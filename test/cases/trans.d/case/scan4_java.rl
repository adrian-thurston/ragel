/*
 * @LANG: java
 * @GENERATED: true
 */


class scan4_java
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
	  'a' => {System.out.print( "pat1\n" );
};

	  [ab]+ . 'c' => {System.out.print( "pat2\n" );
};

	  any;
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
"ba a",
};

static final int inplen = 1;

public static void main (String[] args)
{
	scan4_java machine = new scan4_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
