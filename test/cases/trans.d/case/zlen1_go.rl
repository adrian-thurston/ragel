/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"




%%{
	machine zlen1;
	main := zlen;
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
	if cs >= zlen1_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"",
"x",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
