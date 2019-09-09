#
# @LANG: ruby
# @GENERATED: true
#


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

def run_machine( data )
	p = 0
	pe = data.length
	eof = data.length
	cs = 0;
	_m = 
	_a = 
	buffer = Array.new
	blen = 0
comm = 1
top = 1
stack = Array.new
ts = 1
te = 1
act = 1
value = 1
	%% write init;
	%% write exec;
	if cs >= patact_first_final
		puts "ACCEPT"
	else
		puts "FAIL"
	end
end

inp = [
"abcd foix\n",
"abcd\nanother\n",
"123 foix\n",
"!abcd foix\n",
"!abcd\nanother\n",
"!123 foix\n",
";",
]

inplen = 7

inp.each { |str| run_machine(str) }

