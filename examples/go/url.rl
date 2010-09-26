// -*-go-*-
//
// URL Parser
// Copyright (c) 2010 J.A. Roberts Tunney
// MIT License
//
// To compile:
//
//   ragel -Z -G2 -o url.go url.rl
//   ragel -Z -G2 -o url_authority.go url_authority.rl
//   6g -o url.6 url.go url_authority.go
//   6l -o url url.6
//   ./url
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

package main

import (
	"os"
	"fmt"
	"time"
)

type URL struct {
	Scheme    string // http, sip, file, etc. (never blank, always lowercase)
	User      string // who is you yo
	Pass      string // for like, logging in
	Host      string // IP 4/6 address or hostname (mandatory)
	Port      int    // like 80 or 5060 (default 0)
	Params    string // stuff after ';' (NOT UNESCAPED, used in sip)
	Path      string // stuff starting with '/'
	Query     string // stuff after '?' (NOT UNESCAPED)
	Fragment  string // stuff after '#'
}

%% machine url;
%% write data;

// i parse absolute urls and don't suck at it.  i'll parse just about
// any type of url you can think of and give you a human-friendly data
// structure.
//
// this routine takes no more than a few microseconds, is reentrant,
// performs in a predictable manner (for security/soft-realtime,)
// doesn't modify your `data` buffer, and under no circumstances will
// it panic (i hope!)
func URLParse(data []byte) (url *URL, err os.Error) {
	cs, p, pe, eof := 0, 0, len(data), len(data)
	mark := 0
	url = new(URL)

	// this buffer is so we can unescape while we roll
	var hex byte
	buf := make([]byte, len(data))
	amt := 0

	%%{
		action mark      { mark = p                               }
		action str_start { amt = 0                                }
		action str_char  { buf[amt] = fc; amt++                   }
		action str_lower { buf[amt] = fc + 0x20; amt++            }
		action hex_hi    { hex = unhex(fc) * 16                   }
		action hex_lo    { hex += unhex(fc)
		                   buf[amt] = hex; amt++                  }
		action scheme    { url.Scheme = string(buf[0:amt])        }
		action authority { err = url.parseAuthority(data[mark:p])
		                   if err != nil { return nil, err }      }
		action path      { url.Path = string(buf[0:amt])          }
		action query     { url.Query = string(data[mark:p])       }
		action fragment  { url.Fragment = string(buf[0:amt])      }

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

	if cs < url_first_final {
		if p == pe {
			return nil, os.ErrorString(
				fmt.Sprintf("unexpected eof: %s", data))
		} else {
			return nil, os.ErrorString(
				fmt.Sprintf("error in url at pos %d: %s", p, data))
		}
	}

	return url, nil
}

func unhex(b byte) byte {
	switch {
	case '0' <= b && b <= '9':
		return b - '0'
	case 'a' <= b && b <= 'f':
		return b - 'a' + 10
	case 'A' <= b && b <= 'F':
		return b - 'A' + 10
	}
	return 0
}

//////////////////////////////////////////////////////////////////////

type urlTest struct {
	s []byte
	url URL
}

var urlTests = []urlTest{

	urlTest{
		[]byte("http://user:pass@example.com:80;hello/lol.php?fun#omg"),
		URL{
			Scheme: "http",
			User: "user",
			Pass: "pass",
			Host: "example.com",
			Port: 80,
			Params: "hello",
			Path: "/lol.php",
			Query: "fun",
			Fragment: "omg",
		},
	},

	urlTest{
		[]byte("a:b"),
		URL{
			Scheme: "a",
			Host: "b",
		},
	},

	urlTest{
		[]byte("GoPHeR://@example.com@:;/?#"),
		URL{
			Scheme: "gopher",
			Host: "@example.com@",
			Path: "/",
		},
	},

	urlTest{
		[]byte("ldap://[2001:db8::7]/c=GB?objectClass/?one"),
		URL{
			Scheme: "ldap",
			Host: "2001:db8::7",
			Path: "/c=GB",
			Query: "objectClass/?one",
		},
	},

	urlTest{
		[]byte("http://user@example.com"),
		URL{
			Scheme: "http",
			User: "user",
			Host: "example.com",
		},
	},

	urlTest{
		[]byte("http://品研发和研发管@☃.com:65000;%20"),
		URL{
			Scheme: "http",
			User: "品研发和研发管",
			Host: "☃.com",
			Port: 65000,
			Params: "%20",
		},
	},

	urlTest{
		[]byte("https://example.com:80"),
		URL{
			Scheme: "https",
			Host: "example.com",
			Port: 80,
		},
	},

	urlTest{
		[]byte("file:///etc/passwd"),
		URL{
			Scheme: "file",
			Path: "/etc/passwd",
		},
	},

	urlTest{
		[]byte("file:///c:/WINDOWS/clock.avi"),
		URL{
			Scheme: "file",
			Path: "/c:/WINDOWS/clock.avi", // <-- is this kosher?
		},
	},

	urlTest{
		[]byte("file://hostname/path/to/the%20file.txt"),
		URL{
			Scheme: "file",
			Host: "hostname",
			Path: "/path/to/the file.txt",
		},
	},

	urlTest{
		[]byte("sip:example.com"),
		URL{
			Scheme: "sip",
			Host: "example.com",
		},
	},

	urlTest{
		[]byte("sip:example.com:5060"),
		URL{
			Scheme: "sip",
			Host: "example.com",
			Port: 5060,
		},
	},

	urlTest{
		[]byte("mailto:ditto@pokémon.com"),
		URL{
			Scheme: "mailto",
			User: "ditto",
			Host: "pokémon.com",
		},
	},

	urlTest{
		[]byte("sip:[dead:beef::666]:5060"),
		URL{
			Scheme: "sip",
			Host: "dead:beef::666",
			Port: 5060,
		},
	},

	urlTest{
		[]byte("tel:+12126660420"),
		URL{
			Scheme: "tel",
			Host: "+12126660420",
		},
	},

	urlTest{
		[]byte("sip:bob%20barker:priceisright@[dead:beef::666]:5060;isup-oli=00/palfun.html?haha#omg"),
		URL{
			Scheme: "sip",
			User: "bob barker",
			Pass: "priceisright",
			Host: "dead:beef::666",
			Port: 5060,
			Params: "isup-oli=00",
			Path: "/palfun.html",
			Query: "haha",
			Fragment: "omg",
		},
	},

	urlTest{
		[]byte("http://www.google.com/search?%68l=en&safe=off&q=omfg&aq=f&aqi=g2g-s1g1g-s1g5&aql=&oq=&gs_rfai="),
		URL{
			Scheme: "http",
			Host: "www.google.com",
			Path: "/search",
			Query: "%68l=en&safe=off&q=omfg&aq=f&aqi=g2g-s1g1g-s1g5&aql=&oq=&gs_rfai=",
		},
	},

}

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

func bench() {
	const rounds = 10000
	for _, s := range [][]byte{
		[]byte("a:a"),
		[]byte("http://google.com/"),
		[]byte("sip:jtunney@lobstertech.com"),
		[]byte("http://user:pass@example.com:80;hello/lol.php?fun#omg"),
		[]byte("file:///etc/passwd"),
	} {
		ts1 := time.Nanoseconds()
		for i := 0; i < rounds; i++ {
			URLParse(s)
		}
		ts2 := time.Nanoseconds()
		fmt.Printf("BENCH URLParse(%s) -> %d ns\n", s, (ts2 - ts1) / rounds)
	}
}

func test() (rc int) {
	for _, test := range urlTests {
		url, err := URLParse(test.s)
		if err != nil {
			fmt.Fprintf(os.Stderr, "FAIL url(%#v) %s\n", string(test.s), err)
			rc = 1
			continue
		}
		if !test.compare(url) {
			rc = 1
		}
	}
	return rc
}

func main() {
	rc := test()
	if rc == 0 {
		bench()
	}
	os.Exit(rc)
}
