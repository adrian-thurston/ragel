(*
 * @LANG: ocaml
 * @GENERATED: true
 *)



%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
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
	if !cs >= indep_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "1hello\n";
	exec "1HELLO\n";
	exec "1HeLLo\n";
	exec "2hello\n";
	exec "2HELLO\n";
	exec "2HeLLo\n";
	exec "3hello\n";
	exec "3HELLO\n";
	exec "3HeLLo\n";
	exec "4hello\n";
	exec "4HELLO\n";
	exec "4HeLLo\n";
	exec "5hello\n";
	exec "5HELLO\n";
	exec "5HeLLo\n";
	exec "6hello\n";
	exec "6HELLO\n";
	exec "6HeLLo\n";
  ()
;;

