#
# @LANG: ruby
# @GENERATED: true
#




%%{
	machine eofact;

	action a1 {print( "a1\n" );
}
	action a2 {print( "a2\n" );
}
	action a3 {print( "a3\n" );
}
	action a4 {print( "a4\n" );
}


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

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
	if cs >= eofact_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"",
"h",
"hell",
"hello",
"hello\n",
"t",
"ther",
"there",
"friend",
]

inplen = 9

inp.each { |str| run_machine(str) }

