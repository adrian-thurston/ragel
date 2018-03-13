//
// @LANG: julia
// @GENERATED: true
//




%%{
	machine zlen1;
	main := zlen;
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

	if ( cs >= zlen1_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "" );
	m( "x" );
