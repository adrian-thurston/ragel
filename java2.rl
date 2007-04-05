/*
 * @LANG: java
 */

class java2
{
	%%{
		machine java1;
		alphtype int;

		main := 1 2 3 4 (
			5 6 7 8 | 
			9 10 11 12
			) 1073741824;
		
	}%%

	%% write data;

	static void test( int data[] )
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

	static final int t1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 1073741824 };
	static final int t2[] = { 1, 2, 3, 4, 9, 10, 11, 12, 1073741824 };
	static final int t3[] = { 1, 2, 3, 4, 1073741824 };

	public static void main( String args[] )
	{
		test( t1 );
		test( t2 );
		test( t3 );
	}
}

/* _____OUTPUT_____
ACCEPT
ACCEPT
FAIL
*/
