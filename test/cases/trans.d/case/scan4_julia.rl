//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
	  'a' => {print( "pat1\n" );
};

	  [ab]+ . 'c' => {print( "pat2\n" );
};

	  any;
	*|;
}%%



%% write data;

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
ts = 0;
te = 0;
act = 0;
token = 0;

	%% write init;
	%% write exec;

	if ( cs >= scanner_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "ba a" );
