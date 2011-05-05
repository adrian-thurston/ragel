
%%{
	machine atoi;
	write data;
}%%

let atoi data =
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
  let neg = ref false in
  let res = ref 0 in

	%%{
		action see_neg   { neg := true; }
		action add_digit { res := !res * 10 + (Char.code fc - Char.code '0'); }

		main :=
			( '-'@see_neg | '+' )? ( digit @add_digit )+
			'\n'?
			;

		write init;
		write exec;
	}%%

	if !neg then
		res := (-1) * !res ;

	if !cs < atoi_first_final then
    T.fail "atoi: cs %d < %d" !cs atoi_first_final;

	!res 
;;
let () =
  let t = T.test atoi in
	t "7" 7;
  t "666" 666;
  t "-666" (-666);
  t "+666" 666;
  t "123456789" 123456789;
  t "+123456789\n" 123456789;
  T.error atoi "+ 1234567890";
  ()
;;
