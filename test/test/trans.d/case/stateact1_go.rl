/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"




%%{
	machine state_act;

	action a1 {fmt.Print( "a1\n" );}
	action a2 {fmt.Print( "a2\n" );}
	action b1 {fmt.Print( "b1\n" );}
	action b2 {fmt.Print( "b2\n" );}
	action c1 {fmt.Print( "c1\n" );}
	action c2 {fmt.Print( "c2\n" );}
	action next_again {fnext again;
}

	hi = 'hi';
	line = again: 
			hi 
				>to b1 
				>from b2 
			'\n' 
				>to c1 
				>from c2 
				@next_again;
		 
	main := line*
			>to a1 
			>from a2;
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
	if cs >= state_act_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"hi\nhi\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
