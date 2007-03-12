/*
 * @LANG: java
 * @ALLOW_GENFLAGS: -T0
 */

class export2
{
	%%{
		machine test;

		export c1 = 'c';
		export c2 = 'z';
		export c3 = 't';

		commands := (
			c1 . digit* '\n' @{ System.out.println( "c1" );} |
			c2 . alpha* '\n' @{ System.out.println( "c2" );}|
			c3 . '.'* '\n' @{ System.out.println( "c3" );}
		)*;
			
		main := any*;
	}%%

	%% write exports;
	%% write data;

	static void test( char data[] )
	{
		int cs, p = 0, pe = data.length;
		int top;

		cs = test_en_commands;
		%% write exec;

		if ( cs >= test_first_final )
			System.out.println( "ACCEPT" );
		else
			System.out.println( "FAIL" );
	}

	public static void main( String args[] )
	{
		char data[] = { 
			test_ex_c1, '1', '2', '\n', 
			test_ex_c2, 'a', 'b', '\n', 
			test_ex_c3, '.', '.', '\n',
		};
		test( data );
	}
}


/* _____OUTPUT_____
c1
c2
c3
ACCEPT
*/
