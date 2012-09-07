include "testcase.txl"

keys
	'bool 'var
end keys


define go_statements
		[repeat go_lang_stmt]
end define

define go_lang_stmt
		[al_ragel_stmt]
	|	[go_variable_decl]
	|	[go_expr_stmt]
	|	[go_if_stmt]
	|   '{ [go_statements] '} [NL]
end define

define go_variable_decl
		'var [id] [SP] [SPOFF] [opt union] [go_type_decl] [SPON] [NL]
end define

define go_type_decl
		[al_type_decl]
	|	'bool
	|	'string
	|   'int
	|	'byte
end define

define go_expr_stmt
		[go_expr] '; [NL]
end define

define go_expr
		[go_term] [repeat go_expr_extend]
end define

define go_expr_extend
		[al_expr_op] [go_term]
end define

define go_term
		[al_term]
	|	[SPOFF] [go_type_decl] '( [SPON] [go_term] ')
	|	[id] [repeat go_dot_id]
	|	[SPOFF] [id] [repeat go_dot_id] '( [SPON] [go_args] ')
end define

define go_dot_id
		'. [id]
end define

define go_args
		[list go_expr]
end define

define go_sign
		'- | '+
end define

define go_if_stmt
		'if [go_expr] '{ [NL] [IN]
			[go_statements] [EX]
		[opt go_else]
		'} [NL]
end define

define go_else
		'} 'else '{ [NL] [IN]
			[go_statements] [EX]
end define

define go_lang
		[go_statements]
		'%% [NL]
		[go_statements]
		[ragel_def]
end define

define program
		[lang_indep]
	|	[go_lang]
end define

redefine al_host_block
		'{ [NL] [IN] [al_statements] [EX] '} [NL]
	|	'{ [NL] [IN] [go_statements] [EX] '} [NL]
end define

redefine cond_action_stmt
		'action [id] '{ [al_expr] '} [NL]
	|	'action [id] '{ [SPOFF] [go_expr] '} [SPON] [NL]
end redefine

rule charType
	replace [go_type_decl]
		'char
	by
		'byte
end rule

rule ptrTypes
	replace [go_type_decl]
		'ptr
	by
		'int
end rule

function alStmtToGo1 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		VarDecl [al_variable_decl]
	deconstruct VarDecl
		Type [al_type_decl] Id [id] OptUnion [opt union] ';
	construct GoType [go_type_decl]
		Type
	construct Result [go_variable_decl]
		'var Id OptUnion GoType [charType] [ptrTypes]
	replace [repeat go_lang_stmt]
	by
		Result
end function


rule alTermToGo1
	replace [al_term]
		'first_token_char
	by
		'data '[ts]
end rule

rule alTermToGo2
	replace [al_term]
		'< TypeDecl [al_type_decl] '> '( AlExpr [al_expr] ')
	construct TypeName [stringlit]
		_ [quote TypeDecl]
	construct TypeId [id]
		_ [unquote TypeName]
	by
		TypeId '( AlExpr ')
end rule

function alTermToGo
	replace [al_term]
		AlTerm [al_term]
	by
		AlTerm
			[alTermToGo1]
			[alTermToGo2]
end function

function alExprExtendToGo AlExprExtend [repeat al_expr_extend]
	deconstruct AlExprExtend
		Op [al_expr_op] Term [al_term] Rest [repeat al_expr_extend]
	construct GoRest [repeat go_expr_extend]
		_ [alExprExtendToGo Rest]
	replace [repeat go_expr_extend]
	by
		Op Term [alTermToGo] GoRest
end function

function alExprToGo AlExpr [al_expr]
	deconstruct AlExpr
		ALTerm [al_term] AlExprExtend [repeat al_expr_extend]
	construct GoExprExtend [repeat go_expr_extend]
		_ [alExprExtendToGo AlExprExtend]
	construct Result [opt go_expr]
		ALTerm [alTermToGo] GoExprExtend
	replace [opt go_expr]
	by
		Result
end function

function alStmtToGo2 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		AlExpr [al_expr] ';
	construct OptGoExpr [opt go_expr]
		_ [alExprToGo AlExpr]
	deconstruct OptGoExpr
		GoExpr [go_expr]
	replace [repeat go_lang_stmt]
	by
		GoExpr ';
end function

function alOptElseGo AlOptElse [opt al_else]
	deconstruct AlOptElse
		'else
			AlSubStmt [action_lang_stmt]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct GoSubStmts [repeat go_lang_stmt]
		_ [alToGo AlSubStmts]
	deconstruct GoSubStmts
		GoSubStmt [repeat go_lang_stmt]
	replace [opt go_else]
	by
		'} 'else '{
			GoSubStmt
end function

function alStmtToGo3 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'if '( AlExpr [al_expr] ')
			AlSubStmt [action_lang_stmt]
		AlOptElse [opt al_else]
	construct OptGoExpr [opt go_expr]
		_ [alExprToGo AlExpr]
	deconstruct OptGoExpr
		GoExpr [go_expr]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct GoSubStmts [repeat go_lang_stmt]
		_ [alToGo AlSubStmts]
	deconstruct GoSubStmts
		GoSubStmt [repeat go_lang_stmt]
	construct OptGoElse [opt go_else]
		_ [alOptElseGo AlOptElse]
	replace [repeat go_lang_stmt]
	by
		'if GoExpr '{
			GoSubStmt
		OptGoElse
		'}
end function

function alStmtToGo4a AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printi Id [id] ';
	construct PrintTerm [go_term]
		'fmt '. 'Print '( Id ')
	replace [repeat go_lang_stmt]
	by
		PrintTerm ';
end function

function alStmtToGo4b AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'prints String [stringlit] ';
	construct PrintTerm [go_term]
		'fmt '. 'Print '( String ')
	replace [repeat go_lang_stmt]
	by
		PrintTerm ';
end function

function alStmtToGo4c AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printb Id [id] ';
	construct PrintTerm [go_term]
		'fmt '. 'Print '( 'string '( Id '[:pos] ') ')
	replace [repeat go_lang_stmt]
	by
		PrintTerm ';
end function

function alStmtToGo4d AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'print_token ';
	construct PrintTerm [go_term]
		'fmt '. 'Print '( 'data '[ts:te] ')
	replace [repeat go_lang_stmt]
	by
		PrintTerm ';
end function

function alStmtToGo5 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'{ AlSubStmts [repeat action_lang_stmt] '}
	construct GoSubStmts [repeat go_lang_stmt]
		_ [alToGo AlSubStmts]
	replace [repeat go_lang_stmt]
	by
		'{ GoSubStmts '}
end function

function alStmtToGo6 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		RagelStmt [al_ragel_stmt]
	replace [repeat go_lang_stmt]
	by
		RagelStmt
end function

function alToGo AlStmts [repeat action_lang_stmt]
	deconstruct AlStmts
		FirstStmt [action_lang_stmt] Rest [repeat action_lang_stmt]
	construct GoFirst [repeat go_lang_stmt]
		_
			[alStmtToGo1 FirstStmt]
			[alStmtToGo2 FirstStmt]
			[alStmtToGo3 FirstStmt]
			[alStmtToGo4a FirstStmt]
			[alStmtToGo4b FirstStmt]
			[alStmtToGo4c FirstStmt]
			[alStmtToGo4d FirstStmt]
			[alStmtToGo5 FirstStmt]
			[alStmtToGo6 FirstStmt]
	construct GoRest [repeat go_lang_stmt]
		_ [alToGo Rest]
	replace [repeat go_lang_stmt]
	by
		GoFirst [. GoRest]
end function

rule actionTransGo
	replace [al_host_block]
		'{ AlStmts [repeat action_lang_stmt] '}
	construct GoStmts [repeat go_lang_stmt]
		_ [alToGo AlStmts]
	by
		'{ GoStmts '}
end rule

rule condTransGo
	replace [cond_action_stmt]
		'action Id [id] '{ AlExpr [al_expr] '}
	construct OptGoExpr [opt go_expr]
		_ [alExprToGo AlExpr]
	deconstruct OptGoExpr
		GoExpr [go_expr]
	by
		'action Id '{ GoExpr '}
end rule

rule machineName
	replace $ [machine_stmt]
		'machine _ [id] ';
	import TXLargs [repeat stringlit]
		Arg1 [stringlit] _ [repeat stringlit]
	construct ClassName [id]
		_ [unquote Arg1]
	by
		'machine ClassName ';
end rule

function langTransGo
	replace [program]
		Definitions [repeat action_lang_stmt]
		'%%
		Initializations [repeat action_lang_stmt]
		RagelDef [ragel_def]
	construct GoDefinitions [repeat go_lang_stmt]
		_ [alToGo Definitions]
	construct GoInitializations [repeat go_lang_stmt]
		_ [alToGo Initializations]
	construct NewRagelDef [ragel_def]
		RagelDef [actionTransGo] [condTransGo] [machineName]
	import ArrayInits [go_statements]
		ArrayInitStmts [repeat go_lang_stmt]
	by
		GoDefinitions
		'%%
		ArrayInitStmts [. GoInitializations]
		NewRagelDef
end function

function main
	replace [program]
		P [program]
	export ArrayInits [go_statements]
		_
	by
		P [langTransGo]
end function
