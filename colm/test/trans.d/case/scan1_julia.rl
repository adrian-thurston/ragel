//
// @LANG: julia
// @GENERATED: true
//


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

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
ts = 0;
te = 0;
act = 0;
token = 0;

	%% write init;
	%% write exec;

	if ( cs >= scanner_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "abbc1d1d2\n" );
