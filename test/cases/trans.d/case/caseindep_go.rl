/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"



%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
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
	%% write exec;
}
func finish() {
	if cs >= indep_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"1hello\n",
"1HELLO\n",
"1HeLLo\n",
"2hello\n",
"2HELLO\n",
"2HeLLo\n",
"3hello\n",
"3HELLO\n",
"3HeLLo\n",
"4hello\n",
"4HELLO\n",
"4HeLLo\n",
"5hello\n",
"5HELLO\n",
"5HeLLo\n",
"6hello\n",
"6HELLO\n",
"6HeLLo\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
