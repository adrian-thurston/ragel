/*
 * @LANG: c++
 *
 * Test works with split code gen.
 */

#include "cppscan1.h"

%%{
	machine Scanner;
	access fsm->;

	action pass { fsm->pass(fc); }
	action buf { fsm->buf(fc); }

	action emit_slit { fsm->token( TK_Slit ); }
	action emit_dlit { fsm->token( TK_Dlit ); }
	action emit_id { fsm->token( TK_Id ); }
	action emit_integer_decimal { fsm->token( TK_IntegerDecimal ); }
	action emit_integer_octal { fsm->token( TK_IntegerOctal ); }
	action emit_integer_hex { fsm->token( TK_IntegerHex ); }
	action emit_float { fsm->token( TK_Float ); }
	action emit_symbol { fsm->token( fsm->tokBuf.data[0] ); }
	action tokst { fsm->tokStart = fsm->col; }

	# Single and double literals.
	slit = ( 'L'? ( "'" ( [^'\\\n] | /\\./ )* "'" ) $buf ) >tokst %emit_slit;
	dlit = ( 'L'? ( '"' ( [^"\\\n] | /\\./ )* '"' ) $buf ) >tokst %emit_dlit;

	# Identifiers
	id = ( [a-zA-Z_] [a-zA-Z0-9_]* ) >tokst $buf %emit_id;

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];
	float = 
		( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) >tokst $buf %emit_float;
	
	# Integer decimal. Leading part buffered by float.
	integer_decimal = ( ( '0' | [1-9] [0-9]* ) [ulUL]{0,3} $buf ) %emit_integer_decimal;

	# Integer octal. Leading part buffered by float.
	integer_octal = ( '0' [0-9]+ [ulUL]{0,2} $buf ) %emit_integer_octal;

	# Integer hex. Leading 0 buffered by float.
	integer_hex = ( '0' ( 'x' [0-9a-fA-F]+ [ulUL]{0,2} ) $buf ) %emit_integer_hex;

	# Only buffer the second item, first buffered by symbol. */
	namesep = '::' @buf %{fsm->token( TK_NameSep );};
	deqs = '==' @buf %{fsm->token( TK_EqualsEquals );};
	neqs = '!=' @buf %{fsm->token( TK_NotEquals );};
	and_and = '&&' @buf %{fsm->token( TK_AndAnd );};
	or_or = '||' @buf %{fsm->token( TK_OrOr );};
	mult_assign = '*=' @buf %{fsm->token( TK_MultAssign );};
	percent_assign = '%=' @buf %{fsm->token( TK_PercentAssign );};
	plus_assign = '+=' @buf %{fsm->token( TK_PlusAssign );};
	minus_assign = '-=' @buf %{fsm->token( TK_MinusAssign );};
	amp_assign = '&=' @buf %{fsm->token( TK_AmpAssign );};
	caret_assign = '^=' @buf %{fsm->token( TK_CaretAssign );};
	bar_assign = '|=' @buf %{fsm->token( TK_BarAssign );};
	plus_plus = '++' @buf %{fsm->token( TK_PlusPlus );};
	minus_minus = '--' @buf %{fsm->token( TK_MinusMinus );};
	arrow = '->' @buf %{fsm->token( TK_Arrow );};
	arrow_star = '->*' @buf %{fsm->token( TK_ArrowStar );};
	dot_star = '.*' @buf %{fsm->token( TK_DotStar );};

	# Buffer both items. *
	div_assign = '/=' @{fsm->buf('/');fsm->buf(fc);} %{fsm->token( TK_DivAssign );};

	# Double dot is sent as two dots.
	dot_dot = '..' %{fsm->token('.'); fsm->buf('.'); fsm->token('.');};

	# Three char compounds, first item already buffered. */
	dot_dot_dot = '...' %{fsm->buf('.'); fsm->buf('.'); fsm->token( TK_DotDotDot );};

	# All compunds
	compound = namesep | deqs | neqs | and_and | or_or | mult_assign |
			div_assign | percent_assign | plus_assign | minus_assign |
			amp_assign | caret_assign | bar_assign | plus_plus | minus_minus |
			arrow | arrow_star | dot_star | dot_dot | dot_dot_dot;

	# Single char symbols.
	symbol = 
		( punct - [./_"'] ) >tokst $buf %emit_symbol |
		# Do not immediately buffer slash, may be start of comment.
		'/' >tokst %{ fsm->buf('/'); fsm->token( '/' ); } |
		# Dot covered by float.
		'.' %emit_symbol;

	# Comments and whitespace.
	commc = '/*' @{fsm->pass('/'); fsm->pass('*');} ( any* $0 '*/' @1 ) $pass;
	commcc = '//' @{fsm->pass('/'); fsm->pass('/');} ( any* $0 '\n' @1 ) $pass;
	whitespace = ( any - ( 0 | 33..126 ) )+ $pass;

	action onEOFChar { 
		/* On EOF char, write out the non token buffer. */
		fsm->nonTokBuf.append(0);
		cout << fsm->nonTokBuf.data;
		fsm->nonTokBuf.clear();
	}

	# Using 0 as eof. If seeingAs a result all null characters get ignored.
	EOF = 0 @onEOFChar;

	# All outside code tokens.
	tokens = ( 
		id | slit | dlit | float | integer_decimal | 
		integer_octal | integer_hex | compound | symbol );
	nontok = ( commc | commcc | whitespace | EOF );

	position = (
		'\n' @{ fsm->line += 1; fsm->col = 1; } |
		[^\n] @{ fsm->col += 1; } )*;

	main := ( ( tokens | nontok )** ) & position;
}%%

%% write data;

void Scanner::init( )
{
	Scanner *fsm = this;
	/* A count of the number of characters in 
	 * a token. Used for % sequences. */
	count = 0;
	line = 1;
	col = 1;

	%% write init;
}

int Scanner::execute( const char *data, int len )
{
	Scanner *fsm = this;
	const char *p = data;
	const char *pe = data + len;
	const char *eof = pe;

	%% write exec;
	if ( cs == Scanner_error )
		return -1;
	if ( cs >= Scanner_first_final )
		return 1;
	return 0;
}

int Scanner::finish( )
{
	if ( cs == Scanner_error )
		return -1;
	if ( cs >= Scanner_first_final )
		return 1;
	return 0;
}

void Scanner::token( int id )
{
	/* Leader. */
	if ( nonTokBuf.length > 0 ) {
		nonTokBuf.append(0);
		cout << nonTokBuf.data;
		nonTokBuf.clear();
	}

	/* Token data. */
	tokBuf.append(0);
	cout << '<' << id << '>' << tokBuf.data;
	tokBuf.clear();
}

void Buffer::empty()
{
	if ( data != 0 ) {
		free( data );

		data = 0;
		length = 0;
		allocated = 0;
	}
}

void Buffer::upAllocate( int len )
{
	if ( data == 0 )
		data = (char*) malloc( len );
	else
		data = (char*) realloc( data, len );
	allocated = len;
}

void test( const char *buf )
{
	Scanner scanner(cout);
	scanner.init();
	scanner.execute( buf, strlen(buf) );

	/* The last token is ignored (because there is no next token). Send
	 * trailing null to force the last token into whitespace. */
	char eof = 0;
	if ( scanner.execute( &eof, 1 ) <= 0 ) {
		cerr << "cppscan: scan failed" << endl;
		return;
	}
	cout.flush();
}

int main()
{
	test( 
		"/*\n"
		" *  Copyright \n"
		" */\n"
		"\n"
		"/* Construct an fsmmachine from a graph. */\n"
		"RedFsmAp::RedFsmAp( FsmAp *graph, bool complete )\n"
		":\n"
		"	graph(graph),\n"
		"{\n"
		"	assert( sizeof(RedTransAp) <= sizeof(TransAp) );\n"
		"\n"
		"	reduceMachine();\n"
		"}\n"
		"\n"
		"{\n"
		"	/* Get the transition that we want to extend. */\n"
		"	RedTransAp *extendTrans = list[pos].value;\n"
		"\n"
		"	/* Look ahead in the transition list. */\n"
		"	for ( int next = pos + 1; next < list.length(); pos++, next++ ) {\n"
		"		if ( ! keyOps->eq( list[pos].highKey, nextKey ) )\n"
		"			break;\n"
		"	}\n"
		"	return false;\n"
		"}\n"
		"\n" );

	test( 
		"->*\n"
		".*\n"
		"/*\"*/\n"
		"\"/*\"\n"
		"L'\"'\n"
		"L\"'\"\n" );
	
	return 0;
}

#ifdef _____OUTPUT_____
/*
 *  Copyright 
 */

/* Construct an fsmmachine from a graph. */
<195>RedFsmAp<197>::<195>RedFsmAp<40>( <195>FsmAp <42>*<195>graph<44>, <195>bool <195>complete <41>)
<58>:
	<195>graph<40>(<195>graph<41>)<44>,
<123>{
	<195>assert<40>( <195>sizeof<40>(<195>RedTransAp<41>) <60><<61>= <195>sizeof<40>(<195>TransAp<41>) <41>)<59>;

	<195>reduceMachine<40>(<41>)<59>;
<125>}

<123>{
	/* Get the transition that we want to extend. */
	<195>RedTransAp <42>*<195>extendTrans <61>= <195>list<91>[<195>pos<93>]<46>.<195>value<59>;

	/* Look ahead in the transition list. */
	<195>for <40>( <195>int <195>next <61>= <195>pos <43>+ <218>1<59>; <195>next <60>< <195>list<46>.<195>length<40>(<41>)<59>; <195>pos<212>++<44>, <195>next<212>++ <41>) <123>{
		<195>if <40>( <33>! <195>keyOps<211>-><195>eq<40>( <195>list<91>[<195>pos<93>]<46>.<195>highKey<44>, <195>nextKey <41>) <41>)
			<195>break<59>;
	<125>}
	<195>return <195>false<59>;
<125>}

<214>->*
<215>.*
/*"*/
<192>"/*"
<193>L'"'
<192>L"'"
#endif
