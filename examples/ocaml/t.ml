(* Common utility code *)

let id x = x
let fail fmt = Printf.ksprintf failwith fmt
let pr fmt = Printf.ksprintf print_endline fmt

let failed fmt = Printf.ksprintf (fun s -> prerr_endline s; exit 1) fmt
let test' show f x y = if f x <> y then failed "FAILED: test %S" (show x)
let case = ref 0
let test f x y = incr case; if f x <> y then failed "FAILED: case %d" !case
let error f x = match try Some (f x) with _ -> None with Some _ -> failed "FAILED: fail %S" x | None -> ()

