(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let neg = ref 0
let value = ref 0

%%{
	machine state_chart;

	action begin {neg := 0;
value := 0;
}

	action see_neg {neg := 1;
}

	action add_digit {value := value .contents * 10  + ( fc - 48 )
;
}

	action finish {if neg .contents != 0  then
begin
	value := -1  * value.contents;

end 
;
}

	atoi = (
		start: (
			'-' @see_neg ->om_num | 
			'+' ->om_num |
			[0-9] @add_digit ->more_nums
		),

		# One or more nums.
		om_num: (
			[0-9] @add_digit ->more_nums
		),

		# Zero ore more nums.
		more_nums: (
			[0-9] @add_digit ->more_nums |
			'' -> final
		)
	) >begin %finish;

	action oneof {print_int( value.contents );
print_string( "\n" );
}
	main := ( atoi '\n' @oneof )*;
}%%




%% write data;

let exec data = 
  let buffer = String.create(1024) in 
  let blen :int ref = ref 0 in
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
value := 0;
neg := 0;
	%% write init;
	%% write exec;
	if !cs >= state_chart_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "1\n";
	exec "12\n";
	exec "222222\n";
	exec "+2123\n";
	exec "213 3213\n";
	exec "-12321\n";
	exec "--123\n";
	exec "-99\n";
	exec " -3000\n";
  ()
;;

