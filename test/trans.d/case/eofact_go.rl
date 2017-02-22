/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"




%%{
	machine eofact;

	action a1 {fmt.Print( "a1\n" );}
	action a2 {fmt.Print( "a2\n" );}
	action a3 {fmt.Print( "a3\n" );}
	action a4 {fmt.Print( "a4\n" );}


	main := (
		'hello' @eof a1 %eof a2 '\n'? |
		'there' @eof a3 %eof a4
	);

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
	if cs >= eofact_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"",
"h",
"hell",
"hello",
"hello\n",
"t",
"ther",
"there",
"friend",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
