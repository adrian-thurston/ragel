/*
 * @LANG: d
 */

module cppscan;

import std.c.stdio;
import std.string;

const int BUFSIZE = 2048;

const int TK_Dlit = 192;
const int TK_Slit = 193;
const int TK_Float = 194;
const int TK_Id = 195;
const int TK_NameSep = 197;
const int TK_Arrow = 211;
const int TK_PlusPlus = 212;
const int TK_MinusMinus = 213;
const int TK_ArrowStar = 214;
const int TK_DotStar = 215;
const int TK_ShiftLeft = 216;
const int TK_ShiftRight = 217;
const int TK_IntegerDecimal = 218;
const int TK_IntegerOctal = 219;
const int TK_IntegerHex = 220;
const int TK_EqualsEquals = 223;
const int TK_NotEquals = 224;
const int TK_AndAnd = 225;
const int TK_OrOr = 226;
const int TK_MultAssign = 227;
const int TK_DivAssign = 228;
const int TK_PercentAssign = 229;
const int TK_PlusAssign = 230;
const int TK_MinusAssign = 231;
const int TK_AmpAssign = 232;
const int TK_CaretAssign = 233;
const int TK_BarAssign = 234;
const int TK_DotDotDot = 240;


class Scanner
{
	int line, col;
	int tokStart;
	int inlineDepth;
	int count;
	char[] tokBuf;
	char[] nonTokBuf;

	void pass(char c) { nonTokBuf ~= c; }
	void buf(char c) { tokBuf ~= c; }
	void token( int id )
	{
		/* Leader. */
		if ( nonTokBuf.length > 0 ) {
			printf("%.*s", nonTokBuf);
			nonTokBuf = "";
		}

		/* Token data. */
		printf("<%d>%.*s", id, tokBuf);

		tokBuf = "";
	}

	int cs, stack, top;
	
	%%{
		machine Scanner;
	
		action pass { pass(fc); }
		action buf { buf(fc); }

		action emit_slit { token( TK_Slit ); }
		action emit_dlit { token( TK_Dlit ); }
		action emit_id { token( TK_Id ); }
		action emit_integer_decimal { token( TK_IntegerDecimal ); }
		action emit_integer_octal { token( TK_IntegerOctal ); }
		action emit_integer_hex { token( TK_IntegerHex ); }
		action emit_float { token( TK_Float ); }
		action emit_symbol { token( tokBuf[0] ); }
		action tokst { tokStart = col; }

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
		namesep = '::' @buf %{token( TK_NameSep );};
		deqs = '==' @buf %{token( TK_EqualsEquals );};
		neqs = '!=' @buf %{token( TK_NotEquals );};
		and_and = '&&' @buf %{token( TK_AndAnd );};
		or_or = '||' @buf %{token( TK_OrOr );};
		mult_assign = '*=' @buf %{token( TK_MultAssign );};
		percent_assign = '%=' @buf %{token( TK_PercentAssign );};
		plus_assign = '+=' @buf %{token( TK_PlusAssign );};
		minus_assign = '-=' @buf %{token( TK_MinusAssign );};
		amp_assign = '&=' @buf %{token( TK_AmpAssign );};
		caret_assign = '^=' @buf %{token( TK_CaretAssign );};
		bar_assign = '|=' @buf %{token( TK_BarAssign );};
		plus_plus = '++' @buf %{token( TK_PlusPlus );};
		minus_minus = '--' @buf %{token( TK_MinusMinus );};
		arrow = '->' @buf %{token( TK_Arrow );};
		arrow_star = '->*' @buf %{token( TK_ArrowStar );};
		dot_star = '.*' @buf %{token( TK_DotStar );};

		# Buffer both items. *
		div_assign = '/=' @{buf('/');buf(fc);} %{token( TK_DivAssign );};

		# Double dot is sent as two dots.
		dot_dot = '..' %{token('.'); buf('.'); token('.');};

		# Three char compounds, first item already buffered. */
		dot_dot_dot = '...' %{buf('.'); buf('.'); token( TK_DotDotDot );};

		# All compunds
		compound = namesep | deqs | neqs | and_and | or_or | mult_assign |
				div_assign | percent_assign | plus_assign | minus_assign |
				amp_assign | caret_assign | bar_assign | plus_plus | minus_minus |
				arrow | arrow_star | dot_star | dot_dot | dot_dot_dot;

		# Single char symbols.
		symbol = 
			( punct - [./_"'] ) >tokst $buf %emit_symbol |
			# Do not immediately buffer slash, may be start of comment.
			'/' >tokst %{ buf('/'); token( '/' ); } |
			# Dot covered by float.
			'.' %emit_symbol;

		# Comments and whitespace.
		commc = '/*' @{pass('/'); pass('*');} ( any* $0 '*/' @1 ) $pass;
		commcc = '//' @{pass('/'); pass('/');} ( any* $0 '\n' @1 ) $pass;
		whitespace = ( any - ( 0 | 33..126 ) )+ $pass;

		action onEOFChar { 
			/* On EOF char, write out the non token buffer. */
			printf("%.*s", nonTokBuf);
			nonTokBuf = "";
		}

		# Using 0 as eof. If seeingAs a result all null characters get ignored.
		EOF = 0 @onEOFChar;

		# All outside code tokens.
		tokens = ( 
			id | slit | dlit | float | integer_decimal | 
			integer_octal | integer_hex | compound | symbol );
		nontok = ( commc | commcc | whitespace | EOF );

		position = (
			'\n' @{ line += 1; col = 1; } |
			[^\n] @{ col += 1; } )*;

		main := ( ( tokens | nontok )** ) & position;
	}%%
	
	%% write data noprefix;

	void init( )
	{
		/* A count of the number of characters in 
		 * a token. Used for % sequences. */
		count = 0;
		line = 1;
		col = 1;
		%% write init;
		return 1;
	}

	int execute( char* _data, int _len )
	{
		char *p = _data;
		char *pe = _data + _len;
		char *eof = null;

		%% write exec;

		if ( cs == error )
			return -1;
		if ( cs >= first_final )
			return 1;
		return 0;
	}

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( )
	{
		if ( cs == error )
			return -1;
		if ( cs >= first_final )
			return 1;
		return 0;
	}
};

void test(char[] buf)
{
	Scanner scanner = new Scanner();
	scanner.init();
	scanner.execute( buf.ptr, buf.length );

	/* The last token is ignored (because there is no next token). Send
	 * trailing null to force the last token into whitespace. */
	char eof_char = 0;
	if ( scanner.execute( &eof_char, 1 ) <= 0 ) {
		fprintf(stderr, "cppscan: scan failed\n");
	}
}

int main()
{
	test(
		"/*\n"
		" *  Copyright \n"
		" */\n"
		"\n"
		"RedTransAp *RedFsmAp::reduceTrans( TransAp *trans )\n"
		"{\n"
		"	RedAction *action = 0;\n"
		"	if ( trans->actionTable.length() > 0 ) {\n"
		"		if ( actionMap.insert( trans->actionTable, &action ) )\n"
		"			action->id = nextActionId++;\n"
		"	}\n"
		"	\n"
		"	RedStateAp *targ = (RedStateAp*)trans->toState;\n"
		"	if ( action == 0 ) {\n"
		"		delete trans;\n"
		"		return 0;\n"
		"	}\n"
		"\n"
		"	trans->~TransAp();\n"
		"	inDict = new(trans) RedTransAp( targ, action, nextTransId++ );\n"
		"	transSet.insert( inDict );\n"
		"}\n"
	);

	test(
		"->*\n"
		".*\n"
		"/*\"*/\n"
		"\"/*\"\n"
		"L'\"'\n"
		"L\"'\"\n"
	);

	return 0;
}

/+ _____OUTPUT_____
/*
 *  Copyright 
 */

<195>RedTransAp <42>*<195>RedFsmAp<197>::<195>reduceTrans<40>( <195>TransAp <42>*<195>trans <41>)
<123>{
	<195>RedAction <42>*<195>action <61>= <218>0<59>;
	<195>if <40>( <195>trans<211>-><195>actionTable<46>.<195>length<40>(<41>) <62>> <218>0 <41>) <123>{
		<195>if <40>( <195>actionMap<46>.<195>insert<40>( <195>trans<211>-><195>actionTable<44>, <38>&<195>action <41>) <41>)
			<195>action<211>-><195>id <61>= <195>nextActionId<212>++<59>;
	<125>}
	
	<195>RedStateAp <42>*<195>targ <61>= <40>(<195>RedStateAp<42>*<41>)<195>trans<211>-><195>toState<59>;
	<195>if <40>( <195>action <223>== <218>0 <41>) <123>{
		<195>delete <195>trans<59>;
		<195>return <218>0<59>;
	<125>}

	<195>trans<211>-><126>~<195>TransAp<40>(<41>)<59>;
	<195>inDict <61>= <195>new<40>(<195>trans<41>) <195>RedTransAp<40>( <195>targ<44>, <195>action<44>, <195>nextTransId<212>++ <41>)<59>;
	<195>transSet<46>.<195>insert<40>( <195>inDict <41>)<59>;
<125>}
<214>->*
<215>.*
/*"*/
<192>"/*"
<193>L'"'
<192>L"'"
+++++++++++++++++/
