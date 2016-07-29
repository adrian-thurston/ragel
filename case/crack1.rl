// @LANG: crack

import crack.io cout;

%%{
	machine atoi;

	main := '-'? [0-9]+ '.' @{
		cout `match\n`;
	};
}%%

%% write data;

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;

	%% write init;
	%% write exec;
}

void main()
{
	m( "-99." );
	m( "100." );
	m( "100x." );
	m( "1000." );
}

main();

##### OUTPUT #####
match
match
match
