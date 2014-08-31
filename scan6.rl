(*
 * @LANG: ocaml
 *)

let id x = x
let fail fmt = Printf.ksprintf failwith fmt
let pr fmt = Printf.ksprintf print_endline fmt

let failed fmt = Printf.ksprintf (fun s -> prerr_endline s; exit 1) fmt
let test' show f x y = if f x <> y then failed "FAILED: test %S" (show x)
let case = ref 0
let test f x y = incr case; if f x <> y then failed "FAILED: case %d" !case
let error f x = match try Some (f x) with _ -> None with Some _ -> failed "FAILED: fail %S" x | None -> ()

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

let fail fmt = Printf.ksprintf failwith fmt

let () =
  let expect = ref [`Pat1; `Any; `Pat2; `Any; `Any; `Any; ] in
  let got z = match !expect with
    | [] -> fail "nothing more expected"
    | x::xs -> expect := xs; if z <> x then fail "mismatch"
  in
  let ts = ref 0 and te = ref 0 and cs = ref 0 and act = ref 0 in
  let data = "araabccde" in
  let p = ref 0 and pe = ref (String.length data) in
  let eof = ref !pe in
  %% write init;
  %% write exec;
  ()

