(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let i = ref 0
let j = ref 0
let k = ref 0

%%{
	machine foo;

	action c1 {i .contents != 0}
	action c2 {j .contents != 0}
	action c3 {k .contents != 0}
	action one {print_string( "  one\n" );
}
	action two {print_string( "  two\n" );
}
	action three {print_string( "  three\n" );
}

	action seti {if fc == 48 then
begin
	i := 0;

end 
else
begin
	i := 1;

end
;
}
	action setj {if fc == 48 then
begin
	j := 0;

end 
else
begin
	j := 1;

end
;
}
	action setk {if fc == 48 then
begin
	k := 0;

end 
else
begin
	k := 1;

end
;
}

	action break {fnbreak;}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
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
	if !cs >= foo_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "000abc\n";
	exec "100abc\n";
	exec "010abc\n";
	exec "110abc\n";
	exec "001abc\n";
	exec "101abc\n";
	exec "011abc\n";
	exec "111abc\n";
  ()
;;

