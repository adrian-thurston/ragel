/*
 * @LANG: go
 *
 * This test case is GoLang only because we need to exercise access in translated
 * output.
 */

package main
import "fmt"

%%{
	machine fbreak_eof;
	access foo_;

	main := "hello\n" %{ fbreak; };
}%%

%% write data;

var foo_cs = 0;

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
	if foo_cs >= fbreak_eof_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}

var inp []string = []string {
	"hello\n",
	"there\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}

##### OUTPUT #####
ACCEPT
FAIL
