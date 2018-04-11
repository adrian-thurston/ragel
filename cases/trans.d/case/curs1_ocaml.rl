(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let return_to = ref 0

%%{
	machine curs1;

	unused := 'unused';

	one := 'one' @{print_string( "one\n" );
fnext *return_to.contents;
};

	two := 'two' @{print_string( "two\n" );
fnext *return_to.contents;
};

	main := 
		'1' @{return_to := fcurs;
fnext one; }
	|	'2' @{return_to := fcurs;
fnext two; }
	|	'\n';
}%%




%% write data;

let exec data = 
  let buffer = String.create(1024) in 
  let blen :int ref = ref 0 in
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
	%% write init;
	%% write exec;
	if !cs >= curs1_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "1one2two1one\n";
  ()
;;

