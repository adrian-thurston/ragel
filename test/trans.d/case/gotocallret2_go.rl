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
var val  int ;

%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{fmt.Print( "handle " );fhold; 
		
if ( val == 1 ) {
	fnext *fentry(one); 

}
if ( val == 2 ) {
	fnext *fentry(two); 

}
if ( val == 3 ) {
	fnext main; 

}
};

	one := |*
		'{' => {fmt.Print( "{ " );fcall *fentry(one); 
};
		"[" => {fmt.Print( "[ " );fcall *fentry(two); 
};
		"}" sp* => {fmt.Print( "} " );fret; 
};
		[a-z]+ => {fmt.Print( "word " );val = 1;
fgoto *fentry(handle); 
};
		' ' => {fmt.Print( "space " );};
	*|;

	two := |*
		'{' => {fmt.Print( "{ " );fcall *fentry(one); 
};
		"[" => {fmt.Print( "[ " );fcall *fentry(two); 
};
		']' sp* => {fmt.Print( "] " );fret; 
};
		[a-z]+ => {fmt.Print( "word " );val = 2;
fgoto *fentry(handle); 
};
		' ' => {fmt.Print( "space " );};
	*|;

	main := |* 
		'{' => {fmt.Print( "{ " );fcall one; 
};
		"[" => {fmt.Print( "[ " );fcall two; 
};
		[a-z]+ => {fmt.Print( "word " );val = 3;
fgoto handle; 
};
		[a-z] ' foil' => {fmt.Print( "this is the foil" );};
		' ' => {fmt.Print( "space " );};
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
	if cs >= GotoCallRet_first_final {
		fmt.Println("ACCEPT")
	} else {
		fmt.Println("FAIL")
	}
}
var inp []string = []string {
"{a{b[c d]d}c}\n",
"[a{b[c d]d}c}\n",
"[a[b]c]d{ef{g{h}i}j}l\n",
"{{[]}}\n",
"a b c\n",
"{a b c}\n",
"[a b c]\n",
"{]\n",
"{{}\n",
"[[[[[[]]]]]]\n",
"[[[[[[]]}]]]\n",
};

func main() {
	for _, data := range inp {
		prepare()
		exec(data)
		finish()
	}
}
