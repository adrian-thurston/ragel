(*
// -*-go-*-
//
// Reverse Polish Notation Calculator
// Copyright (c) 2010 J.A. Roberts Tunney
// MIT License
//
// To compile:
//
//   ragel -Z -G2 -o rpn.go rpn.rl
//   6g -o rpn.6 rpn.go
//   6l -o rpn rpn.6
//   ./rpn
//
// To show a diagram of your state machine:
//
//   ragel -V -G2 -p -o rpn.dot rpn.rl
//   dot -Tpng -o rpn.png rpn.dot
//   chrome rpn.png
//
*)

%% machine rpn;
%% write data;

let fail fmt = Printf.ksprintf failwith fmt

let rpn data =
	let (cs, p, pe) = (ref 0, ref 0, ref (String.length data)) in
	let mark = ref 0 in
	let st = Stack.create () in

	%%{
		action mark { mark := !p }
		action push { Stack.push (int_of_string (String.sub data !mark (!p - !mark))) st }
		action add  { let y = Stack.pop st in let x = Stack.pop st in Stack.push (x + y) st }
		action sub  { let y = Stack.pop st in let x = Stack.pop st in Stack.push (x - y) st }
		action mul  { let y = Stack.pop st in let x = Stack.pop st in Stack.push (x * y) st }
		action div  { let y = Stack.pop st in let x = Stack.pop st in Stack.push (x / y) st }
		action abs  { Stack.push (abs (Stack.pop st)) st }
		action abba { Stack.push 666 st }

		stuff  = digit+ >mark %push
		       | '+' @add
		       | '-' @sub
		       | '*' @mul
		       | '/' @div
		       | 'abs' %abs
		       | 'add' %add
		       | 'abba' %abba
		       ;

		main := ( space | stuff space )* ;

		write init;
		write exec;
	}%%

	if !cs < rpn_first_final then
  begin
		if !p = !pe then
			fail "unexpected eof"
		else
			fail "error at position %d" !p
  end;

	if Stack.is_empty st then
		fail "rpn stack empty on result"
  else
  	Stack.pop st

(* ////////////////////////////////////////////////////////////////////// *)

let rpnTests = [
	("666\n", 666);
	("666 111\n", 111);
	("4 3 add\n", 7);
	("4 3 +\n", 7);
	("4 3 -\n", 1);
	("4 3 *\n", 12);
	("6 2 /\n", 3);
	("0 3 -\n", -3);
	("0 3 - abs\n", 3);
	(" 2  2 + 3 - \n", 1);
	("10 7 3 2 * - +\n", 11);
	("abba abba add\n", 1332);
]

let rpnFailTests = [
	("\n")
]

let () =
  List.iter (fun (s,x) -> T.test rpn s x) rpnTests;
  List.iter (fun s -> T.error rpn s) rpnFailTests

