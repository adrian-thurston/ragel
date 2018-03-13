/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"




%%{
	machine erract;

	action err_start {fmt.Print( "err_start\n" );}
	action err_all {fmt.Print( "err_all\n" );}
	action err_middle {fmt.Print( "err_middle\n" );}
	action err_out {fmt.Print( "err_out\n" );}

	action eof_start {fmt.Print( "eof_start\n" );}
	action eof_all {fmt.Print( "eof_all\n" );}
	action eof_middle {fmt.Print( "eof_middle\n" );}
	action eof_out {fmt.Print( "eof_out\n" );}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
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
	var eof int = pe
	%% write exec;
}
func finish() {
	if cs >= erract_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"",
"h",
"x",
"he",
"hx",
"hel",
"hex",
"hell",
"helx",
"hello",
"hellx",
"hello\n",
"hellox",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
