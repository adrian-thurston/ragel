#! /usr/bin/env ocaml

(* test suite *)

open Printf

let names = [
"atoi";
"scan2";
"scan1";
"gotocallret";
"rpn";
]

let () =
  let ragel = match Sys.argv with [|_;s|] -> s | _ -> "ragel" in
  let modes = ["-T0";"-T1";"-F0";"-F1";"-G0";] in
  assert (0 = Sys.command "ocamlc -c -g t.ml");
  List.iter (fun mode ->
  List.iter (fun name ->
    let cmd = sprintf "%s %s -O %s.rl && ocamlc -g t.cmo %s.ml -o %s && ./%s" ragel mode name name name name in
    print_endline ("+ " ^ cmd);
    if 0 <> Sys.command cmd then (prerr_endline "RESULT: FAILED"; exit 2)) names) modes;
  print_endline "RESULT: OK"

