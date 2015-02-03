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
	machine atoi;
	write data;
}%%

let fail fmt = Printf.ksprintf failwith fmt

let atoi data =
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
  let neg = ref false in
  let res = ref 0 in

	%%{
		action see_neg   { neg := true; }
		action add_digit { res := !res * 10 + (fc - Char.code '0'); }

		main :=
			( '-'@see_neg | '+' )? ( digit @add_digit )+
			'\n'?
			;

		write init;
		write exec;
	}%%

	if !neg then
		res := (-1) * !res ;

	print_int res.contents;
	print_string "\n";

	if !cs < atoi_first_final then
    fail "atoi: cs %d < %d" !cs atoi_first_final;

	!res 
;;

let () =
  let t = test atoi in
	t "7" 7;
  t "666" 666;
  t "-666" (-666);
  t "+666" 666;
  t "123456789" 123456789;
  t "+123456789\n" 123456789;
  error atoi "+ 1234567890";
  ()
;;

##### OUTPUT #####
7
666
-666
666
123456789
123456789
0
