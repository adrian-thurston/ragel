(*
 * @LANG: ocaml
 * @GENERATED: true
 *)

let ts = ref 0
let te = ref 0
let act = ref 0
let token = ref 0

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => {print_string( "on last     " );
if p.contents + 1 == te .contents then
begin
	print_string( "yes" );

end 
;
print_string( "\n" );
};

		'b'+ => {print_string( "on next     " );
if p.contents + 1 == te .contents then
begin
	print_string( "yes" );

end 
;
print_string( "\n" );
};

		'c1' 'dxxx'? => {print_string( "on lag      " );
if p.contents + 1 == te .contents then
begin
	print_string( "yes" );

end 
;
print_string( "\n" );
};

		'd1' => {print_string( "lm switch1  " );
if p.contents + 1 == te .contents then
begin
	print_string( "yes" );

end 
;
print_string( "\n" );
};
		'd2' => {print_string( "lm switch2  " );
if p.contents + 1 == te .contents then
begin
	print_string( "yes" );

end 
;
print_string( "\n" );
};

		[d0-9]+ '.';

		'\n';
	*|;
}%%




%% write data;

let exec data = 
  let buffer = String.create(1024) in 
  let blen :int ref = ref 0 in
  let cs = ref 0 in
  let p = ref 0 in
  let pe = ref (String.length data) in
	let eof = pe in
	%% write init;
	%% write exec;
	if !cs >= scanner_first_final then
		print_string "ACCEPT\n"
	else
		print_string "FAIL\n"
;;

let () =
	exec "abbc1d1d2\n";
  ()
;;

