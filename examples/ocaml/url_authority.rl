(*
// -*-go-*-
//
// URL Parser
// Copyright (c) 2010 J.A. Roberts Tunney
// MIT License
//
*)

%% machine url_authority;
%% write data;

(*
// i parse strings like `alice@pokémon.com`.
//
// sounds simple right?  but i also parse stuff like:
//
//   bob%20barker:priceisright@[dead:beef::666]:5060;isup-oli=00
//
// which in actual reality is:
//
// - User: "bob barker"
// - Pass: "priceisright"
// - Host: "dead:beef::666"
// - Port: 5060
// - Params: "isup-oli=00"
//
// which was probably extracted from an absolute url that looked like:
//
//   sip:bob%20barker:priceisright@[dead:beef::666]:5060;isup-oli=00/palfun.html?haha#omg
//
// which was probably extracted from its address form:
//
//   "Bob Barker" <sip:bob%20barker:priceisright@[dead:beef::666]:5060;isup-oli=00/palfun.html?haha#omg>;tag=666
//
// who would have thought this could be so hard ._.
*)

type url = {
	scheme   : string; (* http, sip, file, etc. (never blank, always lowercase) *)
	user     : string; (* who is you *)
	pass     : string; (* for like, logging in *)
	host     : string; (* IP 4/6 address or hostname (mandatory) *)
	port     : int;    (* like 80 or 5060 (default 0) *)
	params   : string; (* stuff after ';' (NOT UNESCAPED, used in sip) *)
	path     : string; (* stuff starting with '/' *)
	query    : string; (* stuff after '?' (NOT UNESCAPED) *)
	fragment : string; (* stuff after '#' *)
}

let fail fmt = Printf.ksprintf failwith fmt

let unhex c =
  match c with
  | '0'..'9' -> Char.code c - Char.code '0'
  | 'a'..'f' -> Char.code c - Char.code 'a' + 10
  | 'A'..'F' -> Char.code c - Char.code 'A' + 10
  | _ -> fail "unhex %C" c

let parse_authority u data =
	let (cs, p, pe, eof) = (ref 0, ref 0, ref (String.length data), ref (String.length data)) in
	let mark = ref 0 in

(*
	// temporary holding place for user:pass and/or host:port cuz an
	// optional term (user[:pass]) coming before a mandatory term
	// (host[:pass]) would require require backtracking and all that
	// evil nondeterministic stuff which ragel seems to hate.  (for
	// this same reason you're also allowed to use square quotes
	// around the username.)
*)
	let (b1, b2) = (ref "", ref "") in

(*
	// this buffer is so we can unescape while we roll
	var hex byte
	buf := make([]byte, len(data))
	amt := 0
*)
  let buf = Buffer.create 10 in
  let hex = ref 0 in

	%%{
		action mark      { mark := !p                             }
		action str_start { Buffer.reset buf                       }
		action str_char  { Buffer.add_char buf fc                 }
		action str_lower { Buffer.add_char buf (Char.lowercase fc)}
		action hex_hi    { hex := unhex fc * 16                   }
		action hex_lo    { Buffer.add_char buf (Char.chr (!hex + unhex fc)) }
		action copy_b1     { b1 := Buffer.contents buf; Buffer.clear buf }
		action copy_b2     { b2 := Buffer.contents buf; Buffer.clear buf }
		action copy_host   { u := { !u with host = !b1 }; Buffer.clear buf }

		action copy_port {
			if !b2 <> "" then
      begin
				u := { !u with port = int_of_string !b2 };
		    if !u.port > 65535 then fail "bad url authority: %S" data
			end
		}

		action params {
			u := { !u with params = String.sub data !mark (!p - !mark) }
		}

		action params_eof {
			u := { !u with params = String.sub data !mark (!p - !mark) }
(*       return nil *)
		}

		action atsymbol {
      u := { !u with user = !b1; pass = !b2 };
			b2 := ""
		}

		action alldone {
			u := { !u with host = !b1 };
			if !u.host = "" then
				u := { !u with host = Buffer.contents buf }
			else
      begin
				if Buffer.length buf > 0 then b2 := Buffer.contents buf;
				if !b2 <> "" then
        begin
          u := { !u with port = int_of_string !b2 };
					if !u.port > 65535 then fail "bad url authority: %S" data
				end
			end
(* 			return nil *)
		}

		# define what a single character is allowed to be
		toxic         = ( cntrl | 127 ) ;
		scary         = ( toxic | space | "\"" | "#" | "%" | "<" | ">" ) ;
		authdelims    = ( "/" | "?" | "#" | ":" | "@" | ";" | "[" | "]" ) ;
		userchars     = any -- ( authdelims | scary ) ;
		userchars_esc = userchars | ":" ;
		passchars     = userchars ;
		hostchars     = passchars | "@" ;
		hostchars_esc = hostchars | ":" ;
		portchars     = digit ;
		paramchars    = hostchars | ":" | ";" ;

		# define how characters trigger actions
		escape        = "%" xdigit xdigit ;
		unescape      = "%" ( xdigit @hex_hi ) ( xdigit @hex_lo ) ;
		userchar      = unescape | ( userchars @str_char ) ;
		userchar_esc  = unescape | ( userchars_esc @str_char ) ;
		passchar      = unescape | ( passchars @str_char ) ;
		hostchar      = unescape | ( hostchars @str_char ) ;
		hostchar_esc  = unescape | ( hostchars_esc @str_char ) ;
		portchar      = unescape | ( portchars @str_char ) ;
		paramchar     = escape | paramchars ;

		# define multi-character patterns
		user_plain    = userchar+ >str_start %copy_b1 ;
		user_quoted   = "[" ( userchar_esc+ >str_start %copy_b1 ) "]" ;
		user          = ( user_quoted | user_plain ) %/alldone ;
		pass          = passchar+ >str_start %copy_b2 %/alldone ;
		host_plain    = hostchar+ >str_start %copy_b1 %copy_host ;
		host_quoted   = "[" ( hostchar_esc+ >str_start %copy_b1 %copy_host ) "]" ;
		host          = ( host_quoted | host_plain ) %/alldone ;
		port          = portchar* >str_start %copy_b2 %copy_port %/alldone ;
		params        = ";" ( paramchar* >mark %params %/params_eof ) ;
		userpass      = user ( ":" pass )? ;
		hostport      = host ( ":" port )? ;
		authority     = ( userpass ( "@" @atsymbol ) )? hostport params? ;

		main := authority;
		write init;
		write exec;
	}%%

  (*
	// if cs >= url_authority_first_final {
	// 	return nil
	// }
  *)

  (*
	// fmt.Println("error state", cs)
	// fmt.Println(string(data))
	// for i := 0; i < p; i++ {
	// 	fmt.Print(" ")
	// }
	// fmt.Println("^")
	// fmt.Println(url)
  *)

