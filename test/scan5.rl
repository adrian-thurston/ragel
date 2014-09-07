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
			print_string "on last     ";
			if !p+1 = !te then
				(on_last := !cnt; incr cnt; print_string "yes");
			print_string "\n";
		};

		'b'+ => {
			print_string "on next     ";
			if !p+1 = !te then
				( on_next := !cnt; incr cnt; print_string "yes");
			print_string "\n";
		};

		'c1' 'dxxx'? => {
			print_string "on lag      ";
			if !p+1 = !te then
				( on_lag := !cnt; incr cnt; print_string "yes"); 
			print_string "\n";
		};

		'd1' => {
			print_string "lm switch1  ";
			if !p+1 = !te then
				(sw1 := !cnt; incr cnt; print_string "yes");
			print_string "\n";
		};
		'd2' => {
			print_string "lm switch2  ";
			if !p+1 = !te then
        (sw2 := !cnt; incr cnt; print_string "yes");
			print_string "\n";
		};

		[d0-9]+ '.';

		'\n';
	*|;

  write data;
}%%

let () =
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
  let t = test' (fun x -> string_of_int !x) (!) in
  t on_last 1;
  t on_next 2;
  t on_lag 3;
  t sw1 4;
  t sw2 5;
  t cnt 6;
  ()

(* _____OUTPUT_____
on last     yes
on next     yes
on lag      yes
lm switch1  yes
lm switch2  yes
_____OUTPUT_____ *)
