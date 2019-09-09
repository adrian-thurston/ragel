#
# @LANG: ruby
# @GENERATED: true
#


%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => {print( "on last     " );
if ( p + 1 == te )
	print( "yes" );

end
print( "\n" );
};

		'b'+ => {print( "on next     " );
if ( p + 1 == te )
	print( "yes" );

end
print( "\n" );
};

		'c1' 'dxxx'? => {print( "on lag      " );
if ( p + 1 == te )
	print( "yes" );

end
print( "\n" );
};

		'd1' => {print( "lm switch1  " );
if ( p + 1 == te )
	print( "yes" );

end
print( "\n" );
};
		'd2' => {print( "lm switch2  " );
if ( p + 1 == te )
	print( "yes" );

end
print( "\n" );
};

		[d0-9]+ '.';

		'\n';
	*|;
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
ts = 1
te = 1
act = 1
token = 1
	%% write init;
	%% write exec;
	if cs >= scanner_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"abbc1d1d2\n",
]

inplen = 1

inp.each { |str| run_machine(str) }

