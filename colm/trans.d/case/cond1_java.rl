/*
 * @LANG: java
 * @GENERATED: true
 */


class cond1_java
{
int i ;
int j ;
int k ;

%%{
	machine foo;

	action c1 {i != 0}
	action c2 {j != 0}
	action c3 {k != 0}
	action one {System.out.print( "  one\n" );
}
	action two {System.out.print( "  two\n" );
}
	action three {System.out.print( "  three\n" );
}

	action seti {if ( fc == 48 )
{
	i = 0;

} 
else {
	i = 1;

}
}
	action setj {if ( fc == 48 )
{
	j = 0;

} 
else {
	j = 1;

}
}
	action setk {if ( fc == 48 )
{
	k = 0;

} 
else {
	k = 1;

}
}

	action break {fnbreak;}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
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
"000abc\n",
"100abc\n",
"010abc\n",
"110abc\n",
"001abc\n",
"101abc\n",
"011abc\n",
"111abc\n",
};

static final int inplen = 8;

public static void main (String[] args)
{
	cond1_java machine = new cond1_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
