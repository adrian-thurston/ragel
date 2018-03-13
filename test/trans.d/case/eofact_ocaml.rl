(*
 * @LANG: ocaml
 * @GENERATED: true
 *)




%%{
	machine eofact;

	action a1 {print_string( "a1\n" );
}
	action a2 {print_string( "a2\n" );
}
	action a3 {print_string( "a3\n" );
}
	action a4 {print_string( "a4\n" );
}


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

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
	if !cs >= eofact_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "";
	exec "h";
	exec "hell";
	exec "hello";
	exec "hello\n";
	exec "t";
	exec "ther";
	exec "there";
	exec "friend";
  ()
;;

