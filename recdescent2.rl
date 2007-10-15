/*
 * @LANG: java
 */

class recdescent2
{
	%%{
		machine recdescent;

		prepush { 
			if ( top == stack_size ) {
				System.out.print( "growing stack\n" );
				stack_size = top * 2;
				// Don't actually bother to resize here, but we do print messages.
				//stack = (int*)realloc( stack, sizeof(int)*stack_size );
			}
		}

		postpop { 
			if ( stack_size > (top * 4) ) {
				stack_size = top * 2;
				// Don't actually bother to resize here, but we do print messages.
				//stack = (int*)realloc( stack, sizeof(int)*stack_size );
				System.out.print( "shrinking stack\n" );
			}
		}

		action item_start { item = p; }

		action item_finish
		{
			String item_data = new String ( data, item, p-item );
			System.out.print( "item: " );
			System.out.print( item_data );
			System.out.print( "\n" );
		}

		action call_main
		{
			System.out.print( "calling main\n" );
			fcall main;
		}

		action return_main
		{
			if ( top == 0 ) {
				System.out.print( "STRAY CLOSE\n" );
				fbreak;
			}

			System.out.print( "returning from main\n" );
			fhold;
			fret;
		}

		id = [a-zA-Z_]+;
		number = [0-9]+;
		ws = [ \t\n]+;

		main := ( 
			ws |
			( number | id ) >item_start %item_finish |

			'{' @call_main '}' |

			'}' @return_main
		)**;
	}%%

	%% write data;

	static void test( char data[] )
	{
		int cs, p = 0, pe = data.length, eof = data.length, item = 0;
		int stack[] = new int[1024];
		int stack_size = 1;
		int top;

		%% write init;
		%% write exec;

		if ( cs == recdescent_error )
			System.out.println( "SCANNER ERROR" );
	}

	public static void main( String args[] )
	{
		test( "88 foo { 99 {{{{}}}}{ } }".toCharArray() );
		test( "76 } sadf".toCharArray() );
	}
}

/* _____OUTPUT_____
item: 88
item: foo
calling main
item: 99
calling main
growing stack
calling main
growing stack
calling main
calling main
growing stack
returning from main
returning from main
returning from main
returning from main
shrinking stack
calling main
returning from main
returning from main
shrinking stack
item: 76
STRAY CLOSE
*/
