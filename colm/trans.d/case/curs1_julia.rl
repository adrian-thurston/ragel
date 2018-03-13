//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine curs1;

	unused := 'unused';

	one := 'one' @{print( "one\n" );
fnext *return_to;};

	two := 'two' @{print( "two\n" );
fnext *return_to;};

	main := 
		'1' @{return_to = fcurs;
fnext one;}
	|	'2' @{return_to = fcurs;
fnext two;}
	|	'\n';
}%%



%% write data;

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
return_to = 0;

	%% write init;
	%% write exec;

	if ( cs >= curs1_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "1one2two1one\n" );
