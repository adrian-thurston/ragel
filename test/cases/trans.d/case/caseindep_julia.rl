//
// @LANG: julia
// @GENERATED: true
//



%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
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

	if ( cs >= indep_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "1hello\n" );
	m( "1HELLO\n" );
	m( "1HeLLo\n" );
	m( "2hello\n" );
	m( "2HELLO\n" );
	m( "2HeLLo\n" );
	m( "3hello\n" );
	m( "3HELLO\n" );
	m( "3HeLLo\n" );
	m( "4hello\n" );
	m( "4HELLO\n" );
	m( "4HeLLo\n" );
	m( "5hello\n" );
	m( "5HELLO\n" );
	m( "5HeLLo\n" );
	m( "6hello\n" );
	m( "6HELLO\n" );
	m( "6HeLLo\n" );
