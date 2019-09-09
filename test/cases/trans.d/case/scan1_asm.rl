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

	# Warning: changing the patterns or the input string will affect the
	# coverage of the scanner action types.
	main := |*
		'a' => {
		.section	.rodata
1:
	.string		"on last     "
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	pushq	%r12
	movq	$1 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	addq	%rcx, %rax
	pushq	%rax
	movq	-24(%rbp), %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	sete	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		2f
	.section	.rodata
4:
	.string		"yes"
	.text
	movq	$4b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
2:
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

};

		'b'+ => {
		.section	.rodata
6:
	.string		"on next     "
	.text
	movq	$6b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	pushq	%r12
	movq	$1 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	addq	%rcx, %rax
	pushq	%rax
	movq	-24(%rbp), %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	sete	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		7f
	.section	.rodata
9:
	.string		"yes"
	.text
	movq	$9b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
7:
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

		'c1' 'dxxx'? => {
		.section	.rodata
11:
	.string		"on lag      "
	.text
	movq	$11b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	pushq	%r12
	movq	$1 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	addq	%rcx, %rax
	pushq	%rax
	movq	-24(%rbp), %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	sete	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		12f
	.section	.rodata
14:
	.string		"yes"
	.text
	movq	$14b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
12:
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

		'd1' => {
		.section	.rodata
16:
	.string		"lm switch1  "
	.text
	movq	$16b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	pushq	%r12
	movq	$1 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	addq	%rcx, %rax
	pushq	%rax
	movq	-24(%rbp), %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	sete	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		17f
	.section	.rodata
19:
	.string		"yes"
	.text
	movq	$19b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
17:
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
		'd2' => {
		.section	.rodata
21:
	.string		"lm switch2  "
	.text
	movq	$21b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	pushq	%r12
	movq	$1 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	addq	%rcx, %rax
	pushq	%rax
	movq	-24(%rbp), %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	sete	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		22f
	.section	.rodata
24:
	.string		"yes"
	.text
	movq	$24b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
22:
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

		[d0-9]+ '.';

		'\n';
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
	.string	"abbc1d1d2\n"

	.align 8
inp:
	.quad	.L_inp_0

	.align 8
inplen:
	.quad 1

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
	
