/*
 * @LANG: java
 * @GENERATED: true
 */


class gotocallret2_java
{
char comm ;
int top ;
int stack [] = new int[32];
int
 ts ;
int
 te ;
int act ;
int val ;

%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{System.out.print( "handle " );
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
		'{' => {System.out.print( "{ " );
fcall *fentry(one);};
		"[" => {System.out.print( "[ " );
fcall *fentry(two);};
		"}" sp* => {System.out.print( "} " );
fret;};
		[a-z]+ => {System.out.print( "word " );
val = 1;
fgoto *fentry(handle);};
		' ' => {System.out.print( "space " );
};
	*|;

	two := |*
		'{' => {System.out.print( "{ " );
fcall *fentry(one);};
		"[" => {System.out.print( "[ " );
fcall *fentry(two);};
		']' sp* => {System.out.print( "] " );
fret;};
		[a-z]+ => {System.out.print( "word " );
val = 2;
fgoto *fentry(handle);};
		' ' => {System.out.print( "space " );
};
	*|;

	main := |* 
		'{' => {System.out.print( "{ " );
fcall one;};
		"[" => {System.out.print( "[ " );
fcall two;};
		[a-z]+ => {System.out.print( "word " );
val = 3;
fgoto handle;};
		[a-z] ' foil' => {System.out.print( "this is the foil" );
};
		' ' => {System.out.print( "space " );
};
		'\n';
	*|;
}%%



%% write data;
int cs;

void init()
{
	%% write init;
}

void exec( char data[], int len )
{
	char buffer [] = new char[1024];
	int blen = 0;
	int p = 0;
	int pe = len;

	int eof = len;
	String _s;
	%% write exec;
}

void finish( )
{
	if ( cs >= GotoCallRet_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"{a{b[c d]d}c}\n",
"[a{b[c d]d}c}\n",
"[a[b]c]d{ef{g{h}i}j}l\n",
"{{[]}}\n",
"a b c\n",
"{a b c}\n",
"[a b c]\n",
"{]\n",
"{{}\n",
"[[[[[[]]]]]]\n",
"[[[[[[]]}]]]\n",
};

static final int inplen = 11;

public static void main (String[] args)
{
	gotocallret2_java machine = new gotocallret2_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
