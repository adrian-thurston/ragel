//
// @LANG: julia
// @GENERATED: true
//




%%{
	machine any1;
	main := any;
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

	if ( cs >= any1_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "" );
	m( "x" );
	m( "xx" );
