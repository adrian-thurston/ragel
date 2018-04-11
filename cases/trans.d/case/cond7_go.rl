/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var i  int ;
var c  int ;

%%{
	machine foo;

	action testi {i > 0}
	action inc {i = i - 1;
c = ( int ) ( fc )
;
fmt.Print( "item: " );fmt.Print( c );fmt.Print( "\n" );}

	count = [0-9] @{i = ( int ) ( fc - 48 )
;
fmt.Print( "count: " );fmt.Print( i );fmt.Print( "\n" );};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
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
	if cs >= foo_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"00\n",
"019\n",
"190\n",
"1719\n",
"1040000\n",
"104000a\n",
"104000\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
