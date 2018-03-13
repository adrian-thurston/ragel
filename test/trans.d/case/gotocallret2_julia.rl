//
// @LANG: julia
// @GENERATED: true
//


%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{print( "handle " );
fhold;if ( val == 1 )
	fnext *fentry(one);
end
if ( val == 2 )
	fnext *fentry(two);
end
if ( val == 3 )
	fnext main;
end
};

	one := |*
		'{' => {print( "{ " );
fcall *fentry(one);};
		"[" => {print( "[ " );
fcall *fentry(two);};
		"}" sp* => {print( "} " );
fret;};
		[a-z]+ => {print( "word " );
val = 1;
fgoto *fentry(handle);};
		' ' => {print( "space " );
};
	*|;

	two := |*
		'{' => {print( "{ " );
fcall *fentry(one);};
		"[" => {print( "[ " );
fcall *fentry(two);};
		']' sp* => {print( "] " );
fret;};
		[a-z]+ => {print( "word " );
val = 2;
fgoto *fentry(handle);};
		' ' => {print( "space " );
};
	*|;

	main := |* 
		'{' => {print( "{ " );
fcall one;};
		"[" => {print( "[ " );
fcall two;};
		[a-z]+ => {print( "word " );
val = 3;
fgoto handle;};
		[a-z] ' foil' => {print( "this is the foil" );
};
		' ' => {print( "space " );
};
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
comm = 0;
top = 0;
stack = Int [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
ts = 0;
te = 0;
act = 0;
val = 0;

	%% write init;
	%% write exec;

	if ( cs >= GotoCallRet_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "{a{b[c d]d}c}\n" );
	m( "[a{b[c d]d}c}\n" );
	m( "[a[b]c]d{ef{g{h}i}j}l\n" );
	m( "{{[]}}\n" );
	m( "a b c\n" );
	m( "{a b c}\n" );
	m( "[a b c]\n" );
	m( "{]\n" );
	m( "{{}\n" );
	m( "[[[[[[]]]]]]\n" );
	m( "[[[[[[]]}]]]\n" );
