#
# @LANG: asm
# @GENERATED: true
#

	.section	.data
	.comm	top,8,8
	.text
	.section	.data
	.comm	stack,8,8
	.text
	.section	.data
	.comm	target,8,8
	.text

%%{
	machine ncall1;

	unused := 'unused';

	one := 'one' @{
		.section	.rodata
1:
	.string		"one\n"
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fnret;
	

};

	two := 'two' @{
		.section	.rodata
2:
	.string		"two\n"
	.text
	movq	$2b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
fnret;
	

};

	main := (
			'1' @{
		movq	$fentry(one), %rax
	pushq	%rax
	popq	%rax
	movq	%rax, target (%rip)
	movq	target(%rip), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fncall	* %rcx;

}
		|	'2' @{
		movq	$fentry(two), %rax
	pushq	%rax
	popq	%rax
	movq	%rax, target (%rip)
	movq	target(%rip), %rax
	pushq	%rax
movq	$0, %rax
popq	%rcx
fncall	* %rcx;

}
		|	'\n'
	)*;
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
	movq	ncall1_first_final(%rip), %rax
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
	.string	"1one2two1one\n"

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
	
