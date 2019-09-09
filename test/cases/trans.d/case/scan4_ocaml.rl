(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let ts = ref 0
let te = ref 0
let act = ref 0
let token = ref 0

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
	  'a' => {print_string( "pat1\n" );
};

	  [ab]+ . 'c' => {print_string( "pat2\n" );
};

	  any;
	*|;
}%%




%% write data;

let exec data = 
  let buffer = String.create(1024) in 
  let blen :int ref = ref 0 in
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
	let eof = pe in
	%% write init;
	%% write exec;
	if !cs >= scanner_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "ba a";
  ()
;;

