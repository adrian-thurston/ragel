include "testcase.txl"

define program
		[lang_indep]
	|	'yes
	|	'no
end define

rule findEof1
	match [machine_expr_item]
		'>/
end rule

rule findEof2
	match [machine_expr_item]
		'</
end rule

rule findEof3
	match [machine_expr_item]
		'$/
end rule

rule findEof4
	match [machine_expr_item]
		'%/
end rule

rule findEof5
	match [machine_expr_item]
		'@/
end rule

rule findEof6
	match [machine_expr_item]
		'<>/
end rule

rule findEof7
	match [repeat machine_expr_item]
		'> 'eof _ [repeat machine_expr_item]
end rule

rule findEof8
	match [repeat machine_expr_item]
		'< 'eof _ [repeat machine_expr_item]
end rule

rule findEof9
	match [repeat machine_expr_item]
		'$ 'eof _ [repeat machine_expr_item]
end rule

rule findEof10
	match [repeat machine_expr_item]
		'% 'eof _ [repeat machine_expr_item]
end rule

rule findEof11
	match [repeat machine_expr_item]
		'@ 'eof _ [repeat machine_expr_item]
end rule

rule findEof12
	match [repeat machine_expr_item]
		'<> 'eof _ [repeat machine_expr_item]
end rule

rule findScanner
	match [machine_expr_item]
		'|* _ [repeat scanner_item] '*|
end rule

function findEof P [program]
	replace [program]
		_ [program]
	where
		P 
			[findEof1] [findEof2] [findEof3]
			[findEof4] [findEof5] [findEof6]
			[findEof7] [findEof8] [findEof9]
			[findEof10] [findEof11] [findEof12]
			[findScanner]
	by
		'yes
end function
		
function main
	replace [program]
		P [program]
	construct NewP [program]
		'no
	by
		NewP [findEof P]
end function
