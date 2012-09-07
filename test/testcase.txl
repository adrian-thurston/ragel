comments
		'#
end comments

tokens
		union "\[[(\\\c)#\]]*\]"
end tokens

compounds
		'%% '%%{ '}%% '== ':= '-> '<> '>= '<= '=>
		'|* '*|
		'>! '<! '$! '%! '@! '<>!
		'>/ '</ '$/ '%/ '@/ '<>/
end compounds

keys
	'int 'bool 'true 'false 'char 'ptr
	'if 'else 'printi 'prints 'printb 'print_token
	'fc 'fpc 'fbreak 'fgoto 'fcall 'fret 'fhold 'fexec
	'machine 'alphtype 'action
	'first_token_char
end keys

define lang_indep
		[al_statements]
		'%% [NL]
		[al_statements]
		[ragel_def]
end define

define ragel_def
		'%%{ [NL] [IN]
			[ragel_program]
		[EX] '}%% [NL]
end define

define ragel_program
		[repeat statement]
end define 

define statement
		[machine_stmt]
	|	[alphtype_stmt]
	|	[action_stmt]
	|	[cond_action_stmt]
	|	[machine_def]
	|	[machine_inst]
end define 

define machine_stmt
		'machine [id] '; [NL]
end define

define alphtype_stmt
		'alphtype [repeat id] '; [NL]
end define

define action_stmt
		'action [id] [al_host_block]
end define

define cond_action_stmt
		'action [id] '{ [al_expr] '} [NL]
end define

define al_statements
		[repeat action_lang_stmt]
end define

define action_lang_stmt
		[al_ragel_stmt]
	|	[al_variable_decl]
	|	[al_expr_stmt]
	|	[al_if_stmt]
	|	[al_print_stmt]
	|	'{ [al_statements] '}
end define

define al_print_stmt
		[print_cmd] [al_expr] '; [NL]
	|	'print_token '; [NL]
end define

define print_cmd
		'printi | 'prints | 'printb
end define

define al_variable_decl
		[al_type_decl] [id] [opt union] '; [NL]
end define

define al_array_decl
		'[ [number] ']
end define

define al_type_decl
		'int | 'bool | 'char | 'ptr
end define 

define al_expr_stmt
		[al_expr] '; [NL]
end define

define al_expr
		[al_term] [repeat al_expr_extend]
end define

define al_expr_extend
		[al_expr_op] [al_term]
end define

define al_expr_op
		'= | '+ | '- | '* | '/ | '== | '<= | '>= | '< | '>
end define

define al_term
		[al_term_base] [opt union]
end define

define al_term_base
		[id]
	|	[SPOFF] [id] '( [SPON] [al_expr] ')
	|	[opt al_sign] [number]
	|	[stringlit] 
	|	[charlit] 
	|	'fc
	|	'true
	|	'false
	|	'( [al_expr] ')
	|	'< [SPOFF] [al_type_decl] '> '( [SPON] [al_expr] ')
	|	'first_token_char
end define

define al_sign
		'- | '+
end define

define al_if_stmt
		'if '( [al_expr] ') [NL] [IN]
			[action_lang_stmt] [EX]
		[opt al_else]
end define

define al_else
		'else [NL] [IN]
			[action_lang_stmt] [EX]
end define

define al_ragel_stmt
		'fbreak '; [NL]
	|	'fhold '; [NL]
	|	'fexec [repeat al_expr] '; [NL]
	|	'fnext [id] '; [NL]
	|	'fgoto [id] '; [NL]
	|	'fcall [id] '; [NL]
	|	'fnext '* [repeat al_expr] '; [NL]
	|	'fgoto '* [repeat al_expr] '; [NL]
	|	'fcall '* [repeat al_expr] '; [NL]
	|	'fret '; [NL]
end define

define machine_def
		[id] '= [machine_expr] '; [NL]
end define

define machine_inst
		[id] ':= [machine_expr] '; [NL]
end define

define machine_expr
		[repeat machine_expr_item]
end define

define scanner_item
		[repeat machine_expr_item] '; [NL]
end define

define machine_expr_item
		[action_embed] [al_host_block]
	|	'|* [repeat scanner_item] '*|
	|	[not ';] [not '*|] [token]
end define

define al_host_block
		'{ [NL] [IN] [al_statements] [EX] '} [NL]
end define

define action_embed
		'> | '$ | '@ | '% | 
		'$! | '=>
end define

