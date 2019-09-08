" Vim syntax file
"
" Language: Ragel
" Author: Adrian Thurston

syntax clear

"
" Outside code
"

" Comments
syntax region ocComment start="\/\*" end="\*\/"
syntax match ocComment "\/\/.*$"

" Anything preprocessor
syntax match ocPreproc "#\(.\|\\\n\)*$"
syntax region ocPreproc start="#" end="[^\\]$"

" Strings
syntax match ocLiteral "'\(\\.\|[^'\\]\)*'"
syntax match ocLiteral "\"\(\\.\|[^\"\\]\)*\""

" C/C++ Keywords
syntax keyword ocType unsigned signed void char short int long float double bool
syntax keyword ocType inline static extern register const volatile auto
syntax keyword ocType union enum struct class typedef
syntax keyword ocType namespace template typename mutable
syntax keyword ocKeyword break continue default do else for
syntax keyword ocKeyword goto if return switch while
syntax keyword ocKeyword new delete this using friend public private protected sizeof
syntax keyword ocKeyword throw try catch operator typeid
syntax keyword ocKeyword and bitor xor compl bitand and_eq or_eq xor_eq not not_eq
syntax keyword ocKeyword static_cast dynamic_cast

" Numbers
syntax match ocNumber "[0-9][0-9]*[Ll]\?[Ll]\?"
syntax match ocNumber "0x[0-9a-fA-F][0-9a-fA-F]*"

" Booleans
syntax keyword ocBoolean true false

" Identifiers
syntax match anyId "[a-zA-Z_][a-zA-Z_0-9]*"

" Inline code only
syntax keyword fsmType fpc fc fcurs fbuf fblen ftargs fstack
syntax keyword fsmKeyword fhold fgoto fcall fncall fret fnret fentry fnext fexec fbreak fnbreak

syntax cluster rlItems contains=rlComment,rlLiteral,rlAugmentOps,rlOtherOps,rlKeywords,rlWrite,rlCodeCurly,rlCodeSemi,rlNumber,anyId,rlLabelColon,rlExprKeywords,rlBuiltIns

syntax region machineSpec1 matchgroup=beginRL start="%%{" end="}%%" contains=@rlItems
syntax region machineSpec2 matchgroup=beginRL start="%%[^{]"rs=e-1 end="$" keepend contains=@rlItems
syntax region machineSpec2 matchgroup=beginRL start="%%$" end="$" keepend contains=@rlItems

" Comments
syntax match rlComment "#.*$" contained

" Literals
syntax match rlLiteral "'\(\\.\|[^'\\]\)*'[i]*" contained
syntax match rlLiteral "\"\(\\.\|[^\"\\]\)*\"[i]*" contained
syntax match rlLiteral /\/\(\\.\|[^\/\\]\)*\/[i]*/ contained
syntax match rlLiteral "\[\(\\.\|[^\]\\]\)*\]" contained

" Numbers
syntax match rlNumber "[0-9][0-9]*" contained
syntax match rlNumber "0x[0-9a-fA-F][0-9a-fA-F]*" contained

" Operators
syntax match rlAugmentOps "[>$%@]" contained
syntax match rlAugmentOps "<>\|<" contained
syntax match rlAugmentOps "[>\<$%@][!\^/*~]" contained
syntax match rlAugmentOps "[>$%]?" contained
syntax match rlAugmentOps "<>[!\^/*~]" contained
syntax match rlAugmentOps "=>" contained
syntax match rlOtherOps "->" contained

syntax match rlOtherOps ":>" contained
syntax match rlOtherOps ":>>" contained
syntax match rlOtherOps "<:" contained

" Keywords
" FIXME: Enable the range keyword post 5.17.
" syntax keyword rlKeywords machine action context include range contained
syntax keyword rlKeywords machine action context include import export prepush postpop contained
syntax keyword rlExprKeywords when inwhen outwhen err lerr eof from to contained

" Built-in rules
syntax keyword rlBuiltIns any ascii extend alpha digit alnum lower upper xdigit cntrl graph print punct space zlen empty contained

" Case Labels
syntax keyword caseLabelKeyword case contained
syntax cluster caseLabelItems contains=ocComment,ocPreproc,ocLiteral,ocType,ocKeyword,caseLabelKeyword,ocNumber,ocBoolean,anyId,fsmType,fsmKeyword
syntax match caseLabelColon "case" contains=@caseLabelItems
syntax match caseLabelColon "case[\t ]\+.*:$" contains=@caseLabelItems
syntax match caseLabelColon "case[\t ]\+.*:[^=:]"me=e-1 contains=@caseLabelItems

" Labels
syntax match ocLabelColon "^[\t ]*[a-zA-Z_][a-zA-Z_0-9]*[ \t]*:$" contains=anyLabel
syntax match ocLabelColon "^[\t ]*[a-zA-Z_][a-zA-Z_0-9]*[ \t]*:[^=:]"me=e-1 contains=anyLabel

syntax match rlLabelColon "[a-zA-Z_][a-zA-Z_0-9]*[ \t]*:$" contained contains=anyLabel
syntax match rlLabelColon "[a-zA-Z_][a-zA-Z_0-9]*[ \t]*:[^=:>]"me=e-1 contained contains=anyLabel
syntax match anyLabel "[a-zA-Z_][a-zA-Z_0-9]*" contained

" All items that can go in a code block.

syntax cluster inlineItems contains=rlCodeCurly,ocComment,ocPreproc,ocLiteral,ocType,ocKeyword,ocNumber,ocBoolean,ocLabelColon,anyId,fsmType,fsmKeyword,caseLabelColon

" Blocks of code. rlCodeCurly is recursive.
syntax region rlCodeCurly matchgroup=NONE start="{" end="}" contained contains=@inlineItems
syntax region rlCodeSemi matchgroup=Type start="\<alphtype\>" start="\<getkey\>" start="\<access\>" start="\<variable\>" matchgroup=NONE end=";" contained contains=@inlineItems

syntax region rlWrite matchgroup=Type start="\<write\>" matchgroup=NONE end="[;)]" contained contains=rlWriteKeywords,rlWriteOptions

syntax keyword rlWriteKeywords init data exec exports start error first_final contained
syntax keyword rlWriteOptions noerror nofinal noprefix noend nocs contained

"
" Sync at the start of machine specs.
"
" Match The ragel delimiters only if there quotes no ahead on the same line.
" On the open marker, use & to consume the leader.
syntax sync match ragelSyncPat grouphere NONE "^[^\'\"%]*%%{&^[^\'\"%]*"
syntax sync match ragelSyncPat grouphere NONE "^[^\'\"%]*%%[^{]&^[^\'\"%]*"
syntax sync match ragelSyncPat grouphere NONE "^[^\'\"]*}%%"

"
" Specifying Groups
"
hi link ocComment Comment
hi link ocPreproc Macro
hi link ocLiteral String
hi link ocType Type
hi link ocKeyword Keyword
hi link ocNumber Number
hi link ocBoolean Boolean
hi link rlComment Comment
hi link rlNumber Number
hi link rlLiteral String
hi link rlAugmentOps Keyword
hi link rlExprKeywords Keyword
hi link rlWriteKeywords Keyword
hi link rlWriteOptions Keyword
hi link rlKeywords Type
hi link fsmType Type
hi link fsmKeyword Keyword
hi link anyLabel Label
hi link caseLabelKeyword Keyword
hi link beginRL Type
hi link rlBuiltIns Constant
 
let b:current_syntax = "ragel"
