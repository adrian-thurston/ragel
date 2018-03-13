/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var i  int ;
var j  int ;
var k  int ;

%%{
	machine foo;

	action c1 {i != 0}
	action c2 {j != 0}
	action c3 {k != 0}
	action one {fmt.Print( "  one\n" );}
	action two {fmt.Print( "  two\n" );}
	action three {fmt.Print( "  three\n" );}

	action seti {if ( fc == 48 ) {
	i = 0;

} else {
	i = 1;

}
}
	action setj {if ( fc == 48 ) {
	j = 0;

} else {
	j = 1;

}
}
	action setk {if ( fc == 48 ) {
	k = 0;

} else {
	k = 1;

}
}

	action break {fnbreak;
}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
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
"000abc\n",
"100abc\n",
"010abc\n",
"110abc\n",
"001abc\n",
"101abc\n",
"011abc\n",
"111abc\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
