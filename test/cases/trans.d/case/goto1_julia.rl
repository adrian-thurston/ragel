//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine goto1;

	unused := 'unused';

	one := 'one' @{print( "one\n" );
target = fentry(main);
fgoto *target;};

	two := 'two' @{print( "two\n" );
target = fentry(main);
fgoto *target;};

	main := 
		'1' @{target = fentry(one);
fgoto *target;}
	|	'2' @{target = fentry(two);
fgoto *target;}
	|	'\n';
}%%



%% write data;

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
target = 0;

	%% write init;
	%% write exec;

	if ( cs >= goto1_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "1one2two1one\n" );
