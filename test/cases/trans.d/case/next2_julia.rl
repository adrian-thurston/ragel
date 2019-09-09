//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine next2;

	unused := 'unused';

	one := 'one' @{print( "one\n" );
target = fentry(main);
fnext *target;};

	two := 'two' @{print( "two\n" );
target = fentry(main);
fnext *target;};

	three := 'three' @{print( "three\n" );
target = fentry(main);
fnext *target;};

	main :=  (
		'1' @{target = fentry(one);
fnext *target;last = 1;
} |

		'2' @{target = fentry(two);
fnext *target;last = 2;
} |

		# This one is conditional based on the last.
		'3' @{if ( last == 2 )
	target = fentry(three);
fnext *target;
end
last = 3;
} 'x' |

		'\n'
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
last = 0;

	%% write init;
	%% write exec;

	if ( cs >= next2_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "1one3x2two3three\n" );
