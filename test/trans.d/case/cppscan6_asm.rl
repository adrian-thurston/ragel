#
# @LANG: asm
# @GENERATED: true
#

	.section	.data
	.comm	ts,8,8
	.text
	.section	.data
	.comm	te,8,8
	.text
	.section	.data
	.comm	act,8,8
	.text
	.section	.data
	.comm	token,8,8
	.text

%%{
	machine scanner;

	action comment {
		movq	$242, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
1:
	.string		"<"
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
2:
	.string		"> "
	.text
	movq	$2b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
3:
	cmp		$0, %rcx
	je		4f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		3b
4:

	.section	.rodata
5:
	.string		"\n"
	.text
	movq	$5b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}


	main := |*

	# Single and double literals.
	( 'L'? "'" ( [^'\\\n] | '\\' any )* "'" ) 
		=> {
		movq	$193, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
6:
	.string		"<"
	.text
	movq	$6b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
7:
	.string		"> "
	.text
	movq	$7b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
8:
	cmp		$0, %rcx
	je		9f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		8b
9:

	.section	.rodata
10:
	.string		"\n"
	.text
	movq	$10b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	( 'L'? '"' ( [^"\\\n] | '\\' any )* '"' ) 
		=> {
		movq	$192, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
11:
	.string		"<"
	.text
	movq	$11b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
12:
	.string		"> "
	.text
	movq	$12b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
13:
	cmp		$0, %rcx
	je		14f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		13b
14:

	.section	.rodata
15:
	.string		"\n"
	.text
	movq	$15b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Identifiers
	( [a-zA-Z_] [a-zA-Z0-9_]* ) 
		=>{
		movq	$195, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
16:
	.string		"<"
	.text
	movq	$16b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
17:
	.string		"> "
	.text
	movq	$17b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
18:
	cmp		$0, %rcx
	je		19f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		18b
19:

	.section	.rodata
20:
	.string		"\n"
	.text
	movq	$20b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];

	( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) 
		=> {
		movq	$194, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
21:
	.string		"<"
	.text
	movq	$21b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
22:
	.string		"> "
	.text
	movq	$22b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
23:
	cmp		$0, %rcx
	je		24f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		23b
24:

	.section	.rodata
25:
	.string		"\n"
	.text
	movq	$25b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	
	# Integer decimal. Leading part buffered by float.
	( ( '0' | [1-9] [0-9]* ) [ulUL]? ) 
		=> {
		movq	$218, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
26:
	.string		"<"
	.text
	movq	$26b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
27:
	.string		"> "
	.text
	movq	$27b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
28:
	cmp		$0, %rcx
	je		29f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		28b
29:

	.section	.rodata
30:
	.string		"\n"
	.text
	movq	$30b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Integer octal. Leading part buffered by float.
	( '0' [0-9]+ [ulUL]? ) 
		=> {
		movq	$219, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
31:
	.string		"<"
	.text
	movq	$31b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
32:
	.string		"> "
	.text
	movq	$32b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
33:
	cmp		$0, %rcx
	je		34f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		33b
34:

	.section	.rodata
35:
	.string		"\n"
	.text
	movq	$35b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Integer hex. Leading 0 buffered by float.
	( '0' ( 'x' [0-9a-fA-F]+ [ulUL]? ) ) 
		=> {
		movq	$220, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
36:
	.string		"<"
	.text
	movq	$36b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
37:
	.string		"> "
	.text
	movq	$37b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
38:
	cmp		$0, %rcx
	je		39f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		38b
39:

	.section	.rodata
40:
	.string		"\n"
	.text
	movq	$40b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Only buffer the second item, first buffered by symbol.
	'::' => {
		movq	$197, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
41:
	.string		"<"
	.text
	movq	$41b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
42:
	.string		"> "
	.text
	movq	$42b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
43:
	cmp		$0, %rcx
	je		44f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		43b
44:

	.section	.rodata
45:
	.string		"\n"
	.text
	movq	$45b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'==' => {
		movq	$223, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
46:
	.string		"<"
	.text
	movq	$46b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
47:
	.string		"> "
	.text
	movq	$47b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
48:
	cmp		$0, %rcx
	je		49f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		48b
49:

	.section	.rodata
50:
	.string		"\n"
	.text
	movq	$50b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'!=' => {
		movq	$224, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
51:
	.string		"<"
	.text
	movq	$51b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
52:
	.string		"> "
	.text
	movq	$52b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
53:
	cmp		$0, %rcx
	je		54f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		53b
54:

	.section	.rodata
55:
	.string		"\n"
	.text
	movq	$55b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'&&' => {
		movq	$225, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
56:
	.string		"<"
	.text
	movq	$56b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
57:
	.string		"> "
	.text
	movq	$57b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
58:
	cmp		$0, %rcx
	je		59f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		58b
59:

	.section	.rodata
60:
	.string		"\n"
	.text
	movq	$60b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'||' => {
		movq	$226, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
61:
	.string		"<"
	.text
	movq	$61b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
62:
	.string		"> "
	.text
	movq	$62b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
63:
	cmp		$0, %rcx
	je		64f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		63b
64:

	.section	.rodata
65:
	.string		"\n"
	.text
	movq	$65b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'*=' => {
		movq	$227, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
66:
	.string		"<"
	.text
	movq	$66b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
67:
	.string		"> "
	.text
	movq	$67b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
68:
	cmp		$0, %rcx
	je		69f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		68b
69:

	.section	.rodata
70:
	.string		"\n"
	.text
	movq	$70b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'/=' => {
		movq	$228, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
71:
	.string		"<"
	.text
	movq	$71b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
72:
	.string		"> "
	.text
	movq	$72b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
73:
	cmp		$0, %rcx
	je		74f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		73b
74:

	.section	.rodata
75:
	.string		"\n"
	.text
	movq	$75b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'%=' => {
		movq	$229, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
76:
	.string		"<"
	.text
	movq	$76b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
77:
	.string		"> "
	.text
	movq	$77b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
78:
	cmp		$0, %rcx
	je		79f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		78b
79:

	.section	.rodata
80:
	.string		"\n"
	.text
	movq	$80b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'+=' => {
		movq	$230, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
81:
	.string		"<"
	.text
	movq	$81b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
82:
	.string		"> "
	.text
	movq	$82b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
83:
	cmp		$0, %rcx
	je		84f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		83b
84:

	.section	.rodata
85:
	.string		"\n"
	.text
	movq	$85b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'-=' => {
		movq	$231, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
86:
	.string		"<"
	.text
	movq	$86b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
87:
	.string		"> "
	.text
	movq	$87b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
88:
	cmp		$0, %rcx
	je		89f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		88b
89:

	.section	.rodata
90:
	.string		"\n"
	.text
	movq	$90b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'&=' => {
		movq	$232, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
91:
	.string		"<"
	.text
	movq	$91b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
92:
	.string		"> "
	.text
	movq	$92b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
93:
	cmp		$0, %rcx
	je		94f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		93b
94:

	.section	.rodata
95:
	.string		"\n"
	.text
	movq	$95b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'^=' => {
		movq	$233, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
96:
	.string		"<"
	.text
	movq	$96b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
97:
	.string		"> "
	.text
	movq	$97b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
98:
	cmp		$0, %rcx
	je		99f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		98b
99:

	.section	.rodata
100:
	.string		"\n"
	.text
	movq	$100b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'|=' => {
		movq	$234, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
101:
	.string		"<"
	.text
	movq	$101b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
102:
	.string		"> "
	.text
	movq	$102b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
103:
	cmp		$0, %rcx
	je		104f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		103b
104:

	.section	.rodata
105:
	.string		"\n"
	.text
	movq	$105b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'++' => {
		movq	$212, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
106:
	.string		"<"
	.text
	movq	$106b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
107:
	.string		"> "
	.text
	movq	$107b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
108:
	cmp		$0, %rcx
	je		109f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		108b
109:

	.section	.rodata
110:
	.string		"\n"
	.text
	movq	$110b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'--' => {
		movq	$213, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
111:
	.string		"<"
	.text
	movq	$111b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
112:
	.string		"> "
	.text
	movq	$112b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
113:
	cmp		$0, %rcx
	je		114f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		113b
114:

	.section	.rodata
115:
	.string		"\n"
	.text
	movq	$115b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'->' => {
		movq	$211, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
116:
	.string		"<"
	.text
	movq	$116b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
117:
	.string		"> "
	.text
	movq	$117b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
118:
	cmp		$0, %rcx
	je		119f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		118b
119:

	.section	.rodata
120:
	.string		"\n"
	.text
	movq	$120b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'->*' => {
		movq	$214, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
121:
	.string		"<"
	.text
	movq	$121b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
122:
	.string		"> "
	.text
	movq	$122b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
123:
	cmp		$0, %rcx
	je		124f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		123b
124:

	.section	.rodata
125:
	.string		"\n"
	.text
	movq	$125b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	'.*' => {
		movq	$215, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
126:
	.string		"<"
	.text
	movq	$126b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
127:
	.string		"> "
	.text
	movq	$127b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
128:
	cmp		$0, %rcx
	je		129f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		128b
129:

	.section	.rodata
130:
	.string		"\n"
	.text
	movq	$130b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Three char compounds, first item already buffered.
	'...' => {
		movq	$240, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
131:
	.string		"<"
	.text
	movq	$131b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
132:
	.string		"> "
	.text
	movq	$132b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
133:
	cmp		$0, %rcx
	je		134f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		133b
134:

	.section	.rodata
135:
	.string		"\n"
	.text
	movq	$135b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Single char symbols.
	( punct - [_"'] ) => {
		movq	-16(%rbp), %rax
	movsbq	(%rax), %rcx
	pushq	%rcx
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
136:
	.string		"<"
	.text
	movq	$136b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
137:
	.string		"> "
	.text
	movq	$137b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
138:
	cmp		$0, %rcx
	je		139f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		138b
139:

	.section	.rodata
140:
	.string		"\n"
	.text
	movq	$140b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Comments and whitespace.
	'/!' ( any* $0 '!/' @1 ) => comment;
	'//' ( any* $0 '\n' @1 ) => comment;
	( any - 33..126 )+ => {
		movq	$241, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, token (%rip)
	.section	.rodata
141:
	.string		"<"
	.text
	movq	$141b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	token(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
142:
	.string		"> "
	.text
	movq	$142b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-16(%rbp), %rax   # ts
	movq	-24(%rbp), %rcx   # te
	subq	%rax, %rcx        # length
143:
	cmp		$0, %rcx
	je		144f
	movsbl	(%rax), %edi
	push	%rax
	push	%rcx
	call	putchar	
	pop		%rcx
	pop		%rax
	addq	$1, %rax
	subq	$1, %rcx
	jmp		143b
144:

	.section	.rodata
145:
	.string		"\n"
	.text
	movq	$145b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	*|;
}%%



	.text
	.comm		buf, 1024, 32
	.comm		bpos, 8, 8
	.comm		stack_data, 1024, 32
	.section	.rodata

.L_str_accept:
	.string "ACCEPT"
.L_str_fail:
	.string "FAIL"
.L_fmt_int:
	.string "%ld"
.L_fmt_str:
	.string "%s"

	%% write data;

	.text
exec:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$96, %rsp

	movq	$stack_data, -56(%rbp)

	pushq	%r12
	pushq	%r13
	movq	%rdi, %r12

	# Get the length.
	movq	%r12, %rdi
	call	strlen
	movq	%r12, %r13
	movslq	%eax, %rax
	addq	%rax, %r13

	movq	$0,	bpos(%rip)
	movq	%r13, -8(%rbp)

	%% write init;
	%% write exec;

	# current state is in r11.
	movq	scanner_first_final(%rip), %rax
	cmpq	%rax, %r11
	jl		.L_exec_fail
	movq	$.L_str_accept, %rdi
	call	puts
	jmp		.L_exec_done
.L_exec_fail:
	movq	$.L_str_fail, %rdi
	call	puts
.L_exec_done:
	popq	%r13
	popq	%r12
	leave
	ret
	.section	.rodata
.L_debug_msg:
	.string	"debug %d\n"
.L_inp_0:
	.string	"\"\\\"hi\" /!\n!/\n44 .44\n44. 44\n44 . 44\n44.44\n_hithere22"
.L_inp_1:
	.string	"'\\''\"\\n\\d'\\\"\"\nhi\n99\n.99\n99e-4\n->*\n||\n0x98\n0x\n//\n/! * !/"
.L_inp_2:
	.string	"'\n'\n"

	.align 8
inp:
	.quad	.L_inp_0
	.quad	.L_inp_1
	.quad	.L_inp_2

	.align 8
inplen:
	.quad 3

	.text
	.globl	main
main:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r12
	movq	$0, %r12
.L_again:
	movq	inplen(%rip), %rax
	cmpq	%r12, %rax
	je		.L_done
	movq	inp(,%r12,8), %rdi
	call	exec
	incq	%r12
	jmp		.L_again
.L_done:
	popq	%r12
	mov		$0, %rax
	leave
	ret
debug:
	movl	%edi, %esi
	movq	$0, %rax
	movq	$.L_debug_msg, %rdi
	call	printf
	ret
	
