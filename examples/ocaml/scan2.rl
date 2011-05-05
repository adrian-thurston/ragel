
%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
        'a' => {
			got `Pat1;
		};

        [ab]+ . 'c' => {
			got `Pat2;
		};

        any => {
			got `Any;
		};
	*|;

  write data;
}%%

let () =
  let expect = ref [`Pat1; `Any; `Pat2; `Any; `Any; `Any; ] in
  let got z = match !expect with
    | [] -> T.fail "nothing more expected"
    | x::xs -> expect := xs; if z <> x then T.fail "mismatch"
  in
  let ts = ref 0 and te = ref 0 and cs = ref 0 and act = ref 0 in
  let data = "araabccde" in
  let p = ref 0 and pe = ref (String.length data) in
  let eof = ref !pe in
  %% write init;
  %% write exec;
  ()

