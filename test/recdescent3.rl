#
# @LANG: ruby
#

%%{
    machine recdescent3;

	prepush { 
		if top == stack_size
			print( "growing stack\n" );
			stack_size = top * 2;
			# Don't actually bother to resize here, but we do print messages.
			# stack = (int*)realloc( stack, sizeof(int)*stack_size );
		end 
	}

	postpop { 
		if stack_size > (top * 4)
			print( "shrinking stack\n" );
			stack_size = top * 2;
			# Don't actually bother to resize here, but we do print messages.
			# stack = (int*)realloc( stack, sizeof(int)*stack_size );
		end
	}

	action item_start { item = p; }

	action item_finish
	{
		print( "item: " );
		print( data[item..p-1] );
		print( "\n" );
	}

	action call_main
	{
		print( "calling main\n" );
		fcall main;
	}

	action return_main
	{
		if top == 0 
			print( "STRAY CLOSE\n" );
			fbreak;
		end

		print( "returning from main\n" );
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

def run_machine( data )
	item = 0;
	p = 0;
	pe = data.length;
	eof = pe;
	cs = 0;
	stack = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
	stack_size = 1;
	top = 0;

	%% write init;
	%% write exec;

	if cs == recdescent3_error
		puts "SCANNER_ERROR"
	end
end

inp = [
		"88 foo { 99 {{{{}}}}{ } }",
		"76 } sadf"
]

inp.each { |str| run_machine(str) }

=begin _____OUTPUT_____
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
=end _____OUTPUT_____
