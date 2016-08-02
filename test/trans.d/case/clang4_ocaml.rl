(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let pos = ref 0
let line = ref 0

%%{
	machine clang;

	# Function to buffer a character.
	action bufChar {	begin
	String.set buffer blen.contents data.[p.contents];
	blen := blen.contents + 1;
	end
}

	# Function to clear the buffer.
	action clearBuf {begin
	blen := 0;
end
}

	action incLine {line := line .contents + 1;
}

	# Functions to dump tokens as they are matched.
	action ident {print_string( "ident(" );
print_int( line.contents );
print_string( "," );
print_int( blen.contents );
print_string( "): " );
print_string( String.sub buffer 0 blen.contents );
print_string( "\n" );
}
	action literal {print_string( "literal(" );
print_int( line.contents );
print_string( "," );
print_int( blen.contents );
print_string( "): " );
print_string( String.sub buffer 0 blen.contents );
print_string( "\n" );
}
	action float {print_string( "float(" );
print_int( line.contents );
print_string( "," );
print_int( blen.contents );
print_string( "): " );
print_string( String.sub buffer 0 blen.contents );
print_string( "\n" );
}
	action integer {print_string( "int(" );
print_int( line.contents );
print_string( "," );
print_int( blen.contents );
print_string( "): " );
print_string( String.sub buffer 0 blen.contents );
print_string( "\n" );
}
	action hex {print_string( "hex(" );
print_int( line.contents );
print_string( "," );
print_int( blen.contents );
print_string( "): " );
print_string( String.sub buffer 0 blen.contents );
print_string( "\n" );
}
	action symbol {print_string( "symbol(" );
print_int( line.contents );
print_string( "," );
print_int( blen.contents );
print_string( "): " );
print_string( String.sub buffer 0 blen.contents );
print_string( "\n" );
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

let exec data = 
  let buffer = String.create(1024) in 
  let blen :int ref = ref 0 in
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
	let eof = pe in
pos := 0;
line := 1;
	%% write init;
	%% write exec;
	if !cs >= clang_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "999 0xaAFF99 99.99 /!\n!/ 'lksdj' //\n\"\n\nliteral\n\n\n\"0x00aba foobardd.ddsf 0x0.9\n";
	exec "wordwithnum00asdf\n000wordfollowsnum,makes new symbol\n\nfinishing early /! unfinished ...\n";
  ()
;;

