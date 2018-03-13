(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let i = ref 0
let c = ref 0

%%{
	machine foo;

	action testi {i .contents > 0}
	action inc {i := i .contents - 1;
c := ( fc )
;
print_string( "item: " );
print_int( c.contents );
print_string( "\n" );
}

	count = [0-9] @{i := ( fc - 48 )
;
print_string( "count: " );
print_int( i.contents );
print_string( "\n" );
};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
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
	exec "00\n";
	exec "019\n";
	exec "190\n";
	exec "1719\n";
	exec "1040000\n";
	exec "104000a\n";
	exec "104000\n";
  ()
;;

