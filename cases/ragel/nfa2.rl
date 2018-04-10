#
# @LANG: asm
# @FILTER: sort
#

%%{
	machine AtoI;

	action begin {
		# neg = 0;
		# val = 0;
		movl	$0, neg(%rip)
		movq	$0, val(%rip)
	}

	action see_neg {
		# neg = 1;
		movl	$1, neg(%rip)
	}

	action add_digit {
		# val = val * 10 + (fc - 48);
		movq	val(%rip), %rax
		imul	$10, %rax
		movsbq	(%r12), %rcx
		subq	$48, %rcx
		add		%rcx, %rax
		movq	%rax, val(%rip)
	}

	action finish {
		#if (neg)
		#{
		#	val = - 1 * val;
		#}

		movl	neg(%rip), %eax
		cmpl	$0, %eax
		je		.finish_L
		movq	val(%rip), %rax
		negq	%rax
		movq	%rax, val(%rip)
	.finish_L:
	}

	action print1 {
		#printf ("%ld", val);
		#fputs ("\n", stdout);
		movl	$.L_fmt_i_nl, %edi
		movq	val(%rip), %rsi
		movl	$0, %eax
		call	printf
	}

	action print2 {
		movl	$.L_saw, %edi
		call	puts
	}

	atoi = 
		(('-' @ see_neg | '+') ?
		(digit @ add_digit) +) > begin %finish;

	main1 = atoi '\n' @print1;
	main2 = [0-9]* '00000000' [0-9]* '\n' @print2;

	main |= (5, 0) main1 | main2;

}%%
	.file	"tmp.c"
	.comm	neg,4,4
	.comm	val,8,8
	.comm	cs,4,4
	.section	.rodata

%% write data;

.L_fmt_i_nl:
	.string "%ld\n"
.L_marker:
	.string "marker\n"
.L_saw:
	.string "saw 80808080"

	.text
	.globl	init
	.type	init, @function
init:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	$0, val(%rip)
	movl	$0, neg(%rip)

%% write init;

	movq    %r11, cs(%rip)

	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	init, .-init
	.section	.rodata
.LC0:
	.string	"%i"
	.text
	.globl	exec
	.type	exec, @function
exec:
.LFB1:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$96, %rsp

	pushq   %r12
	pushq   %r13

	movq	nfa_bp@GOTPCREL(%rip), %rax
	movq	%rax, -80(%rbp)
	movq	$0, -88(%rbp)
	
	movq    cs(%rip), %r11
	movq    %rdi, %r12
	movq    %rsi, %r13

%% write exec;

	movq    %r11, cs(%rip)

	popq	%r13
	popq	%r12

.LRET:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	exec, .-exec
	.section	.rodata
.LC1:
	.string	"-> ACCEPT"
.LC2:
	.string	"-> FAIL"
	.text
	.globl	finish
	.type	finish, @function
finish:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	cs@GOTPCREL (%rip), %rdx
	movl	(%rdx), %edx
	movl	$4, %eax
	cmpl	%eax, %edx
	jl	.L36
	movl	$.LC1, %edi
#	call	puts
	jmp	.L35
.L36:
	movl	$.LC2, %edi
	call	puts
.L35:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	finish, .-finish
	.globl	inp

	.section	.rodata
.LC3:
	.string	"1\n"
.LC4:
	.string	"12\n"
.LC45:
	.string "1002000000002\n",
.LC5:
	.string	"222222\n"
.LC6:
	.string	"+2123\n"
.LC7:
	.string	"-99\n"
.LC8:
	.string	"-12321\n"
.LC9:
	.string	"213 3213\n"
.LC10:
	.string	"--123\n"
.LC11:
	.string	" -3000\n"
	.data
	.align 32
	.type	inp, @object
	.size	inp, 72
inp:
	.quad	.LC3
	.quad	.LC4
	.quad	.LC45
	.quad	.LC5
	.quad	.LC6
	.quad	.LC7
	.quad	.LC8
	.quad	.LC9
	.quad	.LC10
	.quad	.LC11
	.globl	inplen
	.align 4
	.type	inplen, @object
	.size	inplen, 4
inplen:
	.long	10
	.text
	.globl	main
	.type	main, @function
main:
.LFB3:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	$0, -4(%rbp)
	jmp	.L39
.L40:
	movl	$0, %eax
	call	init
	movl	-4(%rbp), %eax
	cltq
	movq	inp(,%rax,8), %rax
	movq	%rax, %rdi
	call	strlen
	movl	%eax, %edx
	movl	-4(%rbp), %eax
	cltq
	movq	inp(,%rax,8), %rax
	movslq	%edx, %rsi
	addq	%rax, %rsi
	movq	%rax, %rdi
	call	exec
	movl	$0, %eax
	call	finish
	addl	$1, -4(%rbp)
.L39:
	movl	inplen(%rip), %eax
	cmpl	%eax, -4(%rbp)
	jl	.L40
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	main, .-main

	.bss
	.align 16
	.type   nfa_len, @object
	.size   nfa_len, 8
nfa_len:
	.zero   8
	.comm   nfa_bp,16384,32

	.ident	"GCC: (Ubuntu 4.8.2-19ubuntu1) 4.8.2"
	.section	.note.GNU-stack,"",@progbits

##### OUTPUT #####
1
1002000000002
12
-12321
2123
222222
-99
-> FAIL
-> FAIL
-> FAIL
saw 80808080
