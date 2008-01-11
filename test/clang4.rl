/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 */

char array[32];
int pos;
int line;
%%
pos = 0;
line = 1;
%%{
	machine clang;

	# Function to buffer a character.
	action bufChar { array[pos] = fc; pos = pos + 1; }

	# Function to clear the buffer.
	action clearBuf { pos = 0; }

	# Functions to dump tokens as they are matched.
	action ident {
		prints "ident(";
		printi line;
		prints ",";
		printi pos;
		prints "): ";
		printb array;
		prints "\n";
	}
	action literal {
		prints "literal(";
		printi line;
		prints ",";
		printi pos;
		prints "): ";
		printb array;
		prints "\n";
	}
	action float {
		prints "float(";
		printi line;
		prints ",";
		printi pos;
		prints "): ";
		printb array;
		prints "\n";
	}
	action integer {
		prints "int(";
		printi line;
		prints ",";
		printi pos;
		prints "): ";
		printb array;
		prints "\n";
	}
	action hex {
		prints "hex(";
		printi line;
		prints ",";
		printi pos;
		prints "): ";
		printb array;
		prints "\n";
	}
	action symbol {
		prints "symbol(";
		printi line;
		prints ",";
		printi pos;
		prints "): ";
		printb array;
		prints "\n";
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
	newline = ( any | '\n' @{ line = line + 1; } )*;

	# The final fsm is the lexer intersected with the newline machine which
	# will count lines for us. Since the newline machine accepts everything,
	# the strings accepted is goverened by the clang_main machine, onto which
	# the newline machine overlays line counting.
	main := clang_main & newline;
}%%
/* _____INPUT_____
"999 0xaAFF99 99.99 /!\n!/ 'lksdj' //\n\"\n\nliteral\n\n\n\"0x00aba foobardd.ddsf 0x0.9\n"
"wordwithnum00asdf\n000wordfollowsnum,makes new symbol\n\nfinishing early /! unfinished ...\n"
_____INPUT_____ */
/* _____OUTPUT_____
int(1,3): 999
hex(1,6): aAFF99
float(1,5): 99.99
literal(2,5): lksdj
literal(8,12): 

literal



hex(8,5): 00aba
ident(8,8): foobardd
symbol(8,1): .
ident(8,4): ddsf
hex(8,1): 0
symbol(8,1): .
int(8,1): 9
ACCEPT
ident(1,17): wordwithnum00asdf
int(2,3): 000
ident(2,14): wordfollowsnum
symbol(2,1): ,
ident(2,5): makes
ident(2,3): new
ident(2,6): symbol
ident(4,9): finishing
ident(4,5): early
FAIL
_____OUTPUT_____ */

