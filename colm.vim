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
	\ construct cons parse parse_tree parse_stop reduce 
	\ match require send send_tree
	\ preeof left right nonassoc prec context struct alias
	\ end eos print nonterm

syntax keyword typeKeywords
	\ int str bool any ref ptr void list_el map_el

syntax keyword Keyword
	\ reject else elsif return yield for while if
	\ typeid in break 
	\ new deref ni cast switch case default

syntax match tokenName "[a-zA-Z_][a-zA-Z_0-9]*" contained
syntax match varCapture "[a-zA-Z_][a-zA-Z_0-9]*:" 
syntax match qual "[a-zA-Z_][a-zA-Z_0-9]*::" 

syntax region defTypes matchgroup=defKeywords
	\ start="\<rl\>" start="\<def\>" start="\<token\>" start="\<ignore\>"
	\ matchgroup=Function end="[a-zA-Z_][a-zA-Z0-9_]*" end="/"me=e-1

syntax region redTypes matchgroup=redBlock
	\ start="\<reduction\>" 
	\ contains=externalCode,extComment
	\ end="\<end\>"

syntax match extComment "#.*$" contained

syntax region externalCode contained 
	\ start="{"
	\ contains=@redItems
	\ end="}"

syntax cluster redItems contains=redRef,redType,redKeyword,redNumber,redIdentifier,redLiteral,redComment,externalCode

syntax region redComment start="\/\*" end="\*\/" contained
syntax match redComment "\/\/.*$" contained

syntax match redLiteral "'\(\\.\|[^'\\]\)*'" contained
syntax match redLiteral "\"\(\\.\|[^\"\\]\)*\"" contained

syntax match redRef "\$\$" contained
syntax match redRef "\$[a-zA-Z_][a-zA-Z0-9_]*" contained
syntax match redRef "@[a-zA-Z_][a-zA-Z0-9_]*" contained
syntax match redRef "\$[0-9]\+" contained
syntax match redRef "@[0-9]\+" contained

syntax match redNumber "[0-9][0-9]*" contained
syntax match redNumber "true" contained
syntax match redNumber "false" contained

syntax match redIdentifier "[a-zA-Z_][a-zA-Z_0-9]*" contained

syntax keyword redType unsigned signed void char short int long float double bool
syntax keyword redType inline static extern register const volatile auto
syntax keyword redType union enum struct class typedef
syntax keyword redType namespace template typename mutable
syntax keyword redKeyword break continue default do else for
syntax keyword redKeyword goto if return switch while
syntax keyword redKeyword new delete this using friend public private protected sizeof
syntax keyword redKeyword throw try catch operator typeid
syntax keyword redKeyword and bitor xor compl bitand and_eq or_eq xor_eq not not_eq
syntax keyword redKeyword static_cast dynamic_cast

syntax sync match colmSyncPat grouphere NONE "([{}]|\<reduction\>|\<token\>|\<ignore\>|\<def\>)"


"
" Specifying Groups
"
hi link tlComment Comment
hi link tlNumber Number
hi link otLit String
hi link rlNumber Number
hi link rlLiteral String
hi link defKeywords Type
hi link redBlock Type
hi link typeKeywords Type
hi link regionDelimiter Type
hi link char String
hi link tokenName Function
hi link varCapture Identifier

hi link extComment Comment
hi link redType Type
hi link redKeyword Keyword
hi link redLiteral String
hi link redRef Function
hi link redComment Comment
hi link redNumber Number
 
let b:current_syntax = "colm"
