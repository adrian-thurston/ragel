#
# @LANG: asm
# @GENERATED: true
#

	.section	.data
	.comm	i,8,8
	.text
	.section	.data
	.comm	j,8,8
	.text
	.section	.data
	.comm	k,8,8
	.text

%%{
	machine foo;

	action c1 {
		movq	i (%rip), %rax
	pushq	%rax
	movq	$0, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	setne	%al
	movsbq	%al, %rax
	pushq	%rax

	popq	%rax
}
	action c2 {
		movq	j (%rip), %rax
	pushq	%rax
	movq	$0, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	setne	%al
	movsbq	%al, %rax
	pushq	%rax

	popq	%rax
}
	action c3 {
		movq	k (%rip), %rax
	pushq	%rax
	movq	$0, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	setne	%al
	movsbq	%al, %rax
	pushq	%rax

	popq	%rax
}
	action one {
		.section	.rodata
1:
	.string		"  one\n"
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action two {
		.section	.rodata
2:
	.string		"  two\n"
	.text
	movq	$2b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action three {
		.section	.rodata
3:
	.string		"  three\n"
	.text
	movq	$3b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}

	action seti {
		movsbq	(%r12), %rax
	pushq	%rax
	movq	$48 , %rax
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
	movq	$0, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, i (%rip)
	jmp		5f
4:
	movq	$1, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, i (%rip)
5:

}
	action setj {
		movsbq	(%r12), %rax
	pushq	%rax
	movq	$48 , %rax
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
	movq	$0, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, j (%rip)
	jmp		7f
6:
	movq	$1, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, j (%rip)
7:

}
	action setk {
		movsbq	(%r12), %rax
	pushq	%rax
	movq	$48 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	sete	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		8f
	movq	$0, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, k (%rip)
	jmp		9f
8:
	movq	$1, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, k (%rip)
9:

}

	action break {
	fnbreak;

}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
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

	%% write init;
	%% write exec;

	# current state is in r11.
	movq	foo_first_final(%rip), %rax
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
	.string	"000abc\n"
.L_inp_1:
	.string	"100abc\n"
.L_inp_2:
	.string	"010abc\n"
.L_inp_3:
	.string	"110abc\n"
.L_inp_4:
	.string	"001abc\n"
.L_inp_5:
	.string	"101abc\n"
.L_inp_6:
	.string	"011abc\n"
.L_inp_7:
	.string	"111abc\n"

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

	.align 8
inplen:
	.quad 8

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
	
