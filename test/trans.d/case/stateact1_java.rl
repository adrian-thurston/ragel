/*
 * @LANG: java
 * @GENERATED: true
 */


class stateact1_java
{



%%{
	machine state_act;

	action a1 {System.out.print( "a1\n" );
}
	action a2 {System.out.print( "a2\n" );
}
	action b1 {System.out.print( "b1\n" );
}
	action b2 {System.out.print( "b2\n" );
}
	action c1 {System.out.print( "c1\n" );
}
	action c2 {System.out.print( "c2\n" );
}
	action next_again {fnext again;}

	hi = 'hi';
	line = again: 
			hi 
				>to b1 
				>from b2 
			'\n' 
				>to c1 
				>from c2 
				@next_again;
		 
	main := line*
			>to a1 
			>from a2;
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
	if ( cs >= state_act_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"hi\nhi\n",
};

static final int inplen = 1;

public static void main (String[] args)
{
	stateact1_java machine = new stateact1_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
