/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"



%%{
	machine rangei;

	main := 
		'a' ../i 'z' .
		'A' ../i 'Z' .
		60 ../i 93 .
		94 ../i 125 .
		86 ../i 101 .
		60 ../i 125
		'';
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
	if cs >= rangei_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"AaBbAa",
"Aa`bAa",
"AaB@Aa",
"AaBbMa",
"AaBbma",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
