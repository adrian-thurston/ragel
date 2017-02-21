/*
 * @LANG: java
 * @GENERATED: true
 */


class any1_java
{



%%{
	machine any1;
	main := any;
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

	String _s;
	%% write exec;
}

void finish( )
{
	if ( cs >= any1_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"",
"x",
"xx",
};

static final int inplen = 3;

public static void main (String[] args)
{
	any1_java machine = new any1_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
