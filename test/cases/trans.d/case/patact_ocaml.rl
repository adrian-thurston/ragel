(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let comm = ref 0
let top = ref 0
let stack = Array.make 32 0
let ts = ref 0
let te = ref 0
let act = ref 0
let value = ref 0

%%{
	machine patact;

	other := |* 
		[a-z]+ => {print_string( "word\n" );
};
		[0-9]+ => {print_string( "num\n" );
};
		[\n ] => {print_string( "space\n" );
};
	*|;

	exec_test := |* 
		[a-z]+ => {print_string( "word (w/lbh)\n" );
fexec te-1; fgoto other; };
		[a-z]+ ' foil' => {print_string( "word (c/lbh)\n" );
};
		[\n ] => {print_string( "space\n" );
};
		'22' => {print_string( "num (w/switch)\n" );
};
		[0-9]+ => {print_string( "num (w/switch)\n" );
fexec te-1; fgoto other;};
		[0-9]+ ' foil' => {print_string( "num (c/switch)\n" );
};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {print_string( "in semi\n" );
fgoto main; };
	*|;

	main := |* 
		[a-z]+ => {print_string( "word (w/lbh)\n" );
fhold; fgoto other; };
		[a-z]+ ' foil' => {print_string( "word (c/lbh)\n" );
};
		[\n ] => {print_string( "space\n" );
};
		'22' => {print_string( "num (w/switch)\n" );
};
		[0-9]+ => {print_string( "num (w/switch)\n" );
fhold; fgoto other;};
		[0-9]+ ' foil' => {print_string( "num (c/switch)\n" );
};
		';' => {print_string( "going to semi\n" );
fhold; fgoto semi;};
		'!' => {print_string( "immdiate\n" );
fgoto exec_test; };
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
	if !cs >= patact_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "abcd foix\n";
	exec "abcd\nanother\n";
	exec "123 foix\n";
	exec "!abcd foix\n";
	exec "!abcd\nanother\n";
	exec "!123 foix\n";
	exec ";";
  ()
;;

