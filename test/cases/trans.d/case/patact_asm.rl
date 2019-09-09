#
# @LANG: asm
# @GENERATED: true
#

	.section	.data
	.comm	comm,8,8
	.text
	.section	.data
	.comm	top,8,8
	.text
	.section	.data
	.comm	stack,8,8
	.text
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
	.comm	value,8,8
	.text

%%{
	machine patact;

	other := |* 
		[a-z]+ => {
		.section	.rodata
1:
	.string		"word\n"
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		[0-9]+ => {
		.section	.rodata
2:
	.string		"num\n"
	.text
	movq	$2b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		[\n ] => {
		.section	.rodata
3:
	.string		"space\n"
	.text
	movq	$3b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	*|;

	exec_test := |* 
		[a-z]+ => {
		.section	.rodata
4:
	.string		"word (w/lbh)\n"
	.text
	movq	$4b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-24(%rbp), %rax
	pushq	%rax
	movq	$1, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	subq	%rcx, %rax
	pushq	%rax
	popq	%rax
	fexec	%rax;
fgoto other; 

};
		[a-z]+ ' foil' => {
		.section	.rodata
5:
	.string		"word (c/lbh)\n"
	.text
	movq	$5b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		[\n ] => {
		.section	.rodata
6:
	.string		"space\n"
	.text
	movq	$6b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		'22' => {
		.section	.rodata
7:
	.string		"num (w/switch)\n"
	.text
	movq	$7b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		[0-9]+ => {
		.section	.rodata
8:
	.string		"num (w/switch)\n"
	.text
	movq	$8b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	-24(%rbp), %rax
	pushq	%rax
	movq	$1, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	subq	%rcx, %rax
	pushq	%rax
	popq	%rax
	fexec	%rax;
fgoto other;

};
		[0-9]+ ' foil' => {
		.section	.rodata
9:
	.string		"num (c/switch)\n"
	.text
	movq	$9b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		'!';# => { print_str "immdiate\n"; fgoto exec_test; };
	*|;

	semi := |* 
		';' => {
		.section	.rodata
10:
	.string		"in semi\n"
	.text
	movq	$10b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fgoto main; 

};
	*|;

	main := |* 
		[a-z]+ => {
		.section	.rodata
11:
	.string		"word (w/lbh)\n"
	.text
	movq	$11b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fhold; 
fgoto other; 

};
		[a-z]+ ' foil' => {
		.section	.rodata
12:
	.string		"word (c/lbh)\n"
	.text
	movq	$12b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		[\n ] => {
		.section	.rodata
13:
	.string		"space\n"
	.text
	movq	$13b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		'22' => {
		.section	.rodata
14:
	.string		"num (w/switch)\n"
	.text
	movq	$14b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		[0-9]+ => {
		.section	.rodata
15:
	.string		"num (w/switch)\n"
	.text
	movq	$15b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fhold; 
fgoto other;

};
		[0-9]+ ' foil' => {
		.section	.rodata
16:
	.string		"num (c/switch)\n"
	.text
	movq	$16b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		';' => {
		.section	.rodata
17:
	.string		"going to semi\n"
	.text
	movq	$17b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fhold; 
fgoto semi;

};
		'!' => {
		.section	.rodata
18:
	.string		"immdiate\n"
	.text
	movq	$18b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fgoto exec_test; 

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
	movq	patact_first_final(%rip), %rax
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
	.string	"abcd foix\n"
.L_inp_1:
	.string	"abcd\nanother\n"
.L_inp_2:
	.string	"123 foix\n"
.L_inp_3:
	.string	"!abcd foix\n"
.L_inp_4:
	.string	"!abcd\nanother\n"
.L_inp_5:
	.string	"!123 foix\n"
.L_inp_6:
	.string	";"

	.align 8
inp:
	.quad	.L_inp_0
	.quad	.L_inp_1
	.quad	.L_inp_2
	.quad	.L_inp_3
	.quad	.L_inp_4
	.quad	.L_inp_5
	.quad	.L_inp_6

	.align 8
inplen:
	.quad 7

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
	
