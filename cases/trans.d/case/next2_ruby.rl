#
# @LANG: ruby
# @GENERATED: true
#


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

def run_machine( data )
	p = 0
	pe = data.length
	eof = data.length
	cs = 0;
	_m = 
	_a = 
	buffer = Array.new
	blen = 0
target = 1
last = 1
	%% write init;
	%% write exec;
	if cs >= next2_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"1one3x2two3three\n",
]

inplen = 1

inp.each { |str| run_machine(str) }

