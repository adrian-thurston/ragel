(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let top = ref 0
let stack = Array.make 32 0
let target = ref 0

%%{
	machine ncall1;

	unused := 'unused';

	one := 'one' @{print_string( "one\n" );
fnret;
	};

	two := 'two' @{print_string( "two\n" );
fnret;
	};

	main := (
			'1' @{target := fentry( one );
fncall *target.contents;
}
		|	'2' @{target := fentry( two );
fncall *target.contents;
}
		|	'\n'
	)*;
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
	if !cs >= ncall1_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "1one2two1one\n";
  ()
;;

