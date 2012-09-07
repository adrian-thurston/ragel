include "testcase.txl"

keys
	'bool 'new
end keys


define csharp_statements
		[repeat csharp_lang_stmt]
end define

define csharp_lang_stmt
		[al_ragel_stmt]
	|	[csharp_variable_decl]
	|	[csharp_expr_stmt]
	|	[csharp_if_stmt]
	|	[EX] '{ [IN] [NL] [csharp_statements] [EX] '} [IN] [NL]
end define

define csharp_variable_decl
		[csharp_type_decl] [opt union] [id] '; [NL]
end define

define csharp_type_decl
		[al_type_decl]
	|	'bool
	|	'String
end define

define csharp_expr_stmt
		[csharp_expr] '; [NL]
end define

define csharp_expr
		[csharp_term] [repeat csharp_expr_extend]
end define

define csharp_expr_extend
		[al_expr_op] [csharp_term]
end define

define csharp_term
		[al_term]
	|	[id] [repeat csharp_dot_id]
	|	[id] [repeat csharp_dot_id] '( [csharp_args] ')
	|	'new [csharp_type_decl] [union]
	|	'new [csharp_type_decl] '( [csharp_args] ') 
end define

define csharp_dot_id
		'. [id]
end define

define csharp_args
		[list csharp_expr] 
end define

define csharp_sign
		'- | '+
end define

define csharp_if_stmt
		'if '( [csharp_expr] ') [NL] [IN]
			[csharp_lang_stmt] [EX]
		[opt csharp_else]
end define

define csharp_else
		'else [NL] [IN]
			[csharp_lang_stmt] [EX]
end define

define csharp_lang
		[csharp_statements]
		'%% [NL]
		[csharp_statements]
		[ragel_def]
end define

define program
		[lang_indep]
	|	[csharp_lang]
end define

redefine al_host_block
		'{ [NL] [IN] [al_statements] [EX] '} [NL]
	|	'{ [NL] [IN] [csharp_statements] [EX] '} [NL]
end define

redefine cond_action_stmt
		'action [id] '{ [al_expr] '} [NL]
	|	'action [id] '{ [csharp_expr] '} [NL]
end redefine


function clearUnion Type [csharp_type_decl] Id [id] 
	replace [opt union]
		Union [union]
	import ArrayInits [csharp_statements]
		Stmts [repeat csharp_lang_stmt]
	export ArrayInits 
		Id '= 'new Type Union '; Stmts
	by
		'[]
end function

rule ptrTypes
	replace [al_type_decl]
		'ptr
	by
		'int
end rule

function alStmtToCSharp1 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		VarDecl [al_variable_decl]
	deconstruct VarDecl
		Type [al_type_decl] Id [id] OptUnion [opt union] ';
	construct CSharpType [csharp_type_decl]
		Type
	construct Result [csharp_variable_decl]
		CSharpType [ptrTypes] OptUnion [clearUnion CSharpType Id] Id ';
	replace [repeat csharp_lang_stmt]
	by
		Result
end function

rule alTermToCSharp1
	replace [al_term]
		'first_token_char
	by
		'data '[ts]
end rule

rule alTermToCSharp2
	replace [al_term]
		'< _ [al_type_decl] '> '( AlExpr [al_expr] ')
	by
		'( AlExpr ')
end rule

function alTermToCSharp
	replace [al_term]
		AlTerm [al_term]
	by
		AlTerm
			[alTermToCSharp1]
			[alTermToCSharp2]
end function

function alExprExtendToCSharp AlExprExtend [repeat al_expr_extend]
	deconstruct AlExprExtend
		Op [al_expr_op] Term [al_term] Rest [repeat al_expr_extend]
	construct CSharpRest [repeat csharp_expr_extend]
		_ [alExprExtendToCSharp Rest]
	replace [repeat csharp_expr_extend]
	by
		Op Term [alTermToCSharp] CSharpRest
end function

function alExprToCSharp AlExpr [al_expr]
	deconstruct AlExpr
		ALTerm [al_term] AlExprExtend [repeat al_expr_extend]
	construct CSharpExprExtend [repeat csharp_expr_extend]
		_ [alExprExtendToCSharp AlExprExtend]
	construct Result [opt csharp_expr]
		ALTerm [alTermToCSharp] CSharpExprExtend
	replace [opt csharp_expr]
	by
		Result 
end function

function alStmtToCSharp2 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		AlExpr [al_expr] ';
	construct OptCSharpExpr [opt csharp_expr]
		_ [alExprToCSharp AlExpr]
	deconstruct OptCSharpExpr
		CSharpExpr [csharp_expr]
	replace [repeat csharp_lang_stmt]
	by
		CSharpExpr ';
end function

function alOptElseCSharp AlOptElse [opt al_else]
	deconstruct AlOptElse
		'else 
			AlSubStmt [action_lang_stmt]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct CSharpSubStmts [repeat csharp_lang_stmt]
		_ [alToCSharp AlSubStmts]
	deconstruct CSharpSubStmts
		CSharpSubStmt [csharp_lang_stmt]
	replace [opt csharp_else]
	by
		'else 
			CSharpSubStmt
end function

function alStmtToCSharp3 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'if '( AlExpr [al_expr] ')
			AlSubStmt [action_lang_stmt]
		AlOptElse [opt al_else]
	construct OptCSharpExpr [opt csharp_expr]
		_ [alExprToCSharp AlExpr]
	deconstruct OptCSharpExpr
		CSharpExpr [csharp_expr]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct CSharpSubStmts [repeat csharp_lang_stmt]
		_ [alToCSharp AlSubStmts]
	deconstruct CSharpSubStmts
		CSharpSubStmt [csharp_lang_stmt]
	construct OptCSharpElse [opt csharp_else]
		_ [alOptElseCSharp AlOptElse]
	replace [repeat csharp_lang_stmt]
	by
		'if '( CSharpExpr ')
			CSharpSubStmt
		OptCSharpElse
end function

function alStmtToCSharp4a AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printi Id [id] ';
	replace [repeat csharp_lang_stmt]
	by
		'Console '. 'Write '( Id ');
end function

function alStmtToCSharp4b AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'prints String [stringlit] ';
	replace [repeat csharp_lang_stmt]
	by
		'Console '. 'Write '( String ');
end function

function alStmtToCSharp4c AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printb Id [id] ';
	replace [repeat csharp_lang_stmt]
	by
		'_s '= 'new 'String '( Id ', '0 ', 'pos ') ';
		'Console '. 'Write '( '_s ');
end function

function alStmtToCSharp4d AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'print_token ';
	replace [repeat csharp_lang_stmt]
	by
		'_s '= 'new 'String '( 'data ', 'ts ', 'te '- 'ts ') ';
		'Console '. 'Write '( '_s ');
end function

function alStmtToCSharp5 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'{ AlSubStmts [repeat action_lang_stmt] '}
	construct CSharpSubStmts [repeat csharp_lang_stmt]
		_ [alToCSharp AlSubStmts]
	replace [repeat csharp_lang_stmt]
	by
		'{ CSharpSubStmts '}
end function

function alStmtToCSharp6 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		RagelStmt [al_ragel_stmt]
	replace [repeat csharp_lang_stmt]
	by
		RagelStmt
end function


function alToCSharp AlStmts [repeat action_lang_stmt]
	deconstruct AlStmts
		FirstStmt [action_lang_stmt] Rest [repeat action_lang_stmt]
	construct CSharpFirst [repeat csharp_lang_stmt]
		_ 
			[alStmtToCSharp1 FirstStmt]
			[alStmtToCSharp2 FirstStmt]
			[alStmtToCSharp3 FirstStmt]
			[alStmtToCSharp4a FirstStmt]
			[alStmtToCSharp4b FirstStmt]
			[alStmtToCSharp4c FirstStmt]
			[alStmtToCSharp4d FirstStmt]
			[alStmtToCSharp5 FirstStmt]
			[alStmtToCSharp6 FirstStmt]
	construct CSharpRest [repeat csharp_lang_stmt]
		_ [alToCSharp Rest]
	replace [repeat csharp_lang_stmt]
	by
		CSharpFirst [. CSharpRest]
end function

rule actionTransCSharp
	replace [al_host_block]
		'{ AlStmts [repeat action_lang_stmt] '}
	construct CSharpStmts [repeat csharp_lang_stmt]
		_ [alToCSharp AlStmts]
	by
		'{ CSharpStmts '}
end rule

rule condTransCSharp
	replace [cond_action_stmt]
		'action Id [id] '{ AlExpr [al_expr] '}
	construct OptCSharpExpr [opt csharp_expr]
		_ [alExprToCSharp AlExpr]
	deconstruct OptCSharpExpr
		CSharpExpr [csharp_expr]
	by
		'action Id '{ CSharpExpr '}
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

function langTransCSharp
	replace [program]
		Definitions [repeat action_lang_stmt]
		'%%
		Initializations [repeat action_lang_stmt]
		RagelDef [ragel_def]
	construct CSharpDefinitions [repeat csharp_lang_stmt]
		_ [alToCSharp Definitions]
	construct CSharpInitializations [repeat csharp_lang_stmt]
		_ [alToCSharp Initializations]
	construct NewRagelDef [ragel_def]
		RagelDef [actionTransCSharp] [condTransCSharp] [machineName]
	import ArrayInits [csharp_statements]
		ArrayInitStmts [repeat csharp_lang_stmt]
	by
		CSharpDefinitions
		'%%
		ArrayInitStmts [. CSharpInitializations]
		NewRagelDef
end function

function main
	replace [program]
		P [program]
	export ArrayInits [csharp_statements]
		_
	by
		P [langTransCSharp] 
end function
