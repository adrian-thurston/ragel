%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => { 
			prints "on last     ";
			if !p+1 = !te then
				(on_last := !cnt; incr cnt; prints "yes");
			prints "\n";
		};

		'b'+ => {
			prints "on next     ";
			if !p+1 = !te then
				( on_next := !cnt; incr cnt; prints "yes");
			prints "\n";
		};

		'c1' 'dxxx'? => {
			prints "on lag      ";
			if !p+1 = !te then
				( on_lag := !cnt; incr cnt; prints "yes"); 
			prints "\n";
		};

		'd1' => {
			prints "lm switch1  ";
			if !p+1 = !te then
				(sw1 := !cnt; incr cnt; prints "yes");
			prints "\n";
		};
		'd2' => {
			prints "lm switch2  ";
			if !p+1 = !te then
        (sw2 := !cnt; incr cnt; prints "yes");
			prints "\n";
		};

		[d0-9]+ '.';

		'\n';
	*|;

  write data;
}%%

let () =
  let prints (_:string) = () in
  let ts = ref 0 and te = ref 0 and act = ref 0 in
  let data = "abbc1d1d2\n" in
  let cs = ref 0 in
  let p = ref 0 and pe = ref (String.length data) in
  let eof = ref !pe in
  %% write init;
  let on_last = ref 0 and on_next = ref 0 and on_lag = ref 0 and
  sw1 = ref 0 and sw2 = ref 0 in
  let cnt = ref 1 in
  %% write exec;
  let t = T.test' (fun x -> string_of_int !x) (!) in
  t on_last 1;
  t on_next 2;
  t on_lag 3;
  t sw1 4;
  t sw2 5;
  t cnt 6;
  ()

(*
on last     yes
on next     yes
on lag      yes
lm switch1  yes
lm switch2  yes
*)
