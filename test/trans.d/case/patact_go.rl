/*
 * @LANG: go
 * @GENERATED: true
 */

package main
import "fmt"

var comm  byte ;
var top  int ;
var stack [32] int ;
var ts  int ;
var te  int ;
var act  int ;
var value  int ;

%%{
	machine patact;

	other := |* 
		[a-z]+ => {fmt.Print( "word\n" );};
		[0-9]+ => {fmt.Print( "num\n" );};
		[\n ] => {fmt.Print( "space\n" );};
	*|;

	exec_test := |* 
		[a-z]+ => {fmt.Print( "word (w/lbh)\n" );fexec te-1; 
fgoto other; 
};
		[a-z]+ ' foil' => {fmt.Print( "word (c/lbh)\n" );};
		[\n ] => {fmt.Print( "space\n" );};
		'22' => {fmt.Print( "num (w/switch)\n" );};
		[0-9]+ => {fmt.Print( "num (w/switch)\n" );fexec te-1; 
fgoto other;
};
		[0-9]+ ' foil' => {fmt.Print( "num (c/switch)\n" );};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {fmt.Print( "in semi\n" );fgoto main; 
};
	*|;

	main := |* 
		[a-z]+ => {fmt.Print( "word (w/lbh)\n" );fhold; 
fgoto other; 
};
		[a-z]+ ' foil' => {fmt.Print( "word (c/lbh)\n" );};
		[\n ] => {fmt.Print( "space\n" );};
		'22' => {fmt.Print( "num (w/switch)\n" );};
		[0-9]+ => {fmt.Print( "num (w/switch)\n" );fhold; 
fgoto other;
};
		[0-9]+ ' foil' => {fmt.Print( "num (c/switch)\n" );};
		';' => {fmt.Print( "going to semi\n" );fhold; 
fgoto semi;
};
		'!' => {fmt.Print( "immdiate\n" );fgoto exec_test; 
};
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
	if cs >= patact_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"abcd foix\n",
"abcd\nanother\n",
"123 foix\n",
"!abcd foix\n",
"!abcd\nanother\n",
"!123 foix\n",
";",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
