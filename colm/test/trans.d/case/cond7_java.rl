/*
 * @LANG: java
 * @GENERATED: true
 */


class cond7_java
{
int i ;
int c ;

%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = ( int ) ( fc )
;
System.out.print( "item: " );
System.out.print( c );
System.out.print( "\n" );
}

	count = [0-9] @{i = ( int ) ( fc - 48 )
;
System.out.print( "count: " );
System.out.print( i );
System.out.print( "\n" );
};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
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
	if ( cs >= foo_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"00\n",
"019\n",
"190\n",
"1719\n",
"1040000\n",
"104000a\n",
"104000\n",
};

static final int inplen = 7;

public static void main (String[] args)
{
	cond7_java machine = new cond7_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
