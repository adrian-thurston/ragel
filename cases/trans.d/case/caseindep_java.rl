/*
 * @LANG: java
 * @GENERATED: true
 */


class caseindep_java
{


%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
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

	String _s;
	%% write exec;
}

void finish( )
{
	if ( cs >= indep_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"1hello\n",
"1HELLO\n",
"1HeLLo\n",
"2hello\n",
"2HELLO\n",
"2HeLLo\n",
"3hello\n",
"3HELLO\n",
"3HeLLo\n",
"4hello\n",
"4HELLO\n",
"4HeLLo\n",
"5hello\n",
"5HELLO\n",
"5HeLLo\n",
"6hello\n",
"6HELLO\n",
"6HeLLo\n",
};

static final int inplen = 18;

public static void main (String[] args)
{
	caseindep_java machine = new caseindep_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
