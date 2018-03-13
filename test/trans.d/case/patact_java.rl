/*
 * @LANG: java
 * @GENERATED: true
 */


class patact_java
{
char comm ;
int top ;
int stack [] = new int[32];
int
 ts ;
int
 te ;
int act ;
int value ;

%%{
	machine patact;

	other := |* 
		[a-z]+ => {System.out.print( "word\n" );
};
		[0-9]+ => {System.out.print( "num\n" );
};
		[\n ] => {System.out.print( "space\n" );
};
	*|;

	exec_test := |* 
		[a-z]+ => {System.out.print( "word (w/lbh)\n" );
fexec te-1;fgoto other;};
		[a-z]+ ' foil' => {System.out.print( "word (c/lbh)\n" );
};
		[\n ] => {System.out.print( "space\n" );
};
		'22' => {System.out.print( "num (w/switch)\n" );
};
		[0-9]+ => {System.out.print( "num (w/switch)\n" );
fexec te-1;fgoto other;};
		[0-9]+ ' foil' => {System.out.print( "num (c/switch)\n" );
};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {System.out.print( "in semi\n" );
fgoto main;};
	*|;

	main := |* 
		[a-z]+ => {System.out.print( "word (w/lbh)\n" );
fhold;fgoto other;};
		[a-z]+ ' foil' => {System.out.print( "word (c/lbh)\n" );
};
		[\n ] => {System.out.print( "space\n" );
};
		'22' => {System.out.print( "num (w/switch)\n" );
};
		[0-9]+ => {System.out.print( "num (w/switch)\n" );
fhold;fgoto other;};
		[0-9]+ ' foil' => {System.out.print( "num (c/switch)\n" );
};
		';' => {System.out.print( "going to semi\n" );
fhold;fgoto semi;};
		'!' => {System.out.print( "immdiate\n" );
fgoto exec_test;};
	*|;
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
	if ( cs >= patact_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"abcd foix\n",
"abcd\nanother\n",
"123 foix\n",
"!abcd foix\n",
"!abcd\nanother\n",
"!123 foix\n",
";",
};

static final int inplen = 7;

public static void main (String[] args)
{
	patact_java machine = new patact_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
