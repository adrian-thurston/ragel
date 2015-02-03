(*
 * @LANG: ocaml
 *)

(*
//
// URL Parser
// Copyright (c) 2010 J.A. Roberts Tunney
// MIT License
//
// Converted to OCaml by ygrek
//
// To compile:
//
//	ragel -O url.rl -o url.ml
//  ragel -O url_authority.rl -o url_authority.ml
//	ocamlopt -g unix.cmxa url_authority.ml url.ml -o url
//	./url
//
// To show a diagram of your state machine:
//
//   ragel -V -G2 -p -o url.dot url.rl
//   dot -Tpng -o url.png url.dot
//   chrome url.png
//
//   ragel -V -G2 -p -o url_authority.dot url_authority.rl
//   dot -Tpng -o url_authority.png url_authority.dot
//   chrome url_authority.png
//
// Reference:
//
// - http://tools.ietf.org/html/rfc3986
//
*)

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
		action str_char  { Buffer.add_char buf data.[p.contents]                }
		action str_lower { Buffer.add_char buf (Char.lowercase data.[p.contents])}
		action hex_hi    { hex := unhex data.[p.contents] * 16                   }
		action hex_lo    { Buffer.add_char buf (Char.chr (!hex + unhex data.[p.contents])) }
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
  ;;


let dummy = {
    scheme = ""; user = ""; pass = ""; host = ""; port = 0; 
    params = ""; path = ""; query = ""; fragment = ""; }

let show u =
  Printf.sprintf "%s :// %s : %s @ %s : %d ;%s %s ?%s #%s" u.scheme u.user u.pass u.host u.port
    u.params u.path u.query u.fragment

%% machine url;
%% write data;

(*
// i parse absolute urls and don't suck at it.  i'll parse just about
// any type of url you can think of and give you a human-friendly data
// structure.
//
// this routine takes no more than a few microseconds, is reentrant,
// performs in a predictable manner (for security/soft-realtime,)
// doesn't modify your `data` buffer, and under no circumstances will
// it panic (i hope!)
*)
let url_parse data =
	let (cs, p, pe, eof) = (ref 0, ref 0, ref (String.length data), ref (String.length data)) in
	let mark = ref 0 in
  let u = ref dummy in

  (*
	// this buffer is so we can unescape while we roll
  *)
	let buf = Buffer.create 16 in
  let hex = ref 0 in

	%%{
		action mark      { mark := !p                             }
		action str_start { Buffer.reset buf                       }
		action str_char  { Buffer.add_char buf data.[p.contents]                 }
		action str_lower { Buffer.add_char buf (Char.lowercase data.[p.contents])}
		action hex_hi    { hex := unhex data.[p.contents] * 16                   }
		action hex_lo    { Buffer.add_char buf (Char.chr (!hex + unhex data.[p.contents])) }
		action scheme    { u := { !u with scheme = Buffer.contents buf } }
		action authority { parse_authority u (String.sub data !mark (!p - !mark)) }
		action path      { u := { !u with path = Buffer.contents buf } }
		action query     { u := { !u with query = String.sub data !mark (!p - !mark) } }
		action fragment  { u := { !u with fragment = Buffer.contents buf } }

		# # do this instead if you *actually* use URNs (lol)
		# action authority { url.Authority = string(data[mark:p]) }

		# define what a single character is allowed to be
		toxic     = ( cntrl | 127 ) ;
		scary     = ( toxic | " " | "\"" | "#" | "%" | "<" | ">" ) ;
		schmchars = ( lower | digit | "+" | "-" | "." ) ;
		authchars = any -- ( scary | "/" | "?" | "#" ) ;
		pathchars = any -- ( scary | "?" | "#" ) ;
		querchars = any -- ( scary | "#" ) ;
		fragchars = any -- ( scary ) ;

		# define how characters trigger actions
		escape    = "%" xdigit xdigit ;
		unescape  = "%" ( xdigit @hex_hi ) ( xdigit @hex_lo ) ;
		schmfirst = ( upper @str_lower ) | ( lower @str_char ) ;
		schmchar  = ( upper @str_lower ) | ( schmchars @str_char ) ;
		authchar  = escape | authchars ;
		pathchar  = unescape | ( pathchars @str_char ) ;
		querchar  = escape | querchars ;
		fragchar  = unescape | ( fragchars @str_char ) ;

		# define multi-character patterns
		scheme    = ( schmfirst schmchar* ) >str_start %scheme ;
		authority = authchar+ >mark %authority ;
		path      = ( ( "/" @str_char ) pathchar* ) >str_start %path ;
		query     = "?" ( querchar* >mark %query ) ;
		fragment  = "#" ( fragchar* >str_start %fragment ) ;
		url       = scheme ":" "//"? authority path? query? fragment?
			      | scheme ":" "//" authority? path? query? fragment?
			      ;

		main := url;
		write init;
		write exec;
	}%%

	if !cs < url_first_final then
		if !p = !pe then
      fail "unexpected eof: %s" data
		else
			fail "error in url at pos %d (%c): %s" !p data.[!p] data
	else
    !u

(* ////////////////////////////////////////////////////////////////////// *)

let tests = [
		"http://user:pass@example.com:80;hello/lol.php?fun#omg",
		{
			scheme = "http";
			user = "user";
			pass = "pass";
			host = "example.com";
			port = 80;
			params = "hello";
			path = "/lol.php";
			query = "fun";
			fragment = "omg";
		};

		"a:b",
		{ dummy with
			scheme = "a";
			host = "b";
		};

		"GoPHeR://@example.com@:;/?#",
		{ dummy with
			scheme = "gopher";
			host = "@example.com@";
			path = "/";
		};

		"ldap://[2001:db8::7]/c=GB?objectClass/?one",
		{ dummy with
			scheme = "ldap";
			host = "2001:db8::7";
			path = "/c=GB";
			query = "objectClass/?one";
		};

		"http://user@example.com",
		{ dummy with
			scheme = "http";
			user = "user";
			host = "example.com";
		};

		"http://品研发和研发管@☃.com:65000;%20",
		{ dummy with
			scheme = "http";
			user = "品研发和研发管";
			host = "☃.com";
			port = 65000;
			params = "%20";
		};

		"https://example.com:80",
		{ dummy with
			scheme = "https";
			host = "example.com";
			port = 80;
		};

		"file:///etc/passwd",
		{ dummy with
			scheme = "file";
			path = "/etc/passwd";
		};

		"file:///c:/WINDOWS/clock.avi",
		{ dummy with
			scheme = "file";
			path = "/c:/WINDOWS/clock.avi"; (* <-- is this kosher? *)
		};

		"file://hostname/path/to/the%20file.txt",
		{ dummy with
			scheme = "file";
			host = "hostname";
			path = "/path/to/the file.txt";
		};

		"sip:example.com",
		{ dummy with
			scheme = "sip";
			host = "example.com";
		};

		"sip:example.com:5060",
		{ dummy with
			scheme = "sip";
			host = "example.com";
			port = 5060;
		};

		"mailto:ditto@pokémon.com",
		{ dummy with
			scheme = "mailto";
			user = "ditto";
			host = "pokémon.com";
		};

		"sip:[dead:beef::666]:5060",
		{ dummy with
			scheme = "sip";
			host = "dead:beef::666";
			port = 5060;
		};

		"tel:+12126660420",
		{ dummy with
			scheme = "tel";
			host = "+12126660420";
		};

		"sip:bob%20barker:priceisright@[dead:beef::666]:5060;isup-oli=00/palfun.html?haha#omg",
		{
			scheme = "sip";
			user = "bob barker";
			pass = "priceisright";
			host = "dead:beef::666";
			port = 5060;
			params = "isup-oli=00";
			path = "/palfun.html";
			query = "haha";
			fragment = "omg";
		};

		"http://www.google.com/search?%68l=en&safe=off&q=omfg&aq=f&aqi=g2g-s1g1g-s1g5&aql=&oq=&gs_rfai=",
		{ dummy with
			scheme = "http";
			host = "www.google.com";
			path = "/search";
			query = "%68l=en&safe=off&q=omfg&aq=f&aqi=g2g-s1g1g-s1g5&aql=&oq=&gs_rfai=";
		};
]

(*
func (test *urlTest) compare(url *URL) (passed bool) {
	if url.Scheme != test.url.Scheme {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) scheme: %#v != %#v\n",
			string(test.s), url.Scheme, test.url.Scheme)
		passed = true
	}
	if url.User != test.url.User {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) user: %#v != %#v\n",
			string(test.s), url.User, test.url.User)
		passed = true
	}
	if url.Pass != test.url.Pass {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) pass: %#v != %#v\n",
			string(test.s), url.Pass, test.url.Pass)
		passed = true
	}
	if url.Host != test.url.Host {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) host: %#v != %#v\n",
			string(test.s), url.Host, test.url.Host)
		passed = true
	}
	if url.Port != test.url.Port {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) port: %#v != %#v\n",
			string(test.s), url.Port, test.url.Port)
		passed = true
	}
	if url.Port != test.url.Port {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) port: %#v != %#v\n",
			string(test.s), url.Port, test.url.Port)
		passed = true
	}
	if url.Params != test.url.Params {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) params: %#v != %#v\n",
			string(test.s), url.Params, test.url.Params)
		passed = true
	}
	if url.Path != test.url.Path {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) path: %#v != %#v\n",
			string(test.s), url.Path, test.url.Path)
		passed = true
	}
	if url.Query != test.url.Query {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) query: %#v != %#v\n",
			string(test.s), url.Query, test.url.Query)
		passed = true
	}
	if url.Fragment != test.url.Fragment {
		fmt.Fprintf(os.Stderr, "FAIL url(%#v) fragment: %#v != %#v\n",
			string(test.s), url.Fragment, test.url.Fragment)
		passed = true
	}
	return !passed
}
*)

let bench () = 
	let rounds = 0 in
	let urls = [
		"a:a";
		"http://google.com/";
		"sip:jtunney@lobstertech.com";
		"http://user:pass@example.com:80;hello/lol.php?fun#omg";
		"file:///etc/passwd";
  ] in
	List.iter (fun url ->
		for i = 1 to rounds do
			ignore (url_parse url)
		done;
		Printf.printf "BENCH parse %S \n%!" url
	) urls

let test () =
  List.iter (fun (s,res) ->
		let url = url_parse s in
		if url <> res then
      fail "got %S for %S" (show url) (*show res*) s
  ) tests

let () =
	test ();
	bench ();
	exit 0

##### OUTPUT #####
BENCH parse "a:a" 
BENCH parse "http://google.com/" 
BENCH parse "sip:jtunney@lobstertech.com" 
BENCH parse "http://user:pass@example.com:80;hello/lol.php?fun#omg" 
BENCH parse "file:///etc/passwd" 
