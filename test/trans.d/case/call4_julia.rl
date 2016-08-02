//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine call4;

	unused := 'unused';

	one := 'one' @{print( "one\n" );
fret;};

	two := 'two' @{print( "two\n" );
fret;};

	main := (
			'1' @{target = fentry(one);
fcall *target;}
		|	'2' @{target = fentry(two);
fcall *target;}
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
target = 0;
top = 0;
stack = Int [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];

	%% write init;
	%% write exec;

	if ( cs >= call4_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "1one2two1one\n" );
