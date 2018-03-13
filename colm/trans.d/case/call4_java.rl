/*
 * @LANG: java
 * @GENERATED: true
 */


class call4_java
{
int target ;
int top ;
int stack [] = new int[32];

%%{
	machine call4;

	unused := 'unused';

	one := 'one' @{System.out.print( "one\n" );
fret;};

	two := 'two' @{System.out.print( "two\n" );
fret;};

	main := (
			'1' @{target = fentry(one);
fcall *target;}
		|	'2' @{target = fentry(two);
fcall *target;}
		|	'\n'
	)*;
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
	if ( cs >= call4_first_final )
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
	call4_java machine = new call4_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
