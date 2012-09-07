include "testcase.txl"

keys
	'boolean 'new
end keys


define ruby_statements
		[repeat ruby_lang_stmt]
end define

define ruby_lang_stmt
		[al_ragel_stmt]
	|	[ruby_expr_stmt]
	|	[ruby_if_stmt]
	|	[EX] 'do [IN] [NL] [ruby_statements] [EX] 'end [IN] [NL]
end define

define ruby_type_decl
		[al_type_decl]
	|	'boolean
end define

define ruby_expr_stmt
		[ruby_expr] '; [NL]
end define

define ruby_expr
		[ruby_term] [repeat ruby_expr_extend]
end define

define ruby_expr_extend
		[al_expr_op] [ruby_term]
end define

define ruby_term
		[al_term]
	|	[stringlit] [union]
	|	[id] [repeat ruby_dot_id]
	|	[SPOFF] [id] [repeat ruby_dot_id] '( [SPON] [ruby_args] ')
	|	[union]
end define

define ruby_dot_id
		'. [id]
end define

define ruby_args
		[list ruby_expr] 
end define

define ruby_sign
		'- | '+
end define

define ruby_if_stmt
		'if [ruby_expr] [NL] [IN]
			[ruby_statements] [EX]
		[opt ruby_else]
		'end [NL]
end define

define ruby_else
		'else [NL] [IN]
			[ruby_statements] [EX]
end define

define ruby_lang
		[ruby_statements]
		'%% [NL]
		[ruby_statements]
		[ragel_def]
end define

define program
		[lang_indep]
	|	[ruby_lang]
end define

redefine al_host_block
		'{ [NL] [IN] [al_statements] [EX] '} [NL]
	|	'{ [NL] [IN] [ruby_statements] [EX] '} [NL]
end define

redefine cond_action_stmt
		'action [id] '{ [al_expr] '} [NL]
	|	'action [id] '{ [ruby_expr] '} [NL]
end redefine

function initDecl1 VarDecl [al_variable_decl]
	deconstruct VarDecl
		'bool Id [id] ';
	replace [repeat ruby_lang_stmt]
	by
		Id '= 'false ';
end function

function initDecl2 VarDecl [al_variable_decl]
	deconstruct VarDecl
		'char Id [id] ';
	replace [repeat ruby_lang_stmt]
	by
		Id '= ''c' ';
end function

function initDecl3 VarDecl [al_variable_decl]
	deconstruct VarDecl
		'int Id [id] ';
	replace [repeat ruby_lang_stmt]
	by
		Id '= '0 ';
end function

function initDecl4 VarDecl [al_variable_decl]
	deconstruct VarDecl
		'ptr Id [id] ';
	replace [repeat ruby_lang_stmt]
	by
		Id '= '-1 ';
end function

function initDecl5 VarDecl [al_variable_decl]
	deconstruct VarDecl
		Type [al_type_decl] Id [id] Union [union] ';
	replace [repeat ruby_lang_stmt]
	by
		Id '= '[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0] ';
end function


function alStmtToRuby1 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		VarDecl [al_variable_decl]
	deconstruct VarDecl
		Type [al_type_decl] Id [id] OptUnion [opt union] ';
	replace [repeat ruby_lang_stmt]
	by
		_ [initDecl1 VarDecl] [initDecl2 VarDecl] 
			[initDecl3 VarDecl] [initDecl4 VarDecl]
			[initDecl5 VarDecl]
end function

rule alTermToRuby1
	replace [al_term]
		'first_token_char
	by
		'data '[ts]
end rule

rule alTermToRuby2
	replace [al_term]
		'< _ [al_type_decl] '> '( AlExpr [al_expr] ')
	by
		'( AlExpr ')
end rule

function alTermToRuby
	replace [al_term]
		AlTerm [al_term]
	by
		AlTerm
			[alTermToRuby1]
			[alTermToRuby2]
end function

function alExprExtendToRuby AlExprExtend [repeat al_expr_extend]
	deconstruct AlExprExtend
		Op [al_expr_op] Term [al_term] Rest [repeat al_expr_extend]
	construct RubyRest [repeat ruby_expr_extend]
		_ [alExprExtendToRuby Rest]
	replace [repeat ruby_expr_extend]
	by
		Op Term [alTermToRuby] RubyRest
end function

% Note: this doesn't go into the ( al_expr ) form of al_term.
function alExprToRuby AlExpr [al_expr]
	deconstruct AlExpr
		ALTerm [al_term] AlExprExtend [repeat al_expr_extend]
	construct RubyExprExtend [repeat ruby_expr_extend]
		_ [alExprExtendToRuby AlExprExtend]
	construct Result [opt ruby_expr]
		ALTerm [alTermToRuby] RubyExprExtend
	replace [opt ruby_expr]
	by
		Result 
end function

function alStmtToRuby2 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		AlExpr [al_expr] ';
	construct OptRubyExpr [opt ruby_expr]
		_ [alExprToRuby AlExpr]
	deconstruct OptRubyExpr
		RubyExpr [ruby_expr]
	replace [repeat ruby_lang_stmt]
	by
		RubyExpr ';
end function

function liftBlock
	replace [repeat ruby_lang_stmt]
		'do Block [repeat ruby_lang_stmt] 'end
	by
		Block
end function

function alOptElseRuby AlOptElse [opt al_else]
	deconstruct AlOptElse
		'else 
			AlSubStmt [action_lang_stmt]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct RubySubStmts [repeat ruby_lang_stmt]
		_ [alToRuby AlSubStmts]
	deconstruct RubySubStmts
		RubySubStmt [ruby_lang_stmt]
	replace [opt ruby_else]
	by
		'else 
			RubySubStmts [liftBlock]
end function

function alStmtToRuby3 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'if '( AlExpr [al_expr] ')
			AlSubStmt [action_lang_stmt]
		AlOptElse [opt al_else]
	construct OptRubyExpr [opt ruby_expr]
		_ [alExprToRuby AlExpr]
	deconstruct OptRubyExpr
		RubyExpr [ruby_expr]
	construct AlSubStmts [repeat action_lang_stmt]
		AlSubStmt
	construct RubySubStmts [repeat ruby_lang_stmt]
		_ [alToRuby AlSubStmts]
	construct OptRubyElse [opt ruby_else]
		_ [alOptElseRuby AlOptElse]
	replace [repeat ruby_lang_stmt]
	by
		'if RubyExpr
			RubySubStmts [liftBlock]
		OptRubyElse
		'end
end function

function alStmtToRuby4a AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printi Id [id] ';
	replace [repeat ruby_lang_stmt]
	by
		'print '( Id ') ';
end function

function alStmtToRuby4b AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'prints String [stringlit] ';
	replace [repeat ruby_lang_stmt]
	by
		'print '( String ') ';
end function

function alStmtToRuby4c AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'printb Id [id] ';
	replace [repeat ruby_lang_stmt]
	by
		'_a = Id '[0..pos-1] ';
		'print '( '_a '. 'pack '( '"c*" ')  ') ';
end function

function alStmtToRuby4d AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'print_token ';
	replace [repeat ruby_lang_stmt]
	by
		'_m = 'data '[ts..te-1] ';
		'print '( '_m '. 'pack '( '"c*" ') ') ';
end function

function alStmtToRuby5 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		'{ AlSubStmts [repeat action_lang_stmt] '}
	construct RubySubStmts [repeat ruby_lang_stmt]
		_ [alToRuby AlSubStmts]
	replace [repeat ruby_lang_stmt]
	by
		'do RubySubStmts 'end
end function

function alStmtToRuby6 AlStmt [action_lang_stmt]
	deconstruct AlStmt
		RagelStmt [al_ragel_stmt]
	replace [repeat ruby_lang_stmt]
	by
		RagelStmt
end function

rule fixCharLit
	replace $ [al_term]
		CharLit [charlit]
	construct BaseId [id]
		'id
	construct Id [id]
		BaseId [unquote CharLit]
	construct EmptyString [stringlit]
		'""
	construct Repl [stringlit]
		EmptyString [quote Id]
	by
		Repl '[0]
end rule


function alToRuby AlStmts [repeat action_lang_stmt]
	deconstruct AlStmts
		FirstStmt [action_lang_stmt] Rest [repeat action_lang_stmt]
	construct RubyFirst [repeat ruby_lang_stmt]
		_ 
			[alStmtToRuby1 FirstStmt]
			[alStmtToRuby2 FirstStmt]
			[alStmtToRuby3 FirstStmt]
			[alStmtToRuby4a FirstStmt]
			[alStmtToRuby4b FirstStmt]
			[alStmtToRuby4c FirstStmt]
			[alStmtToRuby4d FirstStmt]
			[alStmtToRuby5 FirstStmt]
			[alStmtToRuby6 FirstStmt]
			[fixCharLit]
	construct RubyRest [repeat ruby_lang_stmt]
		_ [alToRuby Rest]
	replace [repeat ruby_lang_stmt]
	by
		RubyFirst [. RubyRest]
end function

rule actionTransRuby
	replace [al_host_block]
		'{ AlStmts [repeat action_lang_stmt] '}
	construct RubyStmts [repeat ruby_lang_stmt]
		_ [alToRuby AlStmts]
	by
		'{ RubyStmts '}
end rule

rule condTransRuby
	replace [cond_action_stmt]
		'action Id [id] '{ AlExpr [al_expr] '}
	construct OptRubyExpr [opt ruby_expr]
		_ [alExprToRuby AlExpr]
	deconstruct OptRubyExpr
		RubyExpr [ruby_expr]
	by
		'action Id '{ RubyExpr '}
end rule

rule lowercaseMachine
	replace $ [machine_stmt]
		'machine Id [id] ';
	by
		'machine Id [tolower] ';
end rule

function langTransRuby
	replace [program]
		Definitions [repeat action_lang_stmt]
		'%%
		Initializations [repeat action_lang_stmt]
		RagelDef [ragel_def]
	construct RubyDefinitions [repeat ruby_lang_stmt]
		_ [alToRuby Definitions]
	construct RubyInitializations [repeat ruby_lang_stmt]
		_ [alToRuby Initializations]
	construct NewRagelDef [ragel_def]
		RagelDef [actionTransRuby] [condTransRuby] [lowercaseMachine]
	import ArrayInits [ruby_statements]
		ArrayInitStmts [repeat ruby_lang_stmt]
	by
		RubyDefinitions
		'%%
		ArrayInitStmts [. RubyInitializations]
		NewRagelDef
end function

function main
	replace [program]
		P [program]
	export ArrayInits [ruby_statements]
		_
	by
		P [langTransRuby] 
end function
