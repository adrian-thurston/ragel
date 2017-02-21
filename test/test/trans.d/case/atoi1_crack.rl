//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int neg;
int value;

%%{
	machine atoi;

	action begin {neg = 0;
value = 0;
}

	action see_neg {neg = 1;
}

	action add_digit {value = value * 10 + ( int ( fc - 48 ) ) 
;
}

	action finish {if ( neg != 0 )
{
	value = -1 * value;

} 
}
	action print {cout.format( value );
cout.format( "\n" );
}

	atoi = (
		('-'@see_neg | '+')? (digit @add_digit)+
	) >begin %finish;

	main := atoi '\n' @print;
}%%



%% write data;

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;
value = 0;
neg = 0;

	%% write init;
	%% write exec;

	if ( cs >= atoi_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "1\n" );
	m( "12\n" );
	m( "222222\n" );
	m( "+2123\n" );
	m( "213 3213\n" );
	m( "-12321\n" );
	m( "--123\n" );
	m( "-99\n" );
	m( " -3000\n" );
}

main();
