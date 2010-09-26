// -*-go-*-
//
// URL Parser
// Copyright (c) 2010 J.A. Roberts Tunney
// MIT License
//

package main

import (
	"os"
	"fmt"
	"strconv"
)

%% machine url_authority;
%% write data;

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
func (url *URL) parseAuthority(data []byte) (err os.Error) {
	cs, p, pe, eof := 0, 0, len(data), len(data)
	mark := 0

	// temporary holding place for user:pass and/or host:port cuz an
	// optional term (user[:pass]) coming before a mandatory term
	// (host[:pass]) would require require backtracking and all that
	// evil nondeterministic stuff which ragel seems to hate.  (for
	// this same reason you're also allowed to use square quotes
	// around the username.)
	var b1, b2 string

	// this buffer is so we can unescape while we roll
	var hex byte
	buf := make([]byte, len(data))
	amt := 0

	%%{
		action mark        { mark = p                         }
		action str_start   { amt = 0                          }
		action str_char    { buf[amt] = fc; amt++             }
		action hex_hi      { hex = unhex(fc) * 16             }
		action hex_lo      { hex += unhex(fc)
		                     buf[amt] = hex; amt++            }
		action copy_b1     { b1 = string(buf[0:amt]); amt = 0 }
		action copy_b2     { b2 = string(buf[0:amt]); amt = 0 }
		action copy_host   { url.Host = string(b1); amt = 0   }

		action copy_port {
			if b2 != "" {
				url.Port, err = strconv.Atoi(string(b2))
		        if err != nil { goto fail }
		        if url.Port > 65535 { goto fail }
			}
		}

		action params {
			url.Params = string(data[mark:p])
		}

		action params_eof {
			url.Params = string(data[mark:p])
			return nil
		}

		action atsymbol {
			url.User = string(b1)
			url.Pass = string(b2)
			b2 = ""
		}

		action alldone {
			url.Host = string(b1)
			if url.Host == "" {
				url.Host = string(buf[0:amt])
			} else {
				if amt > 0 {
					b2 = string(buf[0:amt])
				}
				if b2 != "" {
					url.Port, err = strconv.Atoi(string(b2))
					if err != nil { goto fail }
					if url.Port > 65535 { goto fail }
				}
			}
			return nil
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

	// if cs >= url_authority_first_final {
	// 	return nil
	// }

fail:
	// fmt.Println("error state", cs)
	// fmt.Println(string(data))
	// for i := 0; i < p; i++ {
	// 	fmt.Print(" ")
	// }
	// fmt.Println("^")
	// fmt.Println(url)
	return os.ErrorString(fmt.Sprintf("bad url authority: %#v", string(data)))
}
