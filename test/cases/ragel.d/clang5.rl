(*
 * @LANG: ocaml
 *)

let id x = x
let fail fmt = Printf.ksprintf failwith fmt
let pr fmt = Printf.ksprintf print_endline fmt

let failed fmt = Printf.ksprintf (fun s -> prerr_endline s; exit 1) fmt
let test' show f x y = if f x <> y then failed "FAILED: test %S" (show x)
let case = ref 0
let test f x y = incr case; if f x <> y then failed "FAILED: case %d" !case
let error f x = match try Some (f x) with _ -> None with Some _ -> failed "FAILED: fail %S" x | None -> ()


%%{
	machine clang;

	newline = '\n' @{ incr curlin };
	any_count_line = any | newline;

	# Consume a C comment.
	c_comment := any_count_line* :>> '*/' @{fgoto main;};

	main := |*

	# Alpha numberic characters or underscore.
	alnum_u = alnum | '_';

	# Alpha charactres or underscore.
	alpha_u = alpha | '_';

	# Symbols. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving dump the symbol.
	( punct - [_'"] ) {
		pr "symbol(%i): %c" !curlin data.[!ts];
	};

	# Identifier. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving, dump the identifier.
	alpha_u alnum_u* {
		pr "ident(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Single Quote.
	sliteralChar = [^'\\] | newline | ( '\\' . any_count_line );
	'\'' . sliteralChar* . '\'' {
		pr "single_lit(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Double Quote.
	dliteralChar = [^"\\] | newline | ( '\\' any_count_line );
	'"' . dliteralChar* . '"' {
		pr "double_lit(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Whitespace is standard ws, newlines and control codes.
	any_count_line - 0x21..0x7e;

	# Describe both c style comments and c++ style comments. The
	# priority bump on tne terminator of the comments brings us
	# out of the extend* which matches everything.
	'//' [^\n]* newline;

	'/*' { fgoto c_comment; };

	# Match an integer. We don't bother clearing the buf or filling it.
	# The float machine overlaps with int and it will do it.
	digit+ {
		pr "int(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Match a float. Upon entering the machine clear the buf, buffer
	# characters on every trans and dump the float upon leaving.
	digit+ '.' digit+ {
		pr "float(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Match a hex. Upon entering the hex part, clear the buf, buffer characters
	# on every trans and dump the hex on leaving transitions.
	'0x' xdigit+ {
		pr "hex(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	*|;
}%%

%% write data;

let clang data =
	let cs = ref 0 in
	let p = ref 0 in
	let pe = ref (String.length data) in
	let ts = ref 0 in
	let te = ref 0 in
	let eof = pe in 
	let curlin = ref 1 in

	%%{
		write init;
		write exec;
	}%%

	if !cs < clang_first_final then
		fail "atoi: cs %d < %d" !cs clang_first_final;
;;

let () =
	clang "999 0xaAFF99 99.99 /!\n!/ 'lksdj' //\n\"\n\nliteral\n\n\n\"0x00aba foobardd.ddsf 0x0.9\n";
	clang "wordwithnum00asdf\n000wordfollowsnum,makes new symbol\n\nfinishing early /! unfinished ...\n";
  ()
;;


##### OUTPUT #####
int(1): 999
hex(1): 0xaAFF99
float(1): 99.99
symbol(1): /
symbol(1): !
symbol(2): !
symbol(2): /
single_lit(2): 'lksdj'
double_lit(8): "

literal


"
hex(8): 0x00aba
ident(8): foobardd
symbol(8): .
ident(8): ddsf
hex(8): 0x0
symbol(8): .
int(8): 9
ident(1): wordwithnum00asdf
int(2): 000
ident(2): wordfollowsnum
symbol(2): ,
ident(2): makes
ident(2): new
ident(2): symbol
ident(4): finishing
ident(4): early
symbol(4): /
symbol(4): !
ident(4): unfinished
symbol(4): .
symbol(4): .
symbol(4): .
