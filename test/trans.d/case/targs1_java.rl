/*
 * @LANG: java
 * @GENERATED: true
 */


class targs1_java
{
int return_to ;

%%{
	machine targs1;

	unused := 'unused';

	one := 'one' @{System.out.print( "one\n" );
fnext *return_to;};

	two := 'two' @{System.out.print( "two\n" );
fnext *return_to;};

	main := (
			'1' @{return_to = ftargs;
fnext one;}
		|	'2' @{return_to = ftargs;
fnext two;}
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
	if ( cs >= targs1_first_final )
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
	targs1_java machine = new targs1_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
