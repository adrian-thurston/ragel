/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var ts  int ;
var te  int ;
var act  int ;
var token  int ;

%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
	  'a' => {fmt.Print( "pat1\n" );};

	  [ab]+ . 'c' => {fmt.Print( "pat2\n" );};

	  any;
	*|;
}%%


var cs int;
var blen int;
var buffer [1024] byte;

%% write data;

func prepare() {
	%% write init;
}

func exec(data string) {
	var p int = 0
	var pe int = len(data)
	var eof int = pe
	%% write exec;
}
func finish() {
	if cs >= scanner_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"ba a",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
