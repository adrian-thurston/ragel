#
# @LANG: asm
# @GENERATED: true
#



%%{
	machine indep;

	main := (
		( '1' 'hello' ) |
		( '2' "hello" ) |
		( '3' /hello/ ) |
		( '4' 'hello'i ) |
		( '5' "hello"i ) |
		( '6' /hello/i )
	) '\n';
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
	movq	indep_first_final(%rip), %rax
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
	.string	"1hello\n"
.L_inp_1:
	.string	"1HELLO\n"
.L_inp_2:
	.string	"1HeLLo\n"
.L_inp_3:
	.string	"2hello\n"
.L_inp_4:
	.string	"2HELLO\n"
.L_inp_5:
	.string	"2HeLLo\n"
.L_inp_6:
	.string	"3hello\n"
.L_inp_7:
	.string	"3HELLO\n"
.L_inp_8:
	.string	"3HeLLo\n"
.L_inp_9:
	.string	"4hello\n"
.L_inp_10:
	.string	"4HELLO\n"
.L_inp_11:
	.string	"4HeLLo\n"
.L_inp_12:
	.string	"5hello\n"
.L_inp_13:
	.string	"5HELLO\n"
.L_inp_14:
	.string	"5HeLLo\n"
.L_inp_15:
	.string	"6hello\n"
.L_inp_16:
	.string	"6HELLO\n"
.L_inp_17:
	.string	"6HeLLo\n"

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
	.quad	.L_inp_11
	.quad	.L_inp_12
	.quad	.L_inp_13
	.quad	.L_inp_14
	.quad	.L_inp_15
	.quad	.L_inp_16
	.quad	.L_inp_17

	.align 8
inplen:
	.quad 18

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
	
