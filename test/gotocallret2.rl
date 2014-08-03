/*
 * @LANG: indep
 * @NEEDS_EOF: yes
 * @PROHIBIT_GENFLAGS: -B0
 */

char comm;
int top;
int stack[32];
ptr ts;
ptr te;
int act;
int val;

%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{ 
		print_str "handle ";
		fhold; 
		if ( val == 1 ) { fnext *fentry(one); }
		if ( val == 2 ) { fnext *fentry(two); }
		if ( val == 3 ) { fnext main; }
	};

	one := |*
		'{' => { print_str "{ "; fcall *fentry(one); };
		"[" => { print_str "[ "; fcall *fentry(two); };
		"}" sp* => { print_str "} "; fret; };
		[a-z]+ => { print_str "word "; val = 1; fgoto *fentry(handle); };
		' ' => { print_str "space "; };
	*|;

	two := |*
		'{' => { print_str "{ "; fcall *fentry(one); };
		"[" => { print_str "[ "; fcall *fentry(two); };
		']' sp* => { print_str "] "; fret; };
		[a-z]+ => { print_str "word "; val = 2; fgoto *fentry(handle); };
		' ' => { print_str "space "; };
	*|;

	main := |* 
		'{' => { print_str "{ "; fcall one; };
		"[" => { print_str "[ "; fcall two; };
		[a-z]+ => { print_str "word "; val = 3; fgoto handle; };
		[a-z] ' foil' => { print_str "this is the foil";};
		' ' => { print_str "space "; };
		'\n';
	*|;
}%%

#ifdef _____INPUT_____
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
#endif

#ifdef _____OUTPUT_____
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
#endif
