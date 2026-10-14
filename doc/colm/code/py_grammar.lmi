# A part of Python 2's grammar, cut down from grammar/python/python.lm in
# the colm-suite repository.

# tag::globals[]
# The indentation of each block that is open, the innermost on top.
global Indents: list<int> = new list<int>()
Indents->push( 0 )

# Comments and blank lines, kept until the next newline.
global Held: str = ""
# end::globals[]

# tag::tokens[]
lex
	literal `def `if `elif `else `while `for `in `return `pass `print
	literal `( `) `[ `] `, `: `. `= `+ `- `* `/ `% `< `> `== `!=

	token id /[a-zA-Z_] [a-zA-Z_0-9]*/
	token number /[0-9]+/
	token string /'"' [^"\n]* '"' | "'" [^'\n]* "'"/

	ignore ws /[ \t]+/
# end::tokens[]

# tag::indent[]
	# A comment goes after the next newline.
	token comment /'#' [^\n]*/
	{
		Held = Held + input->pull( match_length )
	}

	# So does a line that is blank or holds only a comment. The newline
	# that ends it is scanned again.
	token blank_line /'\n' [ \t]* ('#' [^\n]*)? '\n'/
	{
		Held = Held + input->pull( match_length - 1 )
	}

	token NEWLINE //
	token INDENT //
	token DEDENT //

	# A newline, and the indentation of the line after it.
	token indentation /'\n' [ \t]*/
	{
		Text: str = Held + input->pull( match_length )
		Held = ""
		input->push_ignore( make_token( typeid<ws>, Text ) )

		Level: int = match_length - 1
		if ( Level > Indents->top ) {
			Indents->push( Level )
			input->push( make_token( typeid<INDENT>, "" ) )
		}
		else {
			while ( Level < Indents->top ) {
				Indents->pop()
				input->push( make_token( typeid<DEDENT>, "" ) )
			}
		}
		input->push( make_token( typeid<NEWLINE>, "" ) )
	}
end
# end::indent[]

# tag::statements[]
def program
	[line*]

def line
	[statement]
|	[NEWLINE]

def statement
	[simple_stmt NEWLINE]
|	[compound_stmt]

def simple_stmt
	[print_stmt]
|	[`pass]
|	[`return expression_list?]
|	[expression_list `= expression_list]
|	[expression_list]

def print_stmt
	[`print call]
|	[`print expression_list `,]
|	[`print expression_list]
|	[`print]

def compound_stmt
	[`if expression `: suite elif_part* else_part?]
|	[`while expression `: suite]
|	[`for expression_list `in expression_list `: suite]
|	[`def id `( parameters? `) `: suite]

def elif_part
	[`elif expression `: suite]

def else_part
	[`else `: suite]

def parameters
	[id (`, id)*]

def suite
	[simple_stmt NEWLINE]
|	[NEWLINE INDENT statement+ DEDENT]
# end::statements[]

# tag::expressions[]
def expression_list
	[expression (`, expression)*]

def expression
	[arith comp_op arith]
|	[arith]

def comp_op
	[`<] | [`>] | [`==] | [`!=]

def arith
	[arith `+ term]
|	[arith `- term]
|	[term]

def term
	[term `* factor]
|	[term `/ factor]
|	[term `% factor]
|	[factor]

def factor
	[`- factor]
|	[primary]

def primary
	[atom trailer*]

def trailer
	[call]
|	[`[ expression `]]
|	[`. id]

def call
	[`( `)]
|	[`( expression_list `)]
|	[`( expression_list `, keywords `)]
|	[`( keywords `)]

def keywords
	[keyword (`, keyword)*]

def keyword
	[id `= expression]

def atom
	[id]
|	[number]
|	[string]
|	[`( expression_list `)]
|	[`[ expression_list? `]]
# end::expressions[]
