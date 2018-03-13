(*
 * @LANG: ocaml
 * @GENERATED: true
 *)




%%{
	machine erract;

	action err_start {print_string( "err_start\n" );
}
	action err_all {print_string( "err_all\n" );
}
	action err_middle {print_string( "err_middle\n" );
}
	action err_out {print_string( "err_out\n" );
}

	action eof_start {print_string( "eof_start\n" );
}
	action eof_all {print_string( "eof_all\n" );
}
	action eof_middle {print_string( "eof_middle\n" );
}
	action eof_out {print_string( "eof_out\n" );
}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
		) '\n';
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
	if !cs >= erract_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "";
	exec "h";
	exec "x";
	exec "he";
	exec "hx";
	exec "hel";
	exec "hex";
	exec "hell";
	exec "helx";
	exec "hello";
	exec "hellx";
	exec "hello\n";
	exec "hellox";
  ()
;;

