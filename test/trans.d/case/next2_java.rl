/*
 * @LANG: java
 * @GENERATED: true
 */


class next2_java
{
int target ;
int last ;

%%{
	machine next2;

	unused := 'unused';

	one := 'one' @{System.out.print( "one\n" );
target = fentry(main);
fnext *target;};

	two := 'two' @{System.out.print( "two\n" );
target = fentry(main);
fnext *target;};

	three := 'three' @{System.out.print( "three\n" );
target = fentry(main);
fnext *target;};

	main :=  (
		'1' @{target = fentry(one);
fnext *target;last = 1;
} |

		'2' @{target = fentry(two);
fnext *target;last = 2;
} |

		# This one is conditional based on the last.
		'3' @{if ( last == 2 )
{
	target = fentry(three);
fnext *target;
} 
last = 3;
} 'x' |

		'\n'
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
	if ( cs >= next2_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"1one3x2two3three\n",
};

static final int inplen = 1;

public static void main (String[] args)
{
	next2_java machine = new next2_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
