/*
 * @LANG: go
 *
 * This test case is GoLang only because we need to exercise access in
 * translated output.
 */

package main
import "fmt"

%%{
	machine eofcall3;

	something := "something";
	nothing   := "";

	ret :=
		".\n" %{ fret; };

	main := (
		"goto_hello\n"  %{ fgoto something; } |
		"goto_friend\n" %{ fgoto nothing; } |
		"goto*hello\n"  %{ fgoto *(eofcall3_en_something); } |
		"goto*friend\n" %{ fgoto *(eofcall3_en_nothing); } |
		"call_hello\n"  %{ fcall something; } |
		"call_friend\n" %{ fcall nothing; } |
		"call*hello\n"  %{ fcall *(eofcall3_en_something); } |
		"call*friend\n" %{ fcall *(eofcall3_en_nothing); } |
		"ret_hello" @{ fcall ret; } "MORE" |
		"ret_friend" @{ fcall ret; }
	);

}%%

%% write data;

var cs = 0;
var top = 0;
var stack [10]int;

func prepare() {
	%%write init;
}

func exec(data string) {
	var p int = 0
	var pe int = len(data)
	var eof int = pe
	%% write exec;
}

func finish() {
	if cs >= eofcall3_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}

	if ( cs == eofcall3_en_something ) {
		fmt.Println("something")
	}

	if ( cs == eofcall3_en_nothing ) {
		fmt.Println("nothing")
	}
}

var inp []string = []string {
	"goto_hello\n",
	"goto_friend\n",
	"goto*hello\n",
	"goto*friend\n",
	"call_hello\n",
	"call_friend\n",
	"call*hello\n",
	"call*friend\n",
	"ret_hello.\n",
	"ret_friend.\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}

/* _____OUTPUT_____
FAIL
something
ACCEPT
nothing
FAIL
something
ACCEPT
nothing
FAIL
something
ACCEPT
nothing
FAIL
something
ACCEPT
nothing
FAIL
ACCEPT
_____OUTPUT_____ */
