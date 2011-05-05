(*
 * Demonstrate the use of goto, call and return. This machine expects either a
 * lower case char or a digit as a command then a space followed by the command
 * arg. If the command is a char, then the arg must be an a string of chars.
 * If the command is a digit, then the arg must be a string of digits. This
 * choice is determined by action code, rather than though transition
 * desitinations.
 *)

%%{
	machine GotoCallRet;
  access ;

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := (
		(any-'\n')*'\n'
	) >{ got `Garble; fail <- fail + 1 } @{fgoto main;};

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!{fhold;fret;};
	dig_comm := digit+ $!{fhold;fret;};

	# Choose which to machine to call into based on the command.
	action comm_arg {
		if Char.code comm >= Char.code 'a' then
			fcall alp_comm;
		else
			fcall dig_comm;
	}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{comm <- fc;} ' ' @comm_arg '\n'
	) @{ got `Correct; ok <- ok + 1 };

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{fhold;fgoto garble_line;};
}%%

%% write data;

let gotoCallRet () = object

val mutable comm = 'a'
val mutable cs = 0
(* val mutable cs = 0 *)
val mutable top = 0
val stack = Array.make 32 0
val mutable ok = 0
val mutable fail = 0

initializer
(*   T.pr "init"; *)
  %% write init;
  ()

method execute data isEof =
	let p = ref 0 in
  let pe = ref (String.length data) in
  let eof = ref (if isEof then !pe else (-1)) in
  let got = ignore in

	%% write exec;

	if cs = gotoCallRet_error then
		T.fail "error: parsing";

  if cs < gotoCallRet_first_final && isEof then
		T.fail "error: not recognized"

method status = ok, fail

end

let top () =
	let gcr = gotoCallRet () in
  begin try
	while true do
		gcr#execute (input_line stdin ^ "\n") false
  done with End_of_file -> () end;
	gcr#execute "" true

let test l =
  let gcr = gotoCallRet () in
  List.iter (fun s -> gcr#execute s false) l;
  gcr#execute "" true;
  gcr#status

let () =
  T.test test ["d";" dsds";"ds\n\n3 32\nd";" \n";"q qqq\n3 333\nd \n44\n"] (6,2)

