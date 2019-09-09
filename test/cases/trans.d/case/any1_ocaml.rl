(*
 * @LANG: ocaml
 * @GENERATED: true
 *)




%%{
	machine any1;
	main := any;
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
	if !cs >= any1_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "";
	exec "x";
	exec "xx";
  ()
;;

