#
# @LANG: asm
# @GENERATED: true
#

	.section	.data
	.comm	pos,8,8
	.text
	.section	.data
	.comm	line,8,8
	.text

%%{
	machine clang;

	# Function to buffer a character.
	action bufChar {
		movq	bpos(%rip), %rax
	movb	(%r12), %cl
	movb	%cl, buf(%rax)
	addq	$1, %rax
	movb	$0,	buf(%rax)
	movq	%rax, bpos(%rip)

}

	# Function to clear the buffer.
	action clearBuf {
		movq	$0, bpos(%rip)

}

	action incLine {
		movq	line (%rip), %rax
	pushq	%rax
	movq	$1, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	addq	%rcx, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, line (%rip)

}

	# Functions to dump tokens as they are matched.
	action ident {
		.section	.rodata
1:
	.string		"ident("
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	line(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
2:
	.string		","
	.text
	movq	$2b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	bpos(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
3:
	.string		"): "
	.text
	movq	$3b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$buf, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
4:
	.string		"\n"
	.text
	movq	$4b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action literal {
		.section	.rodata
5:
	.string		"literal("
	.text
	movq	$5b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	line(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
6:
	.string		","
	.text
	movq	$6b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	bpos(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
7:
	.string		"): "
	.text
	movq	$7b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$buf, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
8:
	.string		"\n"
	.text
	movq	$8b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action float {
		.section	.rodata
9:
	.string		"float("
	.text
	movq	$9b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	line(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
10:
	.string		","
	.text
	movq	$10b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	bpos(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
11:
	.string		"): "
	.text
	movq	$11b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$buf, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
12:
	.string		"\n"
	.text
	movq	$12b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action integer {
		.section	.rodata
13:
	.string		"int("
	.text
	movq	$13b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	line(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
14:
	.string		","
	.text
	movq	$14b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	bpos(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
15:
	.string		"): "
	.text
	movq	$15b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$buf, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
16:
	.string		"\n"
	.text
	movq	$16b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action hex {
		.section	.rodata
17:
	.string		"hex("
	.text
	movq	$17b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	line(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
18:
	.string		","
	.text
	movq	$18b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	bpos(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
19:
	.string		"): "
	.text
	movq	$19b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$buf, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
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

}
	action symbol {
		.section	.rodata
21:
	.string		"symbol("
	.text
	movq	$21b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	line(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
22:
	.string		","
	.text
	movq	$22b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	bpos(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
23:
	.string		"): "
	.text
	movq	$23b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$buf, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
24:
	.string		"\n"
	.text
	movq	$24b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}

	# Alpha numberic characters or underscore.
	alnumu = alnum | '_';

	# Alpha charactres or underscore.
	alphau = alpha | '_';

	# Symbols. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving dump the symbol.
	symbol = ( punct - [_'"] ) >clearBuf $bufChar %symbol;

	# Identifier. Upon entering clear the buffer. On all transitions
	# buffer a character. Upon leaving, dump the identifier.
	ident = (alphau . alnumu*) >clearBuf $bufChar %ident;

	# Match single characters inside literal strings. Or match 
	# an escape sequence. Buffers the charater matched.
	sliteralChar =
			( extend - ['\\] ) @bufChar |
			( '\\' . extend @bufChar );
	dliteralChar =
			( extend - ["\\] ) @bufChar |
			( '\\' . extend @bufChar );

	# Single quote and double quota literals. At the start clear
	# the buffer. Upon leaving dump the literal.
	sliteral = ('\'' @clearBuf . sliteralChar* . '\'' ) %literal;
	dliteral = ('"' @clearBuf . dliteralChar* . '"' ) %literal;
	literal = sliteral | dliteral;

	# Whitespace is standard ws, newlines and control codes.
	whitespace = any - 33 .. 126;

	# Describe both c style comments and c++ style comments. The
	# priority bump on tne terminator of the comments brings us
	# out of the extend* which matches everything.
	ccComment = '//' . extend* $0 . '\n' @1;
	cComment = '/!' . extend* $0 . '!/' @1;

	# Match an integer. We don't bother clearing the buf or filling it.
	# The float machine overlaps with int and it will do it.
	integer = digit+ %integer;

	# Match a float. Upon entering the machine clear the buf, buffer
	# characters on every trans and dump the float upon leaving.
	float =  ( digit+ . '.' . digit+ ) >clearBuf $bufChar %float;

	# Match a hex. Upon entering the hex part, clear the buf, buffer characters
	# on every trans and dump the hex on leaving transitions.
	hex = '0x' . xdigit+ >clearBuf $bufChar %hex;

	# Or together all the lanuage elements.
	fin = ( ccComment |
		cComment |
		symbol |
		ident |
		literal |
		whitespace |
		integer |
		float |
		hex );

	# Star the language elements. It is critical in this type of application
	# that we decrease the priority of out transitions before doing so. This
	# is so that when we see 'aa' we stay in the fin machine to match an ident
	# of length two and not wrap around to the front to match two idents of 
	# length one.
	clang_main = ( fin $1 %0 )*;

	# This machine matches everything, taking note of newlines.
	newline = ( any | '\n' @incLine )*;

	# The final fsm is the lexer intersected with the newline machine which
	# will count lines for us. Since the newline machine accepts everything,
	# the strings accepted is goverened by the clang_main machine, onto which
	# the newline machine overlays line counting.
	main := clang_main & newline;
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
	movq	$0, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, pos (%rip)
	movq	$1, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, line (%rip)

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
	movq	clang_first_final(%rip), %rax
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
	.string	"999 0xaAFF99 99.99 /!\n!/ 'lksdj' //\n\"\n\nliteral\n\n\n\"0x00aba foobardd.ddsf 0x0.9\n"
.L_inp_1:
	.string	"wordwithnum00asdf\n000wordfollowsnum,makes new symbol\n\nfinishing early /! unfinished ...\n"

	.align 8
inp:
	.quad	.L_inp_0
	.quad	.L_inp_1

	.align 8
inplen:
	.quad 2

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
	
