(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let comm = ref 0
let top = ref 0
let stack = Array.make 32 0

%%{
	machine gotocallret;

	# A reference to a state in an unused action caused a segfault in 5.8. */
	action unusedAction {fentry( garble_line );
}

	action err_garbling_line {print_string( "error: garbling line\n" );
}
	action goto_main {fnext main; }
	action recovery_failed {print_string( "error: failed to recover\n" );
}

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := ( (any-'\n')*'\n') 
		>err_garbling_line
		@goto_main
		$/recovery_failed;

	action hold_and_return {fhold; fnret;}

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!hold_and_return;
	dig_comm := digit+ $!hold_and_return;

	# Choose which to machine to call into based on the command.
	action comm_arg {if comm .contents >= 97  then
begin
	fncall alp_comm;
		
end 
else
begin
	fncall dig_comm;
		
end
;
}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{comm := fc;
} ' ' @comm_arg @{print_string( "prints\n" );
} '\n'
	) @{print_string( "correct command\n" );
};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;fnext garble_line;}; 
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
	if !cs >= gotocallret_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "lkajsdf\n";
	exec "2134\n";
	exec "(\n";
	exec "\n";
	exec "*234234()0909 092 -234aslkf09`1 11\n";
	exec "1\n";
	exec "909\n";
	exec "1 a\n";
	exec "11 1\n";
	exec "a 1\n";
	exec "aa a\n";
	exec "1 1\n";
	exec "1 123456\n";
	exec "a a\n";
	exec "a abcdef\n";
	exec "h";
	exec "a aa1";
  ()
;;

