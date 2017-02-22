(*
 * @LANG: ocaml
 * @GENERATED: true
 *)




%%{
	machine state_act;

	action a1 {print_string( "a1\n" );
}
	action a2 {print_string( "a2\n" );
}
	action b1 {print_string( "b1\n" );
}
	action b2 {print_string( "b2\n" );
}
	action c1 {print_string( "c1\n" );
}
	action c2 {print_string( "c2\n" );
}
	action next_again {fnext again;}

	hi = 'hi';
	line = again: 
			hi 
				>to b1 
				>from b2 
			'\n' 
				>to c1 
				>from c2 
				@next_again;
		 
	main := line*
			>to a1 
			>from a2;
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
	if !cs >= state_act_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "hi\nhi\n";
  ()
;;

