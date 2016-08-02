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

%%{
	machine GotoCallRet;

	# A reference to a state in an unused action caused a segfault in 5.8. */
	action unusedAction {
		movq	$fentry(garble_line), %rax
	pushq	%rax
	popq	%rax

}

	action err_garbling_line {
		.section	.rodata
1:
	.string		"error: garbling line\n"
	.text
	movq	$1b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}
	action goto_main {
	fgoto main; 

}
	action recovery_failed {
		.section	.rodata
2:
	.string		"error: failed to recover\n"
	.text
	movq	$2b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

}

	# Error machine, consumes to end of 
	# line, then starts the main line over.
	garble_line := ( (any-'\n')*'\n') 
		>err_garbling_line
		@goto_main
		$/recovery_failed;

	action hold_and_return {
	fhold; 
fret;

}

	# Look for a string of alphas or of digits, 
	# on anything else, hold the character and return.
	alp_comm := alpha+ $!hold_and_return;
	dig_comm := digit+ $!hold_and_return;

	# Choose which to machine to call into based on the command.
	action comm_arg {
		movq	comm (%rip), %rax
	pushq	%rax
	movq	$97,	%rax
	pushq	%rax
	popq	%rcx
	popq	%rax
	cmp		%rcx, %rax
	setge	%al
	movsbq	%al, %rax
	pushq	%rax
	popq	%rax
	test	%rax, %rax
	jz		3f
fcall alp_comm;
		
	jmp		4f
3:
fcall dig_comm;
		
4:

}

	# Specifies command string. Note that the arg is left out.
	command = (
		[a-z0-9] @{
		movsbq	(%r12), %rax
	pushq	%rax
	popq	%rax
	movq	%rax, comm (%rip)

} ' ' @comm_arg '\n'
	) @{
		.section	.rodata
5:
	.string		"correct command\n"
	.text
	movq	$5b, %rax
	pushq	%rax
	movq	$.L_fmt_str, %rdi
	popq	%rsi
	movq	$0, %rax
	call	printf

};

	# Any number of commands. If there is an 
	# error anywhere, garble the line.
	main := command* $!{
	fhold;
fgoto garble_line;

}; 
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
	.string	"lkajsdf\n"
.L_inp_1:
	.string	"2134\n"
.L_inp_2:
	.string	"(\n"
.L_inp_3:
	.string	"\n"
.L_inp_4:
	.string	"*234234()0909 092 -234aslkf09`1 11\n"
.L_inp_5:
	.string	"1\n"
.L_inp_6:
	.string	"909\n"
.L_inp_7:
	.string	"1 a\n"
.L_inp_8:
	.string	"11 1\n"
.L_inp_9:
	.string	"a 1\n"
.L_inp_10:
	.string	"aa a\n"
.L_inp_11:
	.string	"1 1\n"
.L_inp_12:
	.string	"1 123456\n"
.L_inp_13:
	.string	"a a\n"
.L_inp_14:
	.string	"a abcdef\n"
.L_inp_15:
	.string	"h"
.L_inp_16:
	.string	"a aa1"

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

	.align 8
inplen:
	.quad 17

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
	
