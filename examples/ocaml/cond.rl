
%% machine cond;
%% write data;

let fail fmt = Printf.ksprintf failwith fmt

let run data =
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in

  %%{
    action rec_num { i = 0; n = getnumber(); }
    action test_len { i++ < n }
    main := (
      "d"
      [0-9]+ %rec_num
      ":"
      ( [a-z] when test_len )*
    )**;

	  write init;
		write exec;
	}%%

	if !cs < cond_first_final then
    fail "parsing failed"

let () =
  run "d2:aac"

