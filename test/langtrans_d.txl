include "testcase.txl"

define d_statements
		[repeat d_lang_stmt]
end define

define d_lang_stmt
		[al_ragel_stmt]
	|	[d_variable_decl]
	|	[d_expr_stmt]
	|	[d_if_stmt]
	|	[EX] '{ [IN] [NL] [d_statements] [EX] '} [IN] [NL]
end define

define d_variable_decl
		[d_type_decl] [id] [opt union] '; [NL]
end define

define d_type_decl
		[al_type_decl]
	|	'char '*
end define

define d_expr_stmt
		[d_expr] '; [NL]
end define

define d_expr
		[d_term] [repeat d_expr_extend]
end define

define d_expr_extend
		[al_expr_op] [d_term]
end define

define d_term
		[al_term]
	|	[id] '( [d_args] ')
end define

define d_args
		[list d_expr] 
end define

define d_sign
		'- | '+
end define

define d_if_stmt
		'if '( [d_expr] ') [NL] [IN]
			[d_lang_stmt] [EX]
		[opt d_else]
end define

define d_else
		'else [NL] [IN]
			[d_lang_stmt] [EX]
end define

define d_lang
		[d_statements]
		'%% [NL]
		[d_statements]
		[ragel_def]
end define

define program
		[lang_indep]
	|	[d_lang]
end define

redefine al_host_block
		'{ [NL] [IN] [al_statements] [EX] '} [NL]
	|	'{ [NL] [IN] [d_statements] [EX] '} [NL]
end define

rule ptrTypes
	replace [d_type_decl]
		'ptr
	by
		'char '*
end rule

function alStmtToD1 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		VarDecl [al_variable_decl]
	deconstruct VarDecl
		Type [al_type_decl] Id [id] OptUnion [opt union] ';
	construct DType [d_type_decl]
		Type
	construct Result [d_variable_decl]
		DType [ptrTypes] Id OptUnion ';
	replace [repeat d_lang_stmt]
	by
		Result
end function

rule alTermToD1
	replace [al_term]
		'first_token_char
	by
		'ts '[0]
end rule

rule alTermToD2
	replace [al_term]
		'< _ [al_type_decl] '> '( AlExpr [al_expr] ')
	by
		'( AlExpr ')
end rule

function alTermToD
	replace [al_term]
		AlTerm [al_term]
	by
		AlTerm
			[alTermToD1]
			[alTermToD2]
end function

function alExprExtendToD AlExprExtend [repeat al_expr_extend]
	deconstruct AlExprExtend
		Op [al_expr_op] Term [al_term] Rest [repeat al_expr_extend]
	construct DRest [repeat d_expr_extend]
		_ [alExprExtendToD Rest]
	replace [repeat d_expr_extend]
	by
		Op Term [alTermToD] DRest
end function

function alExprToD AlExpr [al_expr]
	deconstruct AlExpr
		ALTerm [al_term] AlExprExtend [repeat al_expr_extend]
	construct DExprExtend [repeat d_expr_extend]
		_ [alExprExtendToD AlExprExtend]
	construct Result [opt d_expr]
		ALTerm [alTermToD] DExprExtend
	replace [opt d_expr]
	by
		Result
end function

function alStmtToD2 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		AlExpr [al_expr] ';
	construct OptDExpr [opt d_expr]
		_ [alExprToD AlExpr]
	deconstruct OptDExpr
		DExpr [d_expr]
	replace [repeat d_lang_stmt]
	by
		DExpr ';
end function

function alOptElseD AlOptElse [opt al_else]
	deconstruct AlOptElse
		'else 
			AlSubStmt [action_lang_stmt]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct DSubStmts [repeat d_lang_stmt]
		_ [alToD AlSubStmts]
	deconstruct DSubStmts
		DSubStmt [d_lang_stmt]
	replace [opt d_else]
	by
		'else 
			DSubStmt
end function

function alStmtToD3 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'if '( AlExpr [al_expr] ')
			AlSubStmt [action_lang_stmt]
		AlOptElse [opt al_else]
	construct OptDExpr [opt d_expr]
		_ [alExprToD AlExpr]
	deconstruct OptDExpr
		DExpr [d_expr]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct DSubStmts [repeat d_lang_stmt]
		_ [alToD AlSubStmts]
	deconstruct DSubStmts
		DSubStmt [d_lang_stmt]
	construct OptDElse [opt d_else]
		_ [alOptElseD AlOptElse]
	replace [repeat d_lang_stmt]
	by
		'if '( DExpr ')
			DSubStmt
		OptDElse
end function

function alStmtToD4a AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printi Id [id] ';
	replace [repeat d_lang_stmt]
	by
		'writef '( '"%d" ', Id ') ';
end function

function alStmtToD4b AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'prints String [stringlit] ';
	replace [repeat d_lang_stmt]
	by
		'writef '( '"%s" ', String ') ';
end function

function alStmtToD4c AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printb Id [id] ';
	replace [repeat d_lang_stmt]
	by
		'_s '= Id '[0..pos] ';
		'writef '( '"%s" ', '_s ') ';
end function

function alStmtToD4d AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'print_token ';
	replace [repeat d_lang_stmt]
	by
		'_s '= ts '[0..(te-ts)] ';
		'writef '( '"%s" ', '_s ') ';
end function

function alStmtToD5 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'{ AlSubStmts [repeat action_lang_stmt] '}
	construct DSubStmts [repeat d_lang_stmt]
		_ [alToD AlSubStmts]
	replace [repeat d_lang_stmt]
	by
		'{ DSubStmts '}
end function

function alStmtToD6 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		RagelStmt [al_ragel_stmt]
	replace [repeat d_lang_stmt]
	by
		RagelStmt
end function

function alToD AlStmts [repeat action_lang_stmt]
	deconstruct AlStmts
		FirstStmt [action_lang_stmt] Rest [repeat action_lang_stmt]
	construct DFirst [repeat d_lang_stmt]
		_ 
			[alStmtToD1 FirstStmt]
			[alStmtToD2 FirstStmt]
			[alStmtToD3 FirstStmt]
			[alStmtToD4a FirstStmt]
			[alStmtToD4b FirstStmt]
			[alStmtToD4c FirstStmt]
			[alStmtToD4d FirstStmt]
			[alStmtToD5 FirstStmt]
			[alStmtToD6 FirstStmt]
	construct DRest [repeat d_lang_stmt]
		_ [alToD Rest]
	replace [repeat d_lang_stmt]
	by
		DFirst [. DRest]
end function

rule actionTransD
	replace [al_host_block]
		'{ AlStmts [repeat action_lang_stmt] '}
	construct DStmts [repeat d_lang_stmt]
		_ [alToD AlStmts]
	by
		'{ DStmts '}
end rule

function langTransD
	replace [program]
		Definitions [repeat action_lang_stmt]
		'%%
		Initializations [repeat action_lang_stmt]
		RagelDef [ragel_def]
	construct DDefinitions [repeat d_lang_stmt]
		_ [alToD Definitions]
	construct DInitializations [repeat d_lang_stmt]
		_ [alToD Initializations]
	by
		DDefinitions
		'%%
		DInitializations
		RagelDef [actionTransD]
end function

function main
	replace [program]
		P [program]
	by
		P [langTransD]
end function
