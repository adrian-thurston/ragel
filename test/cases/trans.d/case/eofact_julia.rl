//
// @LANG: julia
// @GENERATED: true
//




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

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""

	%% write init;
	%% write exec;

	if ( cs >= eofact_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "" );
	m( "h" );
	m( "hell" );
	m( "hello" );
	m( "hello\n" );
	m( "t" );
	m( "ther" );
	m( "there" );
	m( "friend" );
