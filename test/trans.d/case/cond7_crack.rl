//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int i;
int c;

%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = ( int ( fc ) ) 
;
cout.format( "item: " );
cout.format( c );
cout.format( "\n" );
}

	count = [0-9] @{i = ( int ( fc - 48 ) ) 
;
cout.format( "count: " );
cout.format( i );
cout.format( "\n" );
};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
}%%



%% write data;

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;

	%% write init;
	%% write exec;

	if ( cs >= foo_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "00\n" );
	m( "019\n" );
	m( "190\n" );
	m( "1719\n" );
	m( "1040000\n" );
	m( "104000a\n" );
	m( "104000\n" );
}

main();
