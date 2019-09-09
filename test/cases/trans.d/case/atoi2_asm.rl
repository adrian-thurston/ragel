#
# @LANG: asm
# @GENERATED: true
#

	.section	.data
	.comm	neg,8,8
	.text
	.section	.data
	.comm	value,8,8
	.text

%%{
	machine state_chart;

	action begin {
		movq	$0, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, neg (%rip)
	movq	$0, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, value (%rip)

}

	action see_neg {
		movq	$1, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, neg (%rip)

}

	action add_digit {
		movq	value (%rip), %rax
	pushq	%rax
	movq	$10 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	imul	%rcx, %rax
	pushq	%rax
	movsbq	(%r12), %rax
	pushq	%rax
	movq	$48, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	subq	%rcx, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	addq	%rcx, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, value (%rip)

}

	action finish {
		movq	neg (%rip), %rax
	pushq	%rax
	movq	$0 , %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	setne	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		1f
	movq	$-1 , %rax
	pushq	%rax
	movq	value(%rip), %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	imul	%rcx, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, value (%rip)
1:

}

	atoi = (
		start: (
			'-' @see_neg ->om_num | 
			'+' ->om_num |
			[0-9] @add_digit ->more_nums
		),

		# One or more nums.
		om_num: (
			[0-9] @add_digit ->more_nums
		),

		# Zero ore more nums.
		more_nums: (
			[0-9] @add_digit ->more_nums |
			'' -> final
		)
	) >begin %finish;

	action oneof {
		movq	value(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
3:
	.string		"\n"
	.text
	movq	$3b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	main := ( atoi '\n' @oneof )*;
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
	movq	%rax, value (%rip)
	movq	$0, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, neg (%rip)

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
	movq	state_chart_first_final(%rip), %rax
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
	.string	"1\n"
.L_inp_1:
	.string	"12\n"
.L_inp_2:
	.string	"222222\n"
.L_inp_3:
	.string	"+2123\n"
.L_inp_4:
	.string	"213 3213\n"
.L_inp_5:
	.string	"-12321\n"
.L_inp_6:
	.string	"--123\n"
.L_inp_7:
	.string	"-99\n"
.L_inp_8:
	.string	" -3000\n"

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

	.align 8
inplen:
	.quad 9

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
	
