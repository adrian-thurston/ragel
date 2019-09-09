(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let target = ref 0

%%{
	machine next1;

	unused := 'unused';

	one := 'one' @{print_string( "one\n" );
target := fentry( main );
fnext *target.contents;
};

	two := 'two' @{print_string( "two\n" );
target := fentry( main );
fnext *target.contents;
};

	main := 
		'1' @{target := fentry( one );
fnext *target.contents;
}
	|	'2' @{target := fentry( two );
fnext *target.contents;
}
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
	if !cs >= next1_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "1one2two1one\n";
  ()
;;

