#
# @LANG: asm
# @GENERATED: true
#

	.section	.data
	.comm	i,8,8
	.text
	.section	.data
	.comm	c,8,8
	.text

%%{
	machine foo;

	action testi {
		movq	i (%rip), %rax
	pushq	%rax
	movq	$0, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	pushq	%rax

	popq	%rax
}
	action inc {
		movq	i (%rip), %rax
	pushq	%rax
	movq	$1, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	subq	%rcx, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, i (%rip)
	movsbq	(%r12), %rax
	pushq	%rax
	popq	%rax
	movq	%rax, c (%rip)
	.section	.rodata
1:
	.string		"item: "
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	c(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	.section	.rodata
2:
	.string		"\n"
	.text
	movq	$2b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}

	count = [0-9] @{
		movsbq	(%r12), %rax
	pushq	%rax
	movq	$48, %rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	subq	%rcx, %rax
	pushq	%rax
	popq	%rax
	movq	%rax, i (%rip)
	.section	.rodata
3:
	.string		"count: "
	.text
	movq	$3b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf
	movq	i(%rip), %rax
	pushq	%rax
	movq	$.L_fmt_int, %rdi
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

};

    sub = 	
		count # record the number of digits 
		( digit when testi @inc )* outwhen !testi;
	
    main := sub sub '\n';
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
	.string	"00\n"
.L_inp_1:
	.string	"019\n"
.L_inp_2:
	.string	"190\n"
.L_inp_3:
	.string	"1719\n"
.L_inp_4:
	.string	"1040000\n"
.L_inp_5:
	.string	"104000a\n"
.L_inp_6:
	.string	"104000\n"

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
	
