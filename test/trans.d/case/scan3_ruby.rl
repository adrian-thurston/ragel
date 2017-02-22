#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => {print( "pat1\n" );
};
		'b' => {print( "pat2\n" );
};
        [ab] any*  => {print( "pat3\n" );
};
	*|;
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
ts = 1
te = 1
act = 1
token = 1
	%% write init;
	%% write exec;
	if cs >= scanner_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"ab89",
]

inplen = 1

inp.each { |str| run_machine(str) }

