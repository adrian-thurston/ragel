/*
 * @LANG: c++
 */

#include <iostream>
#include <string.h>
using namespace std;

#define TK_Dlit 192
#define TK_Slit 193
#define TK_Float 194
#define TK_Id 195
#define TK_NameSep 197
#define TK_Arrow 211
#define TK_PlusPlus 212
#define TK_MinusMinus 213
#define TK_ArrowStar 214
#define TK_DotStar 215
#define TK_ShiftLeft 216
#define TK_ShiftRight 217
#define TK_IntegerDecimal 218
#define TK_IntegerOctal 219
#define TK_IntegerHex 220
#define TK_EqualsEquals 223
#define TK_NotEquals 224
#define TK_AndAnd 225
#define TK_OrOr 226
#define TK_MultAssign 227
#define TK_DivAssign 228
#define TK_PercentAssign 229
#define TK_PlusAssign 230
#define TK_MinusAssign 231
#define TK_AmpAssign 232
#define TK_CaretAssign 233
#define TK_BarAssign 234
#define TK_DotDotDot 240
#define TK_Whitespace 241
#define TK_Comment 242

#define BUFSIZE 4096

int tok;
char buf[BUFSIZE];
const char *ts, *te;
void token( const char *data, int len );
bool discard = false;

struct Scanner
{
	int cs;

	// Initialize the machine. Invokes any init statement blocks. Returns 0
	// if the machine begins in a non-accepting state and 1 if the machine
	// begins in an accepting state.
	int init( );

	// Execute the machine on a block of data. Returns -1 if after processing
	// the data, the machine is in the error state and can never accept, 0 if
	// the machine is in a non-accepting state and 1 if the machine is in an
	// accepting state.
	int execute( const char *data, int len );

	// Indicate that there is no more data. Returns -1 if the machine finishes
	// in the error state and does not accept, 0 if the machine finishes
	// in any other non-accepting state and 1 if the machine finishes in an
	// accepting state.
	int finish( );
};

%%{
	machine Scanner;

	# Single and double literals.
	slit = ( 'L'? "'" ( [^'\\\n] | /\\./ )* "'" ) @{tok = TK_Slit;};
	dlit = ( 'L'? '"' ( [^"\\\n] | /\\./ )* '"' ) @{tok = TK_Dlit;};

	# Identifiers
	id = ( [a-zA-Z_] [a-zA-Z0-9_]* ) @{tok = TK_Id;};

	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;
	float_suffix = [flFL];
	float = 
		( fract_const exponent? float_suffix? |
		digit+ exponent float_suffix? ) @{tok = TK_Float;};
	
	# Integer decimal. Leading part buffered by float.
	integer_decimal = ( ( '0' | [1-9] [0-9]* ) [ulUL]{0,3} ) @{tok = TK_IntegerDecimal;};

	# Integer octal. Leading part buffered by float.
	integer_octal = ( '0' [0-9]+ [ulUL]{0,2} ) @{tok = TK_IntegerOctal;};

	# Integer hex. Leading 0 buffered by float.
	integer_hex = ( '0' ( 'x' [0-9a-fA-F]+ [ulUL]{0,2} ) ) @{tok = TK_IntegerHex;};

	# Only buffer the second item, first buffered by symbol. */
	namesep = '::' @{tok = TK_NameSep;};
	deqs = '==' @{tok = TK_EqualsEquals;};
	neqs = '!=' @{tok = TK_NotEquals;};
	and_and = '&&' @{tok = TK_AndAnd;};
	or_or = '||' @{tok = TK_OrOr;};
	mult_assign = '*=' @{tok = TK_MultAssign;};
	div_assign = '/=' @{tok = TK_DivAssign;};
	percent_assign = '%=' @{tok = TK_PercentAssign;};
	plus_assign = '+=' @{tok = TK_PlusAssign;};
	minus_assign = '-=' @{tok = TK_MinusAssign;};
	amp_assign = '&=' @{tok = TK_AmpAssign;};
	caret_assign = '^=' @{tok = TK_CaretAssign;};
	bar_assign = '|=' @{tok = TK_BarAssign;};
	plus_plus = '++' @{tok = TK_PlusPlus;};
	minus_minus = '--' @{tok = TK_MinusMinus;};
	arrow = '->' @{tok = TK_Arrow;};
	arrow_star = '->*' @{tok = TK_ArrowStar;};
	dot_star = '.*' @{tok = TK_DotStar;};

	# Three char compounds, first item already buffered. */
	dot_dot_dot = '...' @{tok = TK_DotDotDot;};

	# All compunds
	compound = namesep | deqs | neqs | and_and | or_or | mult_assign |
			div_assign | percent_assign | plus_assign | minus_assign |
			amp_assign | caret_assign | bar_assign | plus_plus | minus_minus |
			arrow | arrow_star | dot_star | dot_dot_dot;

	# Single char symbols.
	symbol = ( punct - [_"'] ) @{tok = fc;};

	action discard {
		discard = true;
	}

	# Comments and whitespace.
	commc = '/*' @discard ( any* $0 '*/' @1 ) @{tok = TK_Comment;};
	commcc = '//' @discard ( any* $0 '\n' @1 ) @{tok = TK_Comment;};
	whitespace = ( any - 33..126 )+ >discard @{tok = TK_Whitespace;};

	# All outside code tokens.
	tokens = ( 
		id | slit | dlit | float | integer_decimal | 
		integer_octal | integer_hex | compound | symbol |
		commc | commcc | whitespace );

	action onError {
		if ( tok != 0 ) {
			const char *rst_data;

			if ( tok == TK_Comment || tok == TK_Whitespace ) {
				/* Reset comment status, don't send. */
				discard = false;

				/* Restart right at the error point if consuming whitespace or
				 * a comment. Consume may have spanned multiple buffers. */
				rst_data = fpc;
			}
			else {
				/* Send the token. */
				token( ts, te - ts + 1 );

				/* Restart right after the token. */
				rst_data = te+1;
			}

			ts = 0;
			fexec rst_data;
			fgoto main;
		}
	}

	main := tokens >{ts=fpc;} @{te=fpc;} $!onError;
}%%

%% write data;

int Scanner::init( )
{
	tok = 0;
	ts = 0;
	te = 0;

	%% write init;
	return 1;
}

int Scanner::execute( const char *data, int len )
{
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


void token( const char *data, int len )
{
	cout << "<" << tok << "> ";
	for ( int i = 0; i < len; i++ )
		cout << data[i];
	cout << '\n';
}

void test( const char * data )
{
	Scanner scanner;
	scanner.init();
	scanner.execute( data, strlen(data) );
	scanner.finish();
	if ( tok != 0 && tok != TK_Comment && tok != TK_Whitespace )
		token( ts, te - ts + 1 );
}

int main()
{
	test(
		"/*\n"
		" *  Copyright \n"
		" */\n"
		"\n"
		"\n"
		"/* Move ranges to the singles list. */\n"
		"void RedFsmAp::move( RedStateAp *state )\n"
		"{\n"
		"	RedTranst &range = state->outRange;\n"
		"	for ( int rpos = 0; rpos < range.length(); ) {\n"
		"		if ( can( range, rpos ) ) {\n"
		"			while ( range[rpos].value != range[rpos+1].value ) {\n"
		"				single.append( range[rpos+1] );\n"
		"			}\n"
		"			\n"
		"			range[rpos].highKey = range[rpos+1].highKey;\n"
		"		}\n"
		"		else if ( keyOps->span( range[rpos].lowKey, range[rpos].highKey ) == 1 ) {\n"
		"			single.append( range[rpos] );\n"
		"		}\n"
		"	}\n"
		"}\n"
		"\n" );

	test( 
		"->*\n"
		".*\n"
		"/*\"*/\n"
		"\"/*\"\n"
		"L'\"'\n"
		"L\"'\"\n"
		"...\n" );
}

#ifdef _____OUTPUT_____
<195> void
<195> RedFsmAp
<197> ::
<195> move
<40> (
<195> RedStateAp
<42> *
<195> state
<41> )
<123> {
<195> RedTranst
<38> &
<195> range
<61> =
<195> state
<211> ->
<195> outRange
<59> ;
<195> for
<40> (
<195> int
<195> rpos
<61> =
<218> 0
<59> ;
<195> rpos
<60> <
<195> range
<46> .
<195> length
<40> (
<41> )
<59> ;
<41> )
<123> {
<195> if
<40> (
<195> can
<40> (
<195> range
<44> ,
<195> rpos
<41> )
<41> )
<123> {
<195> while
<40> (
<195> range
<91> [
<195> rpos
<93> ]
<46> .
<195> value
<224> !=
<195> range
<91> [
<195> rpos
<43> +
<218> 1
<93> ]
<46> .
<195> value
<41> )
<123> {
<195> single
<46> .
<195> append
<40> (
<195> range
<91> [
<195> rpos
<43> +
<218> 1
<93> ]
<41> )
<59> ;
<125> }
<195> range
<91> [
<195> rpos
<93> ]
<46> .
<195> highKey
<61> =
<195> range
<91> [
<195> rpos
<43> +
<218> 1
<93> ]
<46> .
<195> highKey
<59> ;
<125> }
<195> else
<195> if
<40> (
<195> keyOps
<211> ->
<195> span
<40> (
<195> range
<91> [
<195> rpos
<93> ]
<46> .
<195> lowKey
<44> ,
<195> range
<91> [
<195> rpos
<93> ]
<46> .
<195> highKey
<41> )
<223> ==
<218> 1
<41> )
<123> {
<195> single
<46> .
<195> append
<40> (
<195> range
<91> [
<195> rpos
<93> ]
<41> )
<59> ;
<125> }
<125> }
<125> }
<214> ->*
<215> .*
<192> "/*"
<193> L'"'
<192> L"'"
<240> ...
#endif
