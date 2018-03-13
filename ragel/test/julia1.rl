// @LANG: julia 

%%{
	machine atoi;

	main := '-'? [0-9]+ '.' @{
		println( "match" );
	};
}%%

%% write data;

function test( data::String )
	p = 0
	pe = length(data)
	eof = length(data)
	cs = 0

	%% write init;

	%% write exec;
end


test( "-99." )
test( "100." )
test( "100x." )
test( "1000." )

######## OUTPUT #######
match
match
match
