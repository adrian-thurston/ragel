//
// @LANG: julia
// @GENERATED: true
//




%%{
	machine erract;

	action err_start {print( "err_start\n" );
}
	action err_all {print( "err_all\n" );
}
	action err_middle {print( "err_middle\n" );
}
	action err_out {print( "err_out\n" );
}

	action eof_start {print( "eof_start\n" );
}
	action eof_all {print( "eof_all\n" );
}
	action eof_middle {print( "eof_middle\n" );
}
	action eof_out {print( "eof_out\n" );
}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
}%%



%% write data;

function m( data::AbstractString )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0
	buffer = ""

	%% write init;
	%% write exec;

	if ( cs >= erract_first_final )
		println( "ACCEPT" );
	else
		println( "FAIL" );
	end
end

	m( "" );
	m( "h" );
	m( "x" );
	m( "he" );
	m( "hx" );
	m( "hel" );
	m( "hex" );
	m( "hell" );
	m( "helx" );
	m( "hello" );
	m( "hellx" );
	m( "hello\n" );
	m( "hellox" );
