# 
# @LANG: asm
# 

%%{

	machine gen_rep;

	action eol { 
		# p+1 == eof

		# Not the most efficient
		movq	$0, %rax
		movq	%r12, %rcx
		addq	$1, %rcx
		movq	-8(%rbp), %rdx
		cmpq	%rcx, %rdx
		jne		1f
		movq	$1, %rax
		1:
	}

	eol = '' %when eol;


	action ini {
		# ini
		movl	$0, c(%rip)
		movq	$1, %rax
	}

	action stay {
		# stay
		movq	$1, %rax
	}

	action repeat {
		# repeat

		movl	c(%rip), %eax
		addl	$1, %eax
		movl	%eax, c(%rip)

		cmpl	$3, %eax
		setl    %al
		movzbl	%al, %eax
	}

	action exit {
		# exit

		movl	c(%rip), %eax
		addl	$1, %eax
		movl	%eax, c(%rip)

		cmpl	$2, %eax
		setge	%al
		movzbl	%al, %eax
	}

	action psh
	{
		# PUSH
		movq	nfa_s@GOTPCREL(%rip), %rax
		movq	-88(%rbp), %rcx
		sal		$3, %rcx
		movl	c(%rip), %edx
		movq	%rdx, 0(%rax,%rcx,)

		# movl	$.L_push, %edi
		# call	puts
	}

	action pop
	{
		# POP
		movq	nfa_s@GOTPCREL(%rip), %rax
		movq	-88(%rbp), %rcx
		sal		$3, %rcx
		movq	0(%rax,%rcx,), %rdx
		movl	%edx, c(%rip)
		movq	$1, %rax

		# movl	$.L_pop, %edi
		# call	puts
	}

	action char
	{
		# movl	$.L_char, %edi
		# movq	c(%rip), %rsi
		# movl	$0, %eax
		# call	printf
	}

	action marker 
	{
		movl	$.L_marker, %edi
		call	puts
		movl	$1, %eax
	}

	main := 
		(
			( '' %when marker
				:nfa( 2, ( 'a' @char ) , 
				psh, pop, ini, stay, repeat, exit ): ' ' ) {2}
			eol
		)
		:>
		any
		@{
			# printf("----- MATCH\n");
			movl	$.L_match, %edi
			call	puts
		}
	;

}%%
	.file	"tmp.c"
	.comm	neg,4,4
	.comm	val,8,8
	.comm	cs,4,4
	.comm	c,4,4
	.section	.rodata

%% write data;

.L_fmt_si_nl:
	.string "set   top: %ld\n"
.L_fmt_ci_nl:
	.string "check top: %ld\n"
.L_fmt_s_nl:
	.string "restart: %d\n"
.L_marker:
	.string "  marker"
.L_match:
	.string "----- MATCH"

.L_push:
	.string "push"
.L_pop:
	.string "pop"
.L_char:
	.string "char %d\n"
.L_ini:
	.string "ini"
.L_min:
	.string "min %d\n"
.L_max:
	.string "max"

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
	subq	$88, %rsp

	pushq   %r12
	pushq   %r13

	movq	nfa_bp@GOTPCREL(%rip), %rax
	movq	%rax, -80(%rbp)
	movq	$0, -88(%rbp)
	
	movq    cs(%rip), %r11
	movq    %rdi, %r12
	movq    %rsi, %r13

	movq	%r13, -8(%rbp)

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

	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	finish, .-finish
	.globl	inp

	.section	.rodata
.LC10:
	.string "a "
.LC11:
	.string "aa "
.LC12:
	.string "aaa "
.LC13:
	.string "aaaa "
.LC14:
	.string "a a "
.LC15:
	.string "aa aa "
.LC16:
	.string "aaa aaa "
.LC17:
	.string "aaaa aaaa "
.LC18:
	.string "a a a "
.LC19:
	.string "aa aa aa "
.LC20:
	.string "aaa aaa aaa "
.LC21:
	.string "aaaa aaaa aaaa "
.LC22:
	.string "aa a "
.LC23:
	.string "aa aaa "
.LC24:
	.string "aa aaaa "
.LC25:
	.string "aaa a "
.LC26:
	.string "aaa aa "
.LC27:
	.string "aaa aaaa "

	.data
	.align 32
	.type	inp, @object
	.size	inp, 72
inp:
	.quad	.LC10
	.quad	.LC11
	.quad	.LC12
	.quad	.LC13
	.quad	.LC14
	.quad	.LC15
	.quad	.LC16
	.quad	.LC17
	.quad	.LC18
	.quad	.LC19
	.quad	.LC20
	.quad	.LC21
	.quad	.LC22
	.quad	.LC23
	.quad	.LC24
	.quad	.LC25
	.quad	.LC26
	.quad	.LC27
	.globl	inplen
	.align 4
	.type	inplen, @object
	.size	inplen, 4
inplen:
	.long	18
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
	call	puts

	movl	-4(%rbp), %eax
	cltq
	movq	inp(,%rax,8), %rax
	movq	%rax, %rdi
	call	strlen
	movl	%eax, %edx
	addl	$1, %edx
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
	.comm	nfa_s,16384,32

	.ident	"GCC: (Ubuntu 4.8.2-19ubuntu1) 4.8.2"
	.section	.note.GNU-stack,"",@progbits

##### OUTPUT #####
a 
  marker
aa 
  marker
  marker
aaa 
  marker
  marker
aaaa 
  marker
a a 
  marker
aa aa 
  marker
  marker
----- MATCH
aaa aaa 
  marker
  marker
----- MATCH
aaaa aaaa 
  marker
a a a 
  marker
aa aa aa 
  marker
  marker
aaa aaa aaa 
  marker
  marker
aaaa aaaa aaaa 
  marker
aa a 
  marker
  marker
aa aaa 
  marker
  marker
----- MATCH
aa aaaa 
  marker
  marker
aaa a 
  marker
  marker
aaa aa 
  marker
  marker
----- MATCH
aaa aaaa 
  marker
  marker
