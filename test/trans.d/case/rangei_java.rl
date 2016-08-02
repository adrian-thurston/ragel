/*
 * @LANG: java
 * @GENERATED: true
 */


class rangei_java
{


%%{
	machine rangei;

	main := 
		'a' ../i 'z' .
		'A' ../i 'Z' .
		60 ../i 93 .
		94 ../i 125 .
		86 ../i 101 .
		60 ../i 125
		'';
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
	if ( cs >= rangei_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"AaBbAa",
"Aa`bAa",
"AaB@Aa",
"AaBbMa",
"AaBbma",
};

static final int inplen = 5;

public static void main (String[] args)
{
	rangei_java machine = new rangei_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
