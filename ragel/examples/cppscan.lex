/*
 * flex equivalent to cppscan.rl
 */

%{

#include <stdio.h>

#define TK_Dlit 256
#define TK_Slit 257
#define TK_Float 258
#define TK_Id 259
#define TK_NameSep 260
#define TK_Arrow 261
#define TK_PlusPlus 262
#define TK_MinusMinus 263
#define TK_ArrowStar 264
#define TK_DotStar 265
#define TK_ShiftLeft 266
#define TK_ShiftRight 267
#define TK_IntegerDecimal 268
#define TK_IntegerOctal 269
#define TK_IntegerHex 270
#define TK_EqualsEquals 271
#define TK_NotEquals 272
#define TK_AndAnd 273
#define TK_OrOr 274
#define TK_MultAssign 275
#define TK_DivAssign 276
#define TK_PercentAssign 277
#define TK_PlusAssign 278
#define TK_MinusAssign 279
#define TK_AmpAssign 280
#define TK_CaretAssign 281
#define TK_BarAssign 282
#define TK_DotDotDot 283
#define TK_Whitespace 284
#define TK_Comment 285

int line = 1, col = 1;

void token( int tok, char *data, int len )
{
	printf( "<%i> ", tok );
	for ( int i = 0; i < len; i++ )
		fputc( data[i], stdout );
	fputc( '\n', stdout );

	/* Count newlines and columns. This code is here mainly for having some
	 * code in the token routine when commenting out the above output during
	 * performance testing. */
	for ( int i = 0; i < len; i ++ ) {
		if ( data[i] == '\n' ) {
			line += 1;
			col = 1;
		}
		else {
			col += 1;
		}
	}
}


%}

%x COMMENT

FRACT_CONST		[0-9]*\.[0-9]+|[0-9]+\.
EXPONENT		[eE][+\-]?[0-9]+
FLOAT_SUFFIX	[flFL]

%%

	/* Single and double literals. */
L?\'([^\'\\\n]|\\.)*\' {
	token( TK_Slit, yytext, yyleng );
}

L?\"([^\"\\\n]|\\.)*\" {
	token( TK_Dlit, yytext, yyleng );
}

[a-zA-Z_][a-zA-Z0-9_]* {
	token( TK_Id, yytext, yyleng );
}

{FRACT_CONST}{EXPONENT}?{FLOAT_SUFFIX}?|[0-9]+{EXPONENT}{FLOAT_SUFFIX}? {
	token( TK_Float, yytext, yyleng );
}

(0|[1-9][0-9]*)[ulUL]{0,3} {
	token( TK_IntegerDecimal, yytext, yyleng );
}

0[0-9]+[ulUL]{0,2} {
	token( TK_IntegerOctal, yytext, yyleng );
}

0x[0-9a-fA-F]+[ulUL]{0,2} {
	token( TK_IntegerHex, yytext, yyleng );
}

:: token( TK_NameSep, yytext, yyleng );
== token( TK_EqualsEquals, yytext, yyleng );
!= token( TK_NotEquals, yytext, yyleng );
&& token( TK_AndAnd, yytext, yyleng );
\|\| token( TK_OrOr, yytext, yyleng );
\*= token( TK_MultAssign, yytext, yyleng );
\/= token( TK_DivAssign, yytext, yyleng );
%= token( TK_PercentAssign, yytext, yyleng );
\+= token( TK_PlusAssign, yytext, yyleng );
-= token( TK_MinusAssign, yytext, yyleng );
&= token( TK_AmpAssign, yytext, yyleng );
^= token( TK_CaretAssign, yytext, yyleng );
\|= token( TK_BarAssign, yytext, yyleng );
\+\+ token( TK_PlusPlus, yytext, yyleng );
-- token( TK_MinusMinus, yytext, yyleng );
-> token( TK_Arrow, yytext, yyleng );
->\* token( TK_ArrowStar, yytext, yyleng );
\.\* token( TK_DotStar, yytext, yyleng );
\.\.\. token( TK_DotDotDot, yytext, yyleng );

\/\*				BEGIN(COMMENT);
<COMMENT>\*\/		BEGIN(INITIAL);
<COMMENT>(.|\n)		{ }

\/\/.*\n			{}
[^!-~]+				{}

[!-/:-@\[-`{-~] token( yytext[0], yytext, yyleng );
	
%%

int yywrap()
{
	/* Once the input is done, no more. */
	return 1;
}

int main()
{
	yylex();
}
