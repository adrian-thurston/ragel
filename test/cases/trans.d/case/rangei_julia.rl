//
// @LANG: julia
// @GENERATED: true
//



%%{
	machine rangei;

	main := 
		'a' ../i 'z' .
		'A' ../i 'Z' .
		60 ../i 93 .
		94 ../i 125 .
		86 ../i 101 .
		60 ../i 125
		'';
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

	if ( cs >= rangei_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "AaBbAa" );
	m( "Aa`bAa" );
	m( "AaB@Aa" );
	m( "AaBbMa" );
	m( "AaBbma" );
