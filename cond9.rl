#
# @LANG: asm
#

%%{
	machine foo;

	action c1 {
		/* i != 0 */
		movl	i(%rip), %eax
	}
	action c2 {
		/* j != 0 */
		movl	j(%rip), %eax
	}
	action c3 {
		/* k != 0 */
		movl	k(%rip), %eax
	}

	action one { 
		/* print_str "  one\n"; */
		movl	$.L_one, %edi
		call	puts
	}
	action two { 
		/* print_str "  two\n"; */
		movl	$.L_two, %edi
		call	puts
	}
	action three {
		/* print_str "  three\n"; */
		movl	$.L_three, %edi
		call	puts
	}

	action seti {
		//if ( fc == 48 )
		//	i = false;
		//else
		//	i = true;
		movb	(%r12), %al
		cmpb	$48, %al
		setne	%al
		movsbl	%al, %eax
		movl	%eax, i(%rip)
	}
	action setj {
		// if ( fc == 48 )
		//	j = false; 
		// else
		//	j = true;
		movb	(%r12), %al
		cmpb	$48, %al
		setne	%al
		movsbl	%al, %eax
		movl	%eax, j(%rip)
	}
	action setk {
		// if ( fc == 48 )
		//	k = false; 
		// else
		//	k = true;
		movb	(%r12), %al
		cmpb	$48, %al
		setne	%al
		movsbl	%al, %eax
		movl	%eax, k(%rip)
	}

	action break {fnbreak;}

	one = 'a' 'b' when c1 'c' @one;
	two = 'a'* 'b' when c2 'c' @two;
	three = 'a'+ 'b' when c3 'c' @three;

	main := 
		[01] @seti
		[01] @setj
		[01] @setk
		( one | two | three ) '\n' @break;
	
}%%

	.file	"cond1_c.c"
	.comm	i,4,4
	.comm	j,4,4
	.comm	k,4,4
	.comm	cs,4,4
	.section	.rodata

%% write data;

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

%% write init;

	movq    %r11, cs(%rip)

	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	init, .-init
	.section	.rodata
.L_one:
	.string	"  one"
.L_two:
	.string	"  two"
.L_three:
	.string	"  three"
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
	subq	$160, %rsp

	push    %r12
	push    %r13

	movq    cs(%rip), %r11
	movq    %rdi, %r12
	movq    %rsi, %r13

%% write exec;

	movq    %r11, cs(%rip)

	pop		%r13
	pop		%r12

	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	exec, .-exec
	.section	.rodata
.LC3:
	.string	"ACCEPT"
.LC4:
	.string	"FAIL"
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
	movl	$15, %eax
	cmpl	%eax, %edx
	jl	.L69
	movl	$.LC3, %edi
	call	puts
	jmp	.L68
.L69:
	movl	$.LC4, %edi
	call	puts
.L68:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	finish, .-finish
	.globl	inp
	.section	.rodata
.LC5:
	.string	"000abc\n"
.LC6:
	.string	"100abc\n"
.LC7:
	.string	"010abc\n"
.LC8:
	.string	"110abc\n"
.LC9:
	.string	"001abc\n"
.LC10:
	.string	"101abc\n"
.LC11:
	.string	"011abc\n"
.LC12:
	.string	"111abc\n"
	.data
	.align 32
	.type	inp, @object
	.size	inp, 64
inp:
	.quad	.LC5
	.quad	.LC6
	.quad	.LC7
	.quad	.LC8
	.quad	.LC9
	.quad	.LC10
	.quad	.LC11
	.quad	.LC12
	.globl	inplen
	.align 4
	.type	inplen, @object
	.size	inplen, 4
inplen:
	.long	8
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
	jmp	.L72
.L73:
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
.L72:
	movl	inplen(%rip), %eax
	cmpl	%eax, -4(%rbp)
	jl	.L73
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 4.8.2-19ubuntu1) 4.8.2"
	.section	.note.GNU-stack,"",@progbits
#ifdef _____OUTPUT_____
FAIL
  one
ACCEPT
  two
ACCEPT
  one
  two
ACCEPT
  three
ACCEPT
  one
  three
ACCEPT
  two
  three
ACCEPT
  one
  two
  three
ACCEPT
#endif
