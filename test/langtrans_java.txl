include "testcase.txl"

keys
	'boolean 'new
end keys


define java_statements
		[repeat java_lang_stmt]
end define

define java_lang_stmt
		[al_ragel_stmt]
	|	[java_variable_decl]
	|	[java_expr_stmt]
	|	[java_if_stmt]
	|	[EX] '{ [IN] [NL] [java_statements] [EX] '} [IN] [NL]
end define

define java_variable_decl
		[java_type_decl] [id] [opt union] '; [NL]
end define

define java_type_decl
		[al_type_decl]
	|	'boolean
	|	'String
end define

define java_expr_stmt
		[java_expr] '; [NL]
end define

define java_expr
		[java_term] [repeat java_expr_extend]
end define

define java_expr_extend
		[al_expr_op] [java_term]
end define

define java_term
		[al_term]
	|	[id] [repeat java_dot_id]
	|	[id] [repeat java_dot_id] '( [java_args] ')
	|	'new [java_type_decl] [union]
	|	'new [java_type_decl] '( [java_args] ') 
end define

define java_dot_id
		'. [id]
end define

define java_args
		[list java_expr] 
end define

define java_sign
		'- | '+
end define

define java_if_stmt
		'if '( [java_expr] ') [NL] [IN]
			[java_lang_stmt] [EX]
		[opt java_else]
end define

define java_else
		'else [NL] [IN]
			[java_lang_stmt] [EX]
end define

define java_lang
		[java_statements]
		'%% [NL]
		[java_statements]
		[ragel_def]
end define

define program
		[lang_indep]
	|	[java_lang]
end define

redefine al_host_block
		'{ [NL] [IN] [al_statements] [EX] '} [NL]
	|	'{ [NL] [IN] [java_statements] [EX] '} [NL]
end define

redefine cond_action_stmt
		'action [id] '{ [al_expr] '} [NL]
	|	'action [id] '{ [java_expr] '} [NL]
end redefine


function clearUnion Type [java_type_decl] Id [id] 
	replace [opt union]
		Union [union]
	import ArrayInits [java_statements]
		Stmts [repeat java_lang_stmt]
	export ArrayInits 
		Id '= 'new Type Union '; Stmts
	by
		'[]
end function

rule boolTypes
	replace [java_type_decl]
		'bool
	by
		'boolean
end rule

rule ptrTypes
	replace [al_type_decl]
		'ptr
	by
		'int
end rule

function alStmtToJava1 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		VarDecl [al_variable_decl]
	deconstruct VarDecl
		Type [al_type_decl] Id [id] OptUnion [opt union] ';
	construct JavaType [java_type_decl]
		Type
	construct Result [java_variable_decl]
		JavaType [boolTypes] [ptrTypes] Id OptUnion [clearUnion JavaType Id] ';
	replace [repeat java_lang_stmt]
	by
		Result
end function

rule alTermToJava1
	replace [al_term]
		'first_token_char
	by
		'data '[ts]
end rule

rule alTermToJava2
	replace [al_term]
		'< _ [al_type_decl] '> '( AlExpr [al_expr] ')
	by
		'( AlExpr ')
end rule

function alTermToJava
	replace [al_term]
		AlTerm [al_term]
	by
		AlTerm
			[alTermToJava1]
			[alTermToJava2]
end function

function alExprExtendToJava AlExprExtend [repeat al_expr_extend]
	deconstruct AlExprExtend
		Op [al_expr_op] Term [al_term] Rest [repeat al_expr_extend]
	construct JavaRest [repeat java_expr_extend]
		_ [alExprExtendToJava Rest]
	replace [repeat java_expr_extend]
	by
		Op Term [alTermToJava] JavaRest
end function

function alExprToJava AlExpr [al_expr]
	deconstruct AlExpr
		ALTerm [al_term] AlExprExtend [repeat al_expr_extend]
	construct JavaExprExtend [repeat java_expr_extend]
		_ [alExprExtendToJava AlExprExtend]
	construct Result [opt java_expr]
		ALTerm [alTermToJava] JavaExprExtend
	replace [opt java_expr]
	by
		Result 
end function

function alStmtToJava2 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		AlExpr [al_expr] ';
	construct OptJavaExpr [opt java_expr]
		_ [alExprToJava AlExpr]
	deconstruct OptJavaExpr
		JavaExpr [java_expr]
	replace [repeat java_lang_stmt]
	by
		JavaExpr ';
end function

function alOptElseJava AlOptElse [opt al_else]
	deconstruct AlOptElse
		'else 
			AlSubStmt [action_lang_stmt]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct JavaSubStmts [repeat java_lang_stmt]
		_ [alToJava AlSubStmts]
	deconstruct JavaSubStmts
		JavaSubStmt [java_lang_stmt]
	replace [opt java_else]
	by
		'else 
			JavaSubStmt
end function

function alStmtToJava3 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'if '( AlExpr [al_expr] ')
			AlSubStmt [action_lang_stmt]
		AlOptElse [opt al_else]
	construct OptJavaExpr [opt java_expr]
		_ [alExprToJava AlExpr]
	deconstruct OptJavaExpr
		JavaExpr [java_expr]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct JavaSubStmts [repeat java_lang_stmt]
		_ [alToJava AlSubStmts]
	deconstruct JavaSubStmts
		JavaSubStmt [java_lang_stmt]
	construct OptJavaElse [opt java_else]
		_ [alOptElseJava AlOptElse]
	replace [repeat java_lang_stmt]
	by
		'if '( JavaExpr ')
			JavaSubStmt
		OptJavaElse
end function

function alStmtToJava4a AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printi Id [id] ';
	replace [repeat java_lang_stmt]
	by
		'System '. 'out '. 'print '( Id ');
end function

function alStmtToJava4b AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'prints String [stringlit] ';
	replace [repeat java_lang_stmt]
	by
		'System '. 'out '. 'print '( String ');
end function

function alStmtToJava4c AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printb Id [id] ';
	replace [repeat java_lang_stmt]
	by
		'_s '= 'new 'String '( Id ', '0 ', 'pos ') ';
		'System '. 'out '. 'print '( '_s ');
end function

function alStmtToJava4d AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'print_token ';
	replace [repeat java_lang_stmt]
	by
		'_s '= 'new 'String '( 'data ', 'ts ', 'te '- 'ts ') ';
		'System '. 'out '. 'print '( '_s ');
end function

function alStmtToJava5 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'{ AlSubStmts [repeat action_lang_stmt] '}
	construct JavaSubStmts [repeat java_lang_stmt]
		_ [alToJava AlSubStmts]
	replace [repeat java_lang_stmt]
	by
		'{ JavaSubStmts '}
end function

function alStmtToJava6 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		RagelStmt [al_ragel_stmt]
	replace [repeat java_lang_stmt]
	by
		RagelStmt
end function


function alToJava AlStmts [repeat action_lang_stmt]
	deconstruct AlStmts
		FirstStmt [action_lang_stmt] Rest [repeat action_lang_stmt]
	construct JavaFirst [repeat java_lang_stmt]
		_ 
			[alStmtToJava1 FirstStmt]
			[alStmtToJava2 FirstStmt]
			[alStmtToJava3 FirstStmt]
			[alStmtToJava4a FirstStmt]
			[alStmtToJava4b FirstStmt]
			[alStmtToJava4c FirstStmt]
			[alStmtToJava4d FirstStmt]
			[alStmtToJava5 FirstStmt]
			[alStmtToJava6 FirstStmt]
	construct JavaRest [repeat java_lang_stmt]
		_ [alToJava Rest]
	replace [repeat java_lang_stmt]
	by
		JavaFirst [. JavaRest]
end function

rule actionTransJava
	replace [al_host_block]
		'{ AlStmts [repeat action_lang_stmt] '}
	construct JavaStmts [repeat java_lang_stmt]
		_ [alToJava AlStmts]
	by
		'{ JavaStmts '}
end rule

rule condTransJava
	replace [cond_action_stmt]
		'action Id [id] '{ AlExpr [al_expr] '}
	construct OptJavaExpr [opt java_expr]
		_ [alExprToJava AlExpr]
	deconstruct OptJavaExpr
		JavaExpr [java_expr]
	by
		'action Id '{ JavaExpr '}
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

function langTransJava
	replace [program]
		Definitions [repeat action_lang_stmt]
		'%%
		Initializations [repeat action_lang_stmt]
		RagelDef [ragel_def]
	construct JavaDefinitions [repeat java_lang_stmt]
		_ [alToJava Definitions]
	construct JavaInitializations [repeat java_lang_stmt]
		_ [alToJava Initializations]
	construct NewRagelDef [ragel_def]
		RagelDef [actionTransJava] [condTransJava] [machineName]
	import ArrayInits [java_statements]
		ArrayInitStmts [repeat java_lang_stmt]
	by
		JavaDefinitions
		'%%
		ArrayInitStmts [. JavaInitializations]
		NewRagelDef
end function

function main
	replace [program]
		P [program]
	export ArrayInits [java_statements]
		_
	by
		P [langTransJava] 
end function
