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
		'a' => {fmt.Print( "on last     " );if ( p + 1 == te ) {
	fmt.Print( "yes" );
}
fmt.Print( "\n" );};

		'b'+ => {fmt.Print( "on next     " );if ( p + 1 == te ) {
	fmt.Print( "yes" );
}
fmt.Print( "\n" );};

		'c1' 'dxxx'? => {fmt.Print( "on lag      " );if ( p + 1 == te ) {
	fmt.Print( "yes" );
}
fmt.Print( "\n" );};

		'd1' => {fmt.Print( "lm switch1  " );if ( p + 1 == te ) {
	fmt.Print( "yes" );
}
fmt.Print( "\n" );};
		'd2' => {fmt.Print( "lm switch2  " );if ( p + 1 == te ) {
	fmt.Print( "yes" );
}
fmt.Print( "\n" );};

		[d0-9]+ '.';

		'\n';
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
"abbc1d1d2\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
