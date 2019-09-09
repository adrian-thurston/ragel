//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine ncall1;

	unused := 'unused';

	one := 'one' @{print( "one\n" );
fnret;};

	two := 'two' @{print( "two\n" );
fnret;};

	main := (
			'1' @{target = fentry(one);
fncall *target;}
		|	'2' @{target = fentry(two);
fncall *target;}
		|	'\n'
	)*;
}%%



%% write data;

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
top = 0;
stack = Int [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
target = 0;

	%% write init;
	%% write exec;

	if ( cs >= ncall1_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "1one2two1one\n" );
