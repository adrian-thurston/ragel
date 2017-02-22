/*
 * @LANG: java
 * @GENERATED: true
 */


class erract2_java
{



%%{
	machine erract;

	action err_start {System.out.print( "err_start\n" );
}
	action err_all {System.out.print( "err_all\n" );
}
	action err_middle {System.out.print( "err_middle\n" );
}
	action err_out {System.out.print( "err_out\n" );
}

	action eof_start {System.out.print( "eof_start\n" );
}
	action eof_all {System.out.print( "eof_all\n" );
}
	action eof_middle {System.out.print( "eof_middle\n" );
}
	action eof_out {System.out.print( "eof_out\n" );
}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
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
	if ( cs >= erract_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"",
"h",
"x",
"he",
"hx",
"hel",
"hex",
"hell",
"helx",
"hello",
"hellx",
"hello\n",
"hellox",
};

static final int inplen = 13;

public static void main (String[] args)
{
	erract2_java machine = new erract2_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
