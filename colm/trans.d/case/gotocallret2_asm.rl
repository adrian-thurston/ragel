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
	.comm	val,8,8
	.text

%%{
	machine GotoCallRet;

	sp = ' ';

	handle := any @{
		.section	.rodata
1:
	.string		"handle "
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fhold; 
		
	movq	val (%rip), %rax
	pushq	%rax
	movq	$1 , %rax
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
	movq	$fentry(one), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fnext	* %rcx;
2:
	movq	val (%rip), %rax
	pushq	%rax
	movq	$2 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	sete	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		4f
	movq	$fentry(two), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fnext	* %rcx;
4:
	movq	val (%rip), %rax
	pushq	%rax
	movq	$3 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	sete	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		6f
fnext main; 
6:

};

	one := |*
		'{' => {
		.section	.rodata
8:
	.string		"{ "
	.text
	movq	$8b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$fentry(one), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fcall	* %rcx;

};
		"[" => {
		.section	.rodata
9:
	.string		"[ "
	.text
	movq	$9b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$fentry(two), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fcall	* %rcx;

};
		"}" sp* => {
		.section	.rodata
10:
	.string		"} "
	.text
	movq	$10b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fret; 

};
		[a-z]+ => {
		.section	.rodata
11:
	.string		"word "
	.text
	movq	$11b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$1, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, val (%rip)
	movq	$fentry(handle), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fgoto	* %rcx;

};
		' ' => {
		.section	.rodata
12:
	.string		"space "
	.text
	movq	$12b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	*|;

	two := |*
		'{' => {
		.section	.rodata
13:
	.string		"{ "
	.text
	movq	$13b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$fentry(one), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fcall	* %rcx;

};
		"[" => {
		.section	.rodata
14:
	.string		"[ "
	.text
	movq	$14b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$fentry(two), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fcall	* %rcx;

};
		']' sp* => {
		.section	.rodata
15:
	.string		"] "
	.text
	movq	$15b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fret; 

};
		[a-z]+ => {
		.section	.rodata
16:
	.string		"word "
	.text
	movq	$16b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$2, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, val (%rip)
	movq	$fentry(handle), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fgoto	* %rcx;

};
		' ' => {
		.section	.rodata
17:
	.string		"space "
	.text
	movq	$17b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
	*|;

	main := |* 
		'{' => {
		.section	.rodata
18:
	.string		"{ "
	.text
	movq	$18b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fcall one; 

};
		"[" => {
		.section	.rodata
19:
	.string		"[ "
	.text
	movq	$19b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fcall two; 

};
		[a-z]+ => {
		.section	.rodata
20:
	.string		"word "
	.text
	movq	$20b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	$3, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, val (%rip)
fgoto handle; 

};
		[a-z] ' foil' => {
		.section	.rodata
21:
	.string		"this is the foil"
	.text
	movq	$21b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
		' ' => {
		.section	.rodata
22:
	.string		"space "
	.text
	movq	$22b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};
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
	movq	GotoCallRet_first_final(%rip), %rax
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
	.string	"{a{b[c d]d}c}\n"
.L_inp_1:
	.string	"[a{b[c d]d}c}\n"
.L_inp_2:
	.string	"[a[b]c]d{ef{g{h}i}j}l\n"
.L_inp_3:
	.string	"{{[]}}\n"
.L_inp_4:
	.string	"a b c\n"
.L_inp_5:
	.string	"{a b c}\n"
.L_inp_6:
	.string	"[a b c]\n"
.L_inp_7:
	.string	"{]\n"
.L_inp_8:
	.string	"{{}\n"
.L_inp_9:
	.string	"[[[[[[]]]]]]\n"
.L_inp_10:
	.string	"[[[[[[]]}]]]\n"

	.align 8
inp:
	.quad	.L_inp_0
	.quad	.L_inp_1
	.quad	.L_inp_2
	.quad	.L_inp_3
	.quad	.L_inp_4
	.quad	.L_inp_5
	.quad	.L_inp_6
	.quad	.L_inp_7
	.quad	.L_inp_8
	.quad	.L_inp_9
	.quad	.L_inp_10

	.align 8
inplen:
	.quad 11

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
	
