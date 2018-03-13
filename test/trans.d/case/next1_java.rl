/*
 * @LANG: java
 * @GENERATED: true
 */


class next1_java
{
int target ;

%%{
	machine next1;

	unused := 'unused';

	one := 'one' @{System.out.print( "one\n" );
target = fentry(main);
fnext *target;};

	two := 'two' @{System.out.print( "two\n" );
target = fentry(main);
fnext *target;};

	main := 
		'1' @{target = fentry(one);
fnext *target;}
	|	'2' @{target = fentry(two);
fnext *target;}
	|	'\n';
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
	if ( cs >= next1_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"1one2two1one\n",
};

static final int inplen = 1;

public static void main (String[] args)
{
	next1_java machine = new next1_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
