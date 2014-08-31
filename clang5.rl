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
		T.pr "symbol(%i): %c" !curlin data.[!ts];
	};

	# Identifier. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving, dump the identifier.
	alpha_u alnum_u* {
		T.pr "ident(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Single Quote.
	sliteralChar = [^'\\] | newline | ( '\\' . any_count_line );
	'\'' . sliteralChar* . '\'' {
		T.pr "single_lit(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Double Quote.
	dliteralChar = [^"\\] | newline | ( '\\' any_count_line );
	'"' . dliteralChar* . '"' {
		T.pr "double_lit(%i): %s" !curlin (String.sub data !ts (!te - !ts));
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
		T.pr "int(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Match a float. Upon entering the machine clear the buf, buffer
	# characters on every trans and dump the float upon leaving.
	digit+ '.' digit+ {
		T.pr "float(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	# Match a hex. Upon entering the hex part, clear the buf, buffer characters
	# on every trans and dump the hex on leaving transitions.
	'0x' xdigit+ {
		T.pr "hex(%i): %s" !curlin (String.sub data !ts (!te - !ts));
	};

	*|;
}%%

%% write data nofinal;

let fail fmt = Printf.ksprintf failwith fmt

let () =
	let data = String.create 2048 in
	let cs = ref 0 and act = ref 0 and have = ref 0 and curlin = ref 1 in
	let ts = ref 0 and te = ref 0 in
	let fin = ref false in

	%% write init;

	while not !fin do
		let p = ref !have and eof = ref (-1) in
		let space = String.length data - !have in

		if space = 0 then
			(* We've used up the entire buffer storing an already-parsed token
			 * prefix that must be preserved. *)
			fail "OUT OF BUFFER SPACE";

    let len = Unix.read Unix.stdin data !p space in
		let pe = ref (!p + len) in

		(* Check if this is the end of file. *)
		if len < space then (eof := !pe; fin := true);

		%% write exec;

		if !cs = clang_error then
			fail "PARSE ERROR";

		if !ts = -1 then
			have := 0
		else
    begin
			(* There is a prefix to preserve, shift it over. *)
			have := !pe - !ts;
			String.blit data !ts data 0 !have;
			te := !te - !ts;
			ts := 0;
		end
	done

(* _____OUTPUT_____
7
666
-666
666
123456789
123456789
0
_____OUTPUT_____ *)
