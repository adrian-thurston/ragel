" Vim syntax file
"
" Language: Colm
" Author: Adrian Thurston

syntax clear

"
" Regular Language Types
"

" Identifiers
syntax match rlId "[a-zA-Z_][a-zA-Z_0-9]*" contained

" Literals
syntax match rlLiteral "'\(\\.\|[^'\\]\)*'[i]*" contained
syntax match rlLiteral "\"\(\\.\|[^\"\\]\)*\"[i]*" contained
syntax match rlLiteral "\[\(\\.\|[^\]\\]\)*\]" contained

" Numbers
syntax match rlNumber "[0-9][0-9]*" contained
syntax match rlNumber "0x[0-9a-fA-F][0-9a-fA-F]*" contained

" Operators
syntax match rlOtherOps ":>" contained
syntax match rlOtherOps ":>>" contained
syntax match rlOtherOps "<:" contained

syntax cluster rlTypes contains=rlId,rlLiteral,rlNumber,rlOtherOps
syntax region rlTypeRegion matchgroup=regionDelimiter start="/" end="/"
	\ contains=@rlTypes

syntax region cflTypeRegion matchgroup=regionDelimiter start="\[" end="\]"
	\ contains=cflTypeRegion,patRegion,otLit,typeKeywords,varCapture,qual
syntax region patRegion matchgroup=String start="\"" end="\"" end="\n"
	\ contains=char,cflTypeRegion

syntax match char "[^\"\[]" contained
syntax match char "\\." contained

syntax match otLit "\~.*$"
syntax match otLit "'\(\\.\|[^'\\]\)*\('[i]*\)\?"
syntax match otLit "`[^ \t\r\]]\+"
syntax match otLit "`\]"

"
" Other stuff
"

syntax match tlComment "#.*$"
syntax match tlIdentifier "[a-zA-Z_][a-zA-Z_0-9]*"
syntax match tlNumber "[0-9][0-9]*"
syntax match tlNumber "nil"
syntax match tlNumber "true"
syntax match tlNumber "false"

syntax keyword Type
	\ commit include literal iter
	\ namespace lex reducefirst global include export
	\ construct cons parse parse_stop match require send
	\ preeof left right nonassoc prec context alias
	\ end

syntax keyword typeKeywords
	\ int str bool any ref ptr void

syntax keyword Keyword
	\ reject else elsif return yield for while if
	\ typeid in break 
	\ new deref ni cast

syntax match tokenName "[a-zA-Z_][a-zA-Z_0-9]*" contained
syntax match varCapture "[a-zA-Z_][a-zA-Z_0-9]*:" 
syntax match qual "[a-zA-Z_][a-zA-Z_0-9]*::" 

syntax region defTypes matchgroup=defKeywords
	\ start="\<rl\>" start="\<def\>" start="\<token\>" start="\<ignore\>"
	\ matchgroup=Function end="[a-zA-Z_][a-zA-Z0-9_]*" end="/"me=e-1

syntax sync match colmSyncPat grouphere NONE "([{}]|\<token\>|\<ignore\>|\<def\>)"

"
" Specifying Groups
"
hi link tlComment Comment
hi link tlNumber Number
hi link otLit String
hi link rlNumber Number
hi link rlLiteral String
hi link defKeywords Type
hi link typeKeywords Type
hi link regionDelimiter Type
hi link char String
hi link tokenName Function
hi link varCapture Identifier
 
let b:current_syntax = "colm"
