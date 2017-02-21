/*
 * @LANG: java
 * @GENERATED: true
 */


class atoi1_java
{
int neg ;
int value ;

%%{
	machine atoi;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + ( int ) ( fc - 48 )
;
}

	action finish {if ( neg != 0 )
{
	value = -1 * value;

} 
}
	action print {System.out.print( value );
System.out.print( "\n" );
}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main := atoi '\n' @print;
}%%



%% write data;
int cs;

void init()
{
value = 0;
neg = 0;
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
	if ( cs >= atoi_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"1\n",
"12\n",
"222222\n",
"+2123\n",
"213 3213\n",
"-12321\n",
"--123\n",
"-99\n",
" -3000\n",
};

static final int inplen = 9;

public static void main (String[] args)
{
	atoi1_java machine = new atoi1_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
