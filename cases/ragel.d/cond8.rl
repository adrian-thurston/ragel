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

%% machine cond;
%% write data;

let fail fmt = Printf.ksprintf failwith fmt

let run data n =
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
  let i = ref 0 in

  %%{
    action test_len { i < n }

    main := (
      "d"
      [0-9]+
      ":"
      ( [a-z] when test_len )* ${ i := i.contents + 1 }
    )**;

		write init;
		write exec;
	}%%

	if !cs < cond_first_final then
	    print_string "fail\n"
	else
		print_string "ok\n"

let () =
  run "d2:abc" ( ref 1 );
  run "d2:abc" ( ref 2 );
  run "d2:abc" ( ref 3 );
  run "d2:abc" ( ref 4 );

##### OUTPUT #####
fail
fail
ok
ok
