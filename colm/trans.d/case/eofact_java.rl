/*
 * @LANG: java
 * @GENERATED: true
 */


class eofact_java
{



%%{
	machine eofact;

	action a1 {System.out.print( "a1\n" );
}
	action a2 {System.out.print( "a2\n" );
}
	action a3 {System.out.print( "a3\n" );
}
	action a4 {System.out.print( "a4\n" );
}


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
	if ( cs >= eofact_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
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

static final int inplen = 9;

public static void main (String[] args)
{
	eofact_java machine = new eofact_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
