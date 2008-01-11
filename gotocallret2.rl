/*
 * @LANG: indep
 */

char comm;
int top;
int stack[32];
ptr ts;
ptr te;
int act;
int val;
%%
%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{ 
		prints "handle ";
		fhold; 
		if ( val == 1 ) fnext *fentry(one); 
		if ( val == 2 ) fnext *fentry(two); 
		if ( val == 3 ) fnext main;
	};

	one := |*
		'{' => { prints "{ "; fcall *fentry(one); };
		"[" => { prints "[ "; fcall *fentry(two); };
		"}" sp* => { prints "} "; fret; };
		[a-z]+ => { prints "word "; val = 1; fgoto *fentry(handle); };
		' ' => { prints "space "; };
	*|;

	two := |*
		'{' => { prints "{ "; fcall *fentry(one); };
		"[" => { prints "[ "; fcall *fentry(two); };
		']' sp* => { prints "] "; fret; };
		[a-z]+ => { prints "word "; val = 2; fgoto *fentry(handle); };
		' ' => { prints "space "; };
	*|;

	main := |* 
		'{' => { prints "{ "; fcall one; };
		"[" => { prints "[ "; fcall two; };
		[a-z]+ => { prints "word "; val = 3; fgoto handle; };
		[a-z] ' foil' => { prints "this is the foil";};
		' ' => { prints "space "; };
		'\n';
	*|;
}%%
/* _____INPUT_____
"{a{b[c d]d}c}\n"
"[a{b[c d]d}c}\n"
"[a[b]c]d{ef{g{h}i}j}l\n"
"{{[]}}\n"
"a b c\n"
"{a b c}\n"
"[a b c]\n"
"{]\n"
"{{}\n"
"[[[[[[]]]]]]\n"
"[[[[[[]]}]]]\n"
_____INPUT_____ */
/* _____OUTPUT_____
{ word handle { word handle [ word handle space word handle ] word handle } word handle } ACCEPT
[ word handle { word handle [ word handle space word handle ] word handle } word handle FAIL
[ word handle [ word handle ] word handle ] word handle { word handle { word handle { word handle } word handle } word handle } word handle ACCEPT
{ { [ ] } } ACCEPT
word handle space word handle space word handle ACCEPT
{ word handle space word handle space word handle } ACCEPT
[ word handle space word handle space word handle ] ACCEPT
{ FAIL
{ { } FAIL
[ [ [ [ [ [ ] ] ] ] ] ] ACCEPT
[ [ [ [ [ [ ] ] FAIL
_____OUTPUT_____ */

