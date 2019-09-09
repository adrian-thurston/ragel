/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var return_to  int ;

%%{
	machine curs1;

	unused := 'unused';

	one := 'one' @{fmt.Print( "one\n" );fnext *return_to;
	
};

	two := 'two' @{fmt.Print( "two\n" );fnext *return_to;
	
};

	main := 
		'1' @{return_to = fcurs;
fnext one; 
}
	|	'2' @{return_to = fcurs;
fnext two; 
}
	|	'\n';
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
	if cs >= curs1_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"1one2two1one\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
