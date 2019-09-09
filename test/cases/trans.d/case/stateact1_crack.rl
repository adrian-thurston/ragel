//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;




%%{
	machine state_act;

	action a1 {cout.format( "a1\n" );
}
	action a2 {cout.format( "a2\n" );
}
	action b1 {cout.format( "b1\n" );
}
	action b2 {cout.format( "b2\n" );
}
	action c1 {cout.format( "c1\n" );
}
	action c2 {cout.format( "c2\n" );
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

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;

	%% write init;
	%% write exec;

	if ( cs >= state_act_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
	m( "hi\nhi\n" );
}

main();
