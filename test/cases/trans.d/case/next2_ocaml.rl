(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let target = ref 0
let last = ref 0

%%{
	machine next2;

	unused := 'unused';

	one := 'one' @{print_string( "one\n" );
target := fentry( main );
fnext *target.contents;
};

	two := 'two' @{print_string( "two\n" );
target := fentry( main );
fnext *target.contents;
};

	three := 'three' @{print_string( "three\n" );
target := fentry( main );
fnext *target.contents;
};

	main :=  (
		'1' @{target := fentry( one );
fnext *target.contents;
last := 1;
} |

		'2' @{target := fentry( two );
fnext *target.contents;
last := 2;
} |

		# This one is conditional based on the last.
		'3' @{if last .contents == 2 then
begin
	target := fentry( three );
fnext *target.contents;

end 
;
last := 3;
} 'x' |

		'\n'
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
	if !cs >= next2_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "1one3x2two3three\n";
  ()
;;

