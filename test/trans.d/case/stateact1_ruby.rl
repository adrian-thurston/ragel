#
# @LANG: ruby
# @GENERATED: true
#




%%{
	machine state_act;

	action a1 {print( "a1\n" );
}
	action a2 {print( "a2\n" );
}
	action b1 {print( "b1\n" );
}
	action b2 {print( "b2\n" );
}
	action c1 {print( "c1\n" );
}
	action c2 {print( "c2\n" );
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

def run_machine( data )
	p = 0
	pe = data.length
	eof = data.length
	cs = 0;
	_m = 
	_a = 
	buffer = Array.new
	blen = 0
	%% write init;
	%% write exec;
	if cs >= state_act_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"hi\nhi\n",
]

inplen = 1

inp.each { |str| run_machine(str) }

