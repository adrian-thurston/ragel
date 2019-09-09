//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

int i;
int j;
int k;

%%{
	machine foo;

	action c1 {i != 0}
	action c2 {j != 0}
	action c3 {k != 0}
	action one {cout.format( "  one\n" );
}
	action two {cout.format( "  two\n" );
}
	action three {cout.format( "  three\n" );
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
	m( "000abc\n" );
	m( "100abc\n" );
	m( "010abc\n" );
	m( "110abc\n" );
	m( "001abc\n" );
	m( "101abc\n" );
	m( "011abc\n" );
	m( "111abc\n" );
}

main();
