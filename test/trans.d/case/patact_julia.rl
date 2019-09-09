//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine patact;

	other := |* 
		[a-z]+ => {print( "word\n" );
};
		[0-9]+ => {print( "num\n" );
};
		[\n ] => {print( "space\n" );
};
	*|;

	exec_test := |* 
		[a-z]+ => {print( "word (w/lbh)\n" );
fexec te-1;fgoto other;};
		[a-z]+ ' foil' => {print( "word (c/lbh)\n" );
};
		[\n ] => {print( "space\n" );
};
		'22' => {print( "num (w/switch)\n" );
};
		[0-9]+ => {print( "num (w/switch)\n" );
fexec te-1;fgoto other;};
		[0-9]+ ' foil' => {print( "num (c/switch)\n" );
};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {print( "in semi\n" );
fgoto main;};
	*|;

	main := |* 
		[a-z]+ => {print( "word (w/lbh)\n" );
fhold;fgoto other;};
		[a-z]+ ' foil' => {print( "word (c/lbh)\n" );
};
		[\n ] => {print( "space\n" );
};
		'22' => {print( "num (w/switch)\n" );
};
		[0-9]+ => {print( "num (w/switch)\n" );
fhold;fgoto other;};
		[0-9]+ ' foil' => {print( "num (c/switch)\n" );
};
		';' => {print( "going to semi\n" );
fhold;fgoto semi;};
		'!' => {print( "immdiate\n" );
fgoto exec_test;};
	*|;
}%%



%% write data;

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""
comm = 0;
top = 0;
stack = Int [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
ts = 0;
te = 0;
act = 0;
value = 0;

	%% write init;
	%% write exec;

	if ( cs >= patact_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "abcd foix\n" );
	m( "abcd\nanother\n" );
	m( "123 foix\n" );
	m( "!abcd foix\n" );
	m( "!abcd\nanother\n" );
	m( "!123 foix\n" );
	m( ";" );
