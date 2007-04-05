/*
 * @LANG: java
 */

class java1
{
	%%{
		machine java1;

		one := 'one\n';
		two := 'two\n';
		four := 'four\n';

		main := 
			( 'hello' | 'there' | 'friend' ) 
			'\n' @{int s = fentry(one); fgoto *s; char c = fc;}
			( 'one' | 'two' | 'four' ) '\n';
	}%%

	%% write data;

	static void test( char data[] )
	{
		int cs, p = 0, pe = data.length;
		int top;

		%% write init;
		%% write exec;

		if ( cs >= java1_first_final )
			System.out.println( "ACCEPT" );
		else
			System.out.println( "FAIL" );
	}

	public static void main( String args[] )
	{
		test( "hello\none\n".toCharArray() );
		test( "there\ntwo\n".toCharArray() );
		test( "friend\nfour\n".toCharArray() );
	}
}

/* _____OUTPUT_____
ACCEPT
FAIL
FAIL
*/
