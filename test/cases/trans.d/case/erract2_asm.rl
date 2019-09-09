#
# @LANG: asm
# @GENERATED: true
#




%%{
	machine erract;

	action err_start {
		.section	.rodata
1:
	.string		"err_start\n"
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action err_all {
		.section	.rodata
2:
	.string		"err_all\n"
	.text
	movq	$2b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action err_middle {
		.section	.rodata
3:
	.string		"err_middle\n"
	.text
	movq	$3b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action err_out {
		.section	.rodata
4:
	.string		"err_out\n"
	.text
	movq	$4b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}

	action eof_start {
		.section	.rodata
5:
	.string		"eof_start\n"
	.text
	movq	$5b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action eof_all {
		.section	.rodata
6:
	.string		"eof_all\n"
	.text
	movq	$6b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action eof_middle {
		.section	.rodata
7:
	.string		"eof_middle\n"
	.text
	movq	$7b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action eof_out {
		.section	.rodata
8:
	.string		"eof_out\n"
	.text
	movq	$8b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}

	main := ( 'hello' 
			>err err_start $err err_all <>err err_middle %err err_out
			>eof eof_start $eof eof_all <>eof eof_middle %eof eof_out
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
	movq	%r13, -8(%rbp)

	%% write init;
	%% write exec;

	# current state is in r11.
	movq	erract_first_final(%rip), %rax
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
	.string	""
.L_inp_1:
	.string	"h"
.L_inp_2:
	.string	"x"
.L_inp_3:
	.string	"he"
.L_inp_4:
	.string	"hx"
.L_inp_5:
	.string	"hel"
.L_inp_6:
	.string	"hex"
.L_inp_7:
	.string	"hell"
.L_inp_8:
	.string	"helx"
.L_inp_9:
	.string	"hello"
.L_inp_10:
	.string	"hellx"
.L_inp_11:
	.string	"hello\n"
.L_inp_12:
	.string	"hellox"

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

	.align 8
inplen:
	.quad 13

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
	
