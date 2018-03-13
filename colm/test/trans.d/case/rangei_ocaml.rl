(*
 * @LANG: ocaml
 * @GENERATED: true
 *)



%%{
	machine rangei;

	main := 
		'a' ../i 'z' .
		'A' ../i 'Z' .
		60 ../i 93 .
		94 ../i 125 .
		86 ../i 101 .
		60 ../i 125
		'';
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
	if !cs >= rangei_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "AaBbAa";
	exec "Aa`bAa";
	exec "AaB@Aa";
	exec "AaBbMa";
	exec "AaBbma";
  ()
;;

