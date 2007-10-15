/*
 * @LANG: java
 */

class erract8
{
	%%{
		machine erract8;

		action on_char  { System.out.println("char: " + data[p]); }
		action on_err   { System.out.println("err: " + data[p]); }
		action to_state { System.out.println("to state: " + data[p]); }

		main := 'heXXX' $on_char $err(on_err) $to(to_state);
	}%%

	%% write data;

	static void test( char data[] )
	{
		int cs, p = 0, pe = data.length;
		int eof = pe;
		int top;

		%% write init;
		%% write exec;

		System.out.println("rest: " + data[p] + data[p+1] + data[p+2]);
	}

	public static void main( String args[] )
	{
		test( "hello".toCharArray() );
	}
}

/* _____OUTPUT_____
char: h
to state: h
char: e
to state: e
err: l
rest: llo
*/
