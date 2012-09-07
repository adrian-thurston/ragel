include "testcase.txl"

define c_statements
		[repeat c_lang_stmt]
end define

define c_lang_stmt
		[al_ragel_stmt]
	|	[c_variable_decl]
	|	[c_expr_stmt]
	|	[c_if_stmt]
	|	[EX] '{ [IN] [NL] [c_statements] [EX] '} [IN] [NL]
end define

define c_variable_decl
		[c_type_decl] [id] [opt union] '; [NL]
end define

define c_type_decl
		[al_type_decl]
	|	'char '*
end define

define c_expr_stmt
		[c_expr] '; [NL]
end define

define c_expr
		[c_term] [repeat c_expr_extend]
end define

define c_expr_extend
		[al_expr_op] [c_term]
end define

define c_term
		[al_term]
	|	[id] '( [c_args] ')
end define

define c_args
		[list c_expr] 
end define

define c_sign
		'- | '+
end define

define c_if_stmt
		'if '( [c_expr] ') [NL] [IN]
			[c_lang_stmt] [EX]
		[opt c_else]
end define

define c_else
		'else [NL] [IN]
			[c_lang_stmt] [EX]
end define

define c_lang
		[c_statements]
		'%% [NL]
		[c_statements]
		[ragel_def]
end define

define program
		[lang_indep]
	|	[c_lang]
end define

redefine al_host_block
		'{ [NL] [IN] [al_statements] [EX] '} [NL]
	|	'{ [NL] [IN] [c_statements] [EX] '} [NL]
end define

redefine cond_action_stmt
		'action [id] '{ [al_expr] '} [NL]
	|	'action [id] '{ [c_expr] '} [NL]
end redefine


rule boolTypes
	replace [al_type_decl]
		'bool
	by
		'int
end rule

rule ptrTypes
	replace [c_type_decl]
		'ptr
	by
		'char '*
end rule

rule boolVals1
	replace [al_term]
		'true
	by
		'1
end rule

rule boolVals2
	replace [al_term]
		'false
	by
		'0
end rule

function alStmtToC1 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		VarDecl [al_variable_decl]
	deconstruct VarDecl
		Type [al_type_decl] Id [id] OptUnion [opt union]';
	construct CType [c_type_decl]
		Type
	construct Result [c_variable_decl]
		CType [boolTypes] [ptrTypes] Id OptUnion ';
	replace [repeat c_lang_stmt]
	by
		Result
end function

rule alTermToC1
	replace [al_term]
		'first_token_char
	by
		'ts '[0]
end rule

rule alTermToC2
	replace [al_term]
		'< _ [al_type_decl] '> '( AlExpr [al_expr] ')
	by
		'( AlExpr ')
end rule

function alTermToC
	replace [al_term]
		AlTerm [al_term]
	by
		AlTerm
			[alTermToC1]
			[alTermToC2]
end function

function alExprExtendToC AlExprExtend [repeat al_expr_extend]
	deconstruct AlExprExtend
		Op [al_expr_op] Term [al_term] Rest [repeat al_expr_extend]
	construct RestC [repeat c_expr_extend]
		_ [alExprExtendToC Rest]
	replace [repeat c_expr_extend]
	by
		Op Term [alTermToC] RestC
end function

function alExprToC AlExpr [al_expr]
	deconstruct AlExpr
		ALTerm [al_term] AlExprExtend [repeat al_expr_extend]
	construct CExprExtend [repeat c_expr_extend]
		_ [alExprExtendToC AlExprExtend]
	construct Result [opt c_expr]
		ALTerm [alTermToC] CExprExtend
	replace [opt c_expr]
	by
		Result [boolVals1] [boolVals2]
end function

function alStmtToC2 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		AlExpr [al_expr] ';
	construct OptCExpr [opt c_expr]
		_ [alExprToC AlExpr]
	deconstruct OptCExpr
		CExpr [c_expr]
	replace [repeat c_lang_stmt]
	by
		CExpr ';
end function

function alOptElseC AlOptElse [opt al_else]
	deconstruct AlOptElse
		'else 
			AlSubStmt [action_lang_stmt]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct CSubStmts [repeat c_lang_stmt]
		_ [alToC AlSubStmts]
	deconstruct CSubStmts
		CSubStmt [c_lang_stmt]
	replace [opt c_else]
	by
		'else 
			CSubStmt
end function

function alStmtToC3 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'if '( AlExpr [al_expr] ')
			AlSubStmt [action_lang_stmt]
		AlOptElse [opt al_else]
	construct OptCExpr [opt c_expr]
		_ [alExprToC AlExpr]
	deconstruct OptCExpr
		CExpr [c_expr]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct CSubStmts [repeat c_lang_stmt]
		_ [alToC AlSubStmts]
	deconstruct CSubStmts
		CSubStmt [c_lang_stmt]
	construct OptCElse [opt c_else]
		_ [alOptElseC AlOptElse]
	replace [repeat c_lang_stmt]
	by
		'if '( CExpr ')
			CSubStmt
		OptCElse
end function

function alStmtToC4a AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printi Id [id] ';
	replace [repeat c_lang_stmt]
	by
		'printf '( '"%i" ', Id ');
end function

function alStmtToC4b AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'prints String [stringlit] ';
	replace [repeat c_lang_stmt]
	by
		'fputs '( String , 'stdout ');
end function

function alStmtToC4c AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printb Id [id] ';
	replace [repeat c_lang_stmt]
	by
		'fwrite '( Id ', '1 ', 'pos ', 'stdout ');
end function

function alStmtToC4d AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'print_token ';
	replace [repeat c_lang_stmt]
	by
		'fwrite '( 'ts ', '1 ', 'te '- 'ts ', 'stdout ');
end function

function alStmtToC5 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'{ AlSubStmts [repeat action_lang_stmt] '}
	construct CSubStmts [repeat c_lang_stmt]
		_ [alToC AlSubStmts]
	replace [repeat c_lang_stmt]
	by
		'{ CSubStmts '}
end function

function alStmtToC6 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		RagelStmt [al_ragel_stmt]
	replace [repeat c_lang_stmt]
	by
		RagelStmt
end function

function alToC AlStmts [repeat action_lang_stmt]
	deconstruct AlStmts
		FirstStmt [action_lang_stmt] Rest [repeat action_lang_stmt]
	construct FirstC [repeat c_lang_stmt]
		_ 
			[alStmtToC1 FirstStmt]
			[alStmtToC2 FirstStmt]
			[alStmtToC3 FirstStmt]
			[alStmtToC4a FirstStmt]
			[alStmtToC4b FirstStmt]
			[alStmtToC4c FirstStmt]
			[alStmtToC4d FirstStmt]
			[alStmtToC5 FirstStmt]
			[alStmtToC6 FirstStmt]
	construct RestC [repeat c_lang_stmt]
		_ [alToC Rest]
	replace [repeat c_lang_stmt]
	by
		FirstC [. RestC]
end function

rule actionTransC
	replace [al_host_block]
		'{ AlStmts [repeat action_lang_stmt] '}
	construct CStmts [repeat c_lang_stmt]
		_ [alToC AlStmts]
	by
		'{ CStmts '}
end rule

rule condTransC
	replace [cond_action_stmt]
		'action Id [id] '{ AlExpr [al_expr] '}
	construct OptCExpr [opt c_expr]
		_ [alExprToC AlExpr]
	deconstruct OptCExpr
		CExpr [c_expr]
	by
		'action Id '{ CExpr '}
end rule

function langTransC
	replace [program]
		Definitions [repeat action_lang_stmt]
		'%%
		Initializations [repeat action_lang_stmt]
		RagelDef [ragel_def]
	construct CDefinitions [repeat c_lang_stmt]
		_ [alToC Definitions]
	construct CInitializations [repeat c_lang_stmt]
		_ [alToC Initializations]
	by
		CDefinitions
		'%%
		CInitializations
		RagelDef [actionTransC] [condTransC]
end function

function main
	replace [program]
		P [program]
	by
		P [langTransC]
end function
