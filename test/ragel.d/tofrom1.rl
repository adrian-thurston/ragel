#
# @LANG: asm
#
# Tests to/from actions in ASM code. Based on atoi.
#

%%{
	machine AtoI;

	action begin {
		# neg = 0;
		# val = 0;
		movl	$0, neg(%rip)
		movl	$0, val(%rip)
	}

	action see_neg {
		# neg = 1;
		movl	$1, neg(%rip)
	}

	action add_digit {
		# val = val * 10 + (fc - 48);
		movl	val(%rip), %eax
		imul	$10, %eax
		movsbl	(%r12), %ecx
		subl	$48, %ecx
		add		%ecx, %eax
		movl	%eax, val(%rip)
	}

	action finish {
		#if (neg)
		#{
		#	val = - 1 * val;
		#}

		movl	neg(%rip), %eax
		cmpl	$0, %eax
		je		.finish_L
		movl	val(%rip), %eax
		negl	%eax
		movl	%eax, val(%rip)
	.finish_L:
	}

	action print {
		movl	$.L_fmt_i_nl, %edi
		movl	val(%rip), %esi
		movl	$0, %eax
		call	printf
	}

	action tos {
		movl	$.L_tos, %edi
		movsbl	(%r12), %esi
		movl	$0, %eax
		call	printf
	}

	action froms {
		movl	$.L_froms, %edi
		movsbl	(%r12), %esi
		movl	$0, %eax
		call	printf
	}

	atoi = 
		(('-' @ see_neg | '+') ?
		(digit @ add_digit) +) > begin %finish;

	main := ( atoi '\n' @ print )
			$to(tos) $from(froms);

}%%
	.file	"tmp.c"
	.comm	neg,4,4
	.comm	val,4,4
	.comm	cs,4,4
	.section	.rodata

%% write data;

.L_fmt_i_nl:
	.string "%i\n"
.L_tos:
	.string "to on %d\n"
.L_froms:
	.string "from on %d\n"

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
	movl	$0, val(%rip)
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

	push    %r12
	push    %r13

	movq    cs(%rip), %r11
	movq    %rdi, %r12
	movq    %rsi, %r13

%% write exec;

	movq    %r11, cs(%rip)

	pop		%r13
	pop		%r12

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
	movl	cs(%rip), %edx
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
	.long	9
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
	.ident	"GCC: (Ubuntu 4.8.2-19ubuntu1) 4.8.2"
	.section	.note.GNU-stack,"",@progbits

##### OUTPUT #####
from on 49
to on 49
from on 10
1
to on 10
from on 49
to on 49
from on 50
to on 50
from on 10
12
to on 10
from on 50
to on 50
from on 50
to on 50
from on 50
to on 50
from on 50
to on 50
from on 50
to on 50
from on 50
to on 50
from on 10
222222
to on 10
from on 43
to on 43
from on 50
to on 50
from on 49
to on 49
from on 50
to on 50
from on 51
to on 51
from on 10
2123
to on 10
from on 45
to on 45
from on 57
to on 57
from on 57
to on 57
from on 10
-99
to on 10
from on 45
to on 45
from on 49
to on 49
from on 50
to on 50
from on 51
to on 51
from on 50
to on 50
from on 49
to on 49
from on 10
-12321
to on 10
from on 50
to on 50
from on 49
to on 49
from on 51
to on 51
from on 32
-> FAIL
from on 45
to on 45
from on 45
-> FAIL
from on 32
-> FAIL
