/*
 * @LANG: java
 * @GENERATED: true
 */


class clang4_java
{
int pos ;
int line ;

%%{
	machine clang;

	# Function to buffer a character.
	action bufChar {	buffer[blen] = fc;
	blen += 1;
}

	# Function to clear the buffer.
	action clearBuf {	blen = 0;
}

	action incLine {line = line + 1;
}

	# Functions to dump tokens as they are matched.
	action ident {System.out.print( "ident(" );
System.out.print( line );
System.out.print( "," );
System.out.print( blen );
System.out.print( "): " );
System.out.print( new String( buffer, 0, blen ) );
System.out.print( "\n" );
}
	action literal {System.out.print( "literal(" );
System.out.print( line );
System.out.print( "," );
System.out.print( blen );
System.out.print( "): " );
System.out.print( new String( buffer, 0, blen ) );
System.out.print( "\n" );
}
	action float {System.out.print( "float(" );
System.out.print( line );
System.out.print( "," );
System.out.print( blen );
System.out.print( "): " );
System.out.print( new String( buffer, 0, blen ) );
System.out.print( "\n" );
}
	action integer {System.out.print( "int(" );
System.out.print( line );
System.out.print( "," );
System.out.print( blen );
System.out.print( "): " );
System.out.print( new String( buffer, 0, blen ) );
System.out.print( "\n" );
}
	action hex {System.out.print( "hex(" );
System.out.print( line );
System.out.print( "," );
System.out.print( blen );
System.out.print( "): " );
System.out.print( new String( buffer, 0, blen ) );
System.out.print( "\n" );
}
	action symbol {System.out.print( "symbol(" );
System.out.print( line );
System.out.print( "," );
System.out.print( blen );
System.out.print( "): " );
System.out.print( new String( buffer, 0, blen ) );
System.out.print( "\n" );
}

	# Alpha numberic characters or underscore.
	alnumu = alnum | '_';

	# Alpha charactres or underscore.
	alphau = alpha | '_';

	# Symbols. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving dump the symbol.
	symbol = ( punct - [_'"] ) >clearBuf $bufChar %symbol;

	# Identifier. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving, dump the identifier.
	ident = (alphau . alnumu*) >clearBuf $bufChar %ident;

	# Match single characters inside literal strings. Or match 
	# an escape sequence. Buffers the charater matched.
	sliteralChar =
			( extend - ['\\] ) @bufChar |
			( '\\' . extend @bufChar );
	dliteralChar =
			( extend - ["\\] ) @bufChar |
			( '\\' . extend @bufChar );

	# Single quote and double quota literals. At the start clear
	# the buffer. Upon leaving dump the literal.
	sliteral = ('\'' @clearBuf . sliteralChar* . '\'' ) %literal;
	dliteral = ('"' @clearBuf . dliteralChar* . '"' ) %literal;
	literal = sliteral | dliteral;

	# Whitespace is standard ws, newlines and control codes.
	whitespace = any - 33 .. 126;

	# Describe both c style comments and c++ style comments. The
	# priority bump on tne terminator of the comments brings us
	# out of the extend* which matches everything.
	ccComment = '//' . extend* $0 . '\n' @1;
	cComment = '/!' . extend* $0 . '!/' @1;

	# Match an integer. We don't bother clearing the buf or filling it.
	# The float machine overlaps with int and it will do it.
	integer = digit+ %integer;

	# Match a float. Upon entering the machine clear the buf, buffer
	# characters on every trans and dump the float upon leaving.
	float =  ( digit+ . '.' . digit+ ) >clearBuf $bufChar %float;

	# Match a hex. Upon entering the hex part, clear the buf, buffer characters
	# on every trans and dump the hex on leaving transitions.
	hex = '0x' . xdigit+ >clearBuf $bufChar %hex;

	# Or together all the lanuage elements.
	fin = ( ccComment |
		cComment |
		symbol |
		ident |
		literal |
		whitespace |
		integer |
		float |
		hex );

	# Star the language elements. It is critical in this type of application
	# that we decrease the priority of out transitions before doing so. This
	# is so that when we see 'aa' we stay in the fin machine to match an ident
	# of length two and not wrap around to the front to match two idents of 
	# length one.
	clang_main = ( fin $1 %0 )*;

	# This machine matches everything, taking note of newlines.
	newline = ( any | '\n' @incLine )*;

	# The final fsm is the lexer intersected with the newline machine which
	# will count lines for us. Since the newline machine accepts everything,
	# the strings accepted is goverened by the clang_main machine, onto which
	# the newline machine overlays line counting.
	main := clang_main & newline;
}%%



%% write data;
int cs;

void init()
{
pos = 0;
line = 1;
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
	if ( cs >= clang_first_final )
		System.out.println( "ACCEPT" );
	else
		System.out.println( "FAIL" );
}

static final String inp[] = {
"999 0xaAFF99 99.99 /!\n!/ 'lksdj' //\n\"\n\nliteral\n\n\n\"0x00aba foobardd.ddsf 0x0.9\n",
"wordwithnum00asdf\n000wordfollowsnum,makes new symbol\n\nfinishing early /! unfinished ...\n",
};

static final int inplen = 2;

public static void main (String[] args)
{
	clang4_java machine = new clang4_java();
	for ( int i = 0; i < inplen; i++ ) {
		machine.init();
		machine.exec( inp[i].toCharArray(), inp[i].length() );
		machine.finish();
	}
}
}
