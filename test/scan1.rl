/*
 * @LANG: indep
 */
ptr ts;
ptr te;
int act;
int token;
%%
%%{
	machine scanner;

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => { 
			prints "on last     ";
			if ( p+1 == te )
				prints "yes";
			prints "\n";
		};

		'b'+ => {
			prints "on next     ";
			if ( p+1 == te )
				prints "yes";
			prints "\n";
		};

		'c1' 'dxxx'? => {
			prints "on lag      ";
			if ( p+1 == te )
				prints "yes";
			prints "\n";
		};

		'd1' => {
			prints "lm switch1  ";
			if ( p+1 == te )
				prints "yes";
			prints "\n";
		};
		'd2' => {
			prints "lm switch2  ";
			if ( p+1 == te )
				prints "yes";
			prints "\n";
		};

		[d0-9]+ '.';

		'\n';
	*|;
}%%
/* _____INPUT_____
"abbc1d1d2\n"
_____INPUT_____ */
/* _____OUTPUT_____
on last     yes
on next     yes
on lag      yes
lm switch1  yes
lm switch2  yes
ACCEPT
_____OUTPUT_____ */
