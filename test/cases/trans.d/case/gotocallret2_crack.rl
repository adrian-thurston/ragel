//
// @LANG: crack
// @GENERATED: true
//

import crack.io cout;
import crack.lang Buffer;

byte comm;
int top;
array[int] stack = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ];
int
 ts;
int
 te;
int act;
int val;

%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{cout.format( "handle " );
fhold;if ( val == 1 )
{
	fnext *fentry(one);
} 
if ( val == 2 )
{
	fnext *fentry(two);
} 
if ( val == 3 )
{
	fnext main;
} 
};

	one := |*
		'{' => {cout.format( "{ " );
fcall *fentry(one);};
		"[" => {cout.format( "[ " );
fcall *fentry(two);};
		"}" sp* => {cout.format( "} " );
fret;};
		[a-z]+ => {cout.format( "word " );
val = 1;
fgoto *fentry(handle);};
		' ' => {cout.format( "space " );
};
	*|;

	two := |*
		'{' => {cout.format( "{ " );
fcall *fentry(one);};
		"[" => {cout.format( "[ " );
fcall *fentry(two);};
		']' sp* => {cout.format( "] " );
fret;};
		[a-z]+ => {cout.format( "word " );
val = 2;
fgoto *fentry(handle);};
		' ' => {cout.format( "space " );
};
	*|;

	main := |* 
		'{' => {cout.format( "{ " );
fcall one;};
		"[" => {cout.format( "[ " );
fcall two;};
		[a-z]+ => {cout.format( "word " );
val = 3;
fgoto handle;};
		[a-z] ' foil' => {cout.format( "this is the foil" );
};
		' ' => {cout.format( "space " );
};
		'\n';
	*|;
}%%



%% write data;

void m( String s )
{
	byteptr data = s.buffer;
	int p = 0;
	int pe = s.size;
	int cs;
	String buffer;
	int eof = pe;

	%% write init;
	%% write exec;

	if ( cs >= GotoCallRet_first_final ) {
		cout `ACCEPT\n`;
	}
	else {
		cout `FAIL\n`;
	}
}

void main()
{
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
}

main();
