/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@cs.queensu.ca>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

%{

#define YY_NEVER_INTERACTIVE 1
//#define WANT_TOKEN_WRITE

#include <iostream>
#include "ragel.h"
#include "rlparse.h"
#include "parsedata.h"
#include "buffer.h"

using std::cout;
using std::cerr;
using std::endl;

Buffer tokbuf;
int builtinBrace = 0;
bool inlineWhitespace = true;
bool handlingInclude = false;
bool multiline = false;

/* Used for recognising host language code blocks, init with anything not
 * involved in the host lang test. */
int previous_tokens[2] = { TK_Section, TK_Section };

/* These keep track of the start of an inline comment or literal string for
 * reporting unterminated comments or strings. */
int il_comm_lit_first_line;
int il_comm_lit_first_column;

/* These keep track of the start of a code block for reporting unterminated
 * code blocks. */
int il_code_first_line;
int il_code_first_column;

/* Include Stack data. */
YY_BUFFER_STATE buff_stack[INCLUDE_STACK_SIZE];
bool multiline_stack[INCLUDE_STACK_SIZE];
int inc_stack_ptr = 0;

YYSTYPE *yylval;
YYLTYPE *yylloc;

extern InputData *id;
extern int includeDepth;

void garble();

void extendToken( char *data, int len );
void extendToken();

int emitToken( int token, char *data, int len );
int emitNoData( int token );
void passThrough( char *data );
bool openMachineSpecBlock();
void popInclude();

enum InlineBlockType {
	CurlyDelimited,
	SemiTerminated
} inlineBlockType;

/* Using a wrapper for the parser, must the lex declaration. */
#define YY_DECL int ragel_lex()

%}

/* Outside an fsm machine specification ("outside code"). */
%x		OC_SGL_LIT
%x		OC_DBL_LIT
%x		OC_C_COM
%x		OC_CXX_COM

/* Inside a fsm machine specification. */
%x		RL_INITIAL
%x		RL_SLIT
%x		RL_DLIT
%x		RL_OREXP
%x		RL_REGEXP
%x		RL_REGEXP_OR
%x		RL_SHELL_COM
%x		RL_VERBOSE_EMBED
%x		RL_WRITE

/* Inline code. */
%x		IL_INITIAL
%x		IL_SGL_LIT
%x		IL_DBL_LIT
%x		IL_C_COM
%x		IL_CXX_COM

WSCHAR [\t\n\v\f\r ]
IDENT [a-zA-Z_][a-zA-Z_0-9]*

%%

	/* Numbers in outter code. */
<INITIAL>[0-9]+ {
	garble();
	passThrough( yytext );
}

	/* Words in outter code. */
<INITIAL>{IDENT} {
	garble();
	passThrough( yytext );
}

	/* Begin a c style comment. */
<INITIAL>"/*" {
	BEGIN(OC_C_COM);
	extendToken();
	passThrough( yytext );
}
	/* Data in a C style comment. */
<OC_C_COM>. 		extendToken(); passThrough( yytext );
<OC_C_COM>\n 		extendToken(); passThrough( yytext );

	/* Terminate a C style comment. */
<OC_C_COM>"*/" {
	BEGIN(INITIAL);
	garble();
	passThrough( yytext );
}

	/* Begin a C++ style comment. */
<INITIAL>"//" {
	BEGIN(OC_CXX_COM);
	extendToken();
	passThrough( yytext );
}
	/* Data in a C++ style comment. */
<OC_CXX_COM>[^\n]+ {
	extendToken();
	passThrough( yytext );
}
	/* Terminate a C++ style comment. */
<OC_CXX_COM>\n {
	BEGIN(INITIAL);
	garble();
	passThrough( yytext );
}


	/* Start literals. */
<INITIAL>\' {
	BEGIN(OC_SGL_LIT);
	extendToken();
	passThrough( yytext );
}
<INITIAL>\" {
	BEGIN(OC_DBL_LIT);
	extendToken();
	passThrough( yytext );
}
	/* Various escape sequences in literals. We don't need to get them
	 * all here. We just need to pick off the ones that could confuse us
	 * about the literal we are matchine */
<OC_SGL_LIT,OC_DBL_LIT>\\\'		extendToken(); passThrough( yytext );
<OC_SGL_LIT,OC_DBL_LIT>\\\"		extendToken(); passThrough( yytext );
<OC_SGL_LIT,OC_DBL_LIT>\\\\		extendToken(); passThrough( yytext );
	/* Characters in literals. */
<OC_DBL_LIT>[^\"]				extendToken(); passThrough( yytext );
<OC_SGL_LIT>[^\']				extendToken(); passThrough( yytext );
	/* Terminate a double literal */
<OC_DBL_LIT>\" {
	BEGIN(INITIAL);
	garble();
	passThrough( yytext );
}
	/* Terminate a single literal. */
<OC_SGL_LIT>\' {
	BEGIN(INITIAL);
	garble();
	passThrough( yytext );
}

	/* Whitespace. */
<INITIAL>{WSCHAR}+ {
	garble();
	passThrough( yytext );
}

	/* Section Deliminator */
<INITIAL>"%%" {
	BEGIN(RL_INITIAL);
	multiline = false;
	return emitNoData( TK_Section );
}

	/* Section Deliminator */
<INITIAL>"%%{" {
	BEGIN(RL_INITIAL);
	multiline = true;
	return emitNoData( TK_Section );
}

<INITIAL>"{" {
	garble();
	passThrough( yytext );
}

<INITIAL>"}" {
	garble();
	passThrough( yytext );
}

<INITIAL>";" {
	garble();
	passThrough( yytext );
}

	/* Any other characters. */
<INITIAL>. {
	garble();
	passThrough( yytext );
}

	/* Numbers. */
<RL_INITIAL,IL_INITIAL>[0-9][0-9]* {	
	return emitToken( TK_UInt, yytext, yyleng );
}
<RL_INITIAL,IL_INITIAL>0x[0-9a-fA-F][0-9a-fA-F]* {	
	return emitToken( TK_Hex, yytext, yyleng );
}

	/* Keywords in RL and IL. */
<RL_INITIAL>variable\ [a-zA-Z_]+ {
	BEGIN(IL_INITIAL);
	inlineBlockType = SemiTerminated;
	return emitToken( KW_Variable, yytext+9, yyleng-9 );
}
<RL_INITIAL>access {
	BEGIN(IL_INITIAL);
	inlineBlockType = SemiTerminated;
	return emitNoData( KW_Access );
}
<RL_INITIAL>action {
	return emitNoData( KW_Action );
}
<RL_INITIAL>alphtype {
	BEGIN(IL_INITIAL);
	inlineWhitespace = false;
	inlineBlockType = SemiTerminated;
	return emitNoData( KW_AlphType );
}
<RL_INITIAL>getkey {
	BEGIN(IL_INITIAL);
	inlineBlockType = SemiTerminated;
	return emitNoData( KW_GetKey );
}
<RL_INITIAL>when {
	return emitNoData( KW_When );
}
<RL_INITIAL>eof {
	return emitNoData( KW_Eof );
}
<RL_INITIAL>err {
	return emitNoData( KW_Err );
}
<RL_INITIAL>lerr {
	return emitNoData( KW_Lerr );
}
<RL_INITIAL>to {
	return emitNoData( KW_To );
}
<RL_INITIAL>from {
	return emitNoData( KW_From );
}


	/*
<RL_INITIAL>range {
	return emitNoData( KW_Range );
}*/

<RL_INITIAL>write {
	BEGIN(RL_WRITE);
	return emitNoData( KW_Write );
}
<RL_INITIAL>machine {
	return emitNoData( KW_Machine );
}
<RL_INITIAL>include {
	/* Include tokens statments are processed by both the scanner and the
	 * parser.  The scanner opens the include file and switches to it and the
	 * parser invokes a new parser for handling the tokens. We use
	 * handlingInclude to indicate that the scanner is processing an include
	 * directive. Ends at ; */
	handlingInclude = true;
	return emitNoData( KW_Include );
}

<RL_WRITE>{WSCHAR}+ garble();
<RL_WRITE>; {
	BEGIN(RL_INITIAL);
	return emitNoData( ';' );
}

	/* These must be synced in rlparse.y */
<IL_INITIAL>fpc {
	return emitNoData( KW_PChar );
}
<IL_INITIAL>fc {
	return emitNoData( KW_Char );
}
<IL_INITIAL>fhold {
	return emitNoData( KW_Hold );
}
<IL_INITIAL>fgoto {
	return emitNoData( KW_Goto );
}
<IL_INITIAL>fcall {
	return emitNoData( KW_Call );
}
<IL_INITIAL>fret {
	return emitNoData( KW_Ret );
}
<IL_INITIAL>fcurs {
	return emitNoData( KW_CurState );
}
<IL_INITIAL>ftargs {
	return emitNoData( KW_TargState );
}
<IL_INITIAL>fentry {
	return emitNoData( KW_Entry );
}
<IL_INITIAL>fnext {
	return emitNoData( KW_Next );
}
<IL_INITIAL>fexec {
	return emitNoData( KW_Exec );
}
<IL_INITIAL>fbreak {
	return emitNoData( KW_Break );
}

	/* Words. */
<RL_INITIAL,IL_INITIAL,RL_WRITE>{IDENT} {
	return emitToken( TK_Word, yytext, yyleng );
}

	/* Begin a shell style comment. */
<RL_INITIAL>#			{
	BEGIN(RL_SHELL_COM);
	extendToken();
}
	/* Data in a shell style comment. */
<RL_SHELL_COM>[^\n]+		{
	extendToken();
}
	/* Terminate a C++ style comment. */
<RL_SHELL_COM>\n		{
	BEGIN(RL_INITIAL);
	garble();
}

	/* 
	 * Start single and double literals.
	 */
<RL_INITIAL>'			{
	BEGIN(RL_SLIT);
	extendToken();
}
<RL_INITIAL>\"			{
	BEGIN(RL_DLIT);
	extendToken();
}

	/* Escape sequences in single and double literals. */
<RL_SLIT,RL_DLIT>\\0		extendToken( "\0", 1 );
<RL_SLIT,RL_DLIT>\\a		extendToken( "\a", 1 );
<RL_SLIT,RL_DLIT>\\b		extendToken( "\b", 1 );
<RL_SLIT,RL_DLIT>\\t		extendToken( "\t", 1 );
<RL_SLIT,RL_DLIT>\\n		extendToken( "\n", 1 );
<RL_SLIT,RL_DLIT>\\v		extendToken( "\v", 1 );
<RL_SLIT,RL_DLIT>\\f		extendToken( "\f", 1 );
<RL_SLIT,RL_DLIT>\\r		extendToken( "\r", 1 );
<RL_SLIT,RL_DLIT>\\\n		extendToken();
<RL_SLIT,RL_DLIT>\\.		extendToken( yytext+1, 1 );

	/* Characters in literals. */
<RL_SLIT>[^']						extendToken( yytext, 1 );
<RL_DLIT>[^"]						extendToken( yytext, 1 );

	/* Terminate a single literal. */
<RL_SLIT>'[i]* {
	BEGIN(RL_INITIAL);
	return emitToken( yytext[1] == 'i' ? TK_CiLiteral : TK_Literal, 0, 0 );
}
	/* Terminate a double literal */
<RL_DLIT>\"[i]* {
	BEGIN(RL_INITIAL);
	return emitToken( yytext[1] == 'i' ? TK_CiLiteral : TK_Literal, 0, 0 );
}

	/*
	 * Start an OR expression. 
	 */
<RL_INITIAL>"["			{
	BEGIN(RL_OREXP);
	return emitNoData( RE_SqOpen );
}

<RL_INITIAL>"\[^"	{
	BEGIN(RL_OREXP);
	return emitNoData( RE_SqOpenNeg );
}

	/* Escape sequences in OR expressions. */
<RL_OREXP>\\0		{ return emitToken( RE_Char, "\0", 1 ); }
<RL_OREXP>\\a		{ return emitToken( RE_Char, "\a", 1 ); }
<RL_OREXP>\\b		{ return emitToken( RE_Char, "\b", 1 ); }
<RL_OREXP>\\t		{ return emitToken( RE_Char, "\t", 1 ); }
<RL_OREXP>\\n		{ return emitToken( RE_Char, "\n", 1 ); }
<RL_OREXP>\\v		{ return emitToken( RE_Char, "\v", 1 ); }
<RL_OREXP>\\f		{ return emitToken( RE_Char, "\f", 1 ); }
<RL_OREXP>\\r		{ return emitToken( RE_Char, "\r", 1 ); }
<RL_OREXP>\\\n		{ garble(); }
<RL_OREXP>\\.		{ return emitToken( RE_Char, yytext+1, 1 ); }

	/* Range dash in an OR expression. */
<RL_OREXP>-	{
	return emitNoData( RE_Dash );
}

	/* Characters in an OR expression. */
<RL_OREXP>[^\]] {
	return emitToken( RE_Char, yytext, 1 );
}

	/* Terminate an OR expression. */
<RL_OREXP>\]	{
	BEGIN(RL_INITIAL); 
	return emitNoData( RE_SqClose );
}

	/* 
	 * Start a regular expression. 
	 */
<RL_INITIAL>\/		{
	BEGIN(RL_REGEXP);
	return emitNoData( RE_Slash );
}

	/* Escape sequences in regular expressions. */
<RL_REGEXP,RL_REGEXP_OR>\\0		{
	return emitToken( RE_Char, "\0", 1 );
}
<RL_REGEXP,RL_REGEXP_OR>\\a		{
	return emitToken( RE_Char, "\a", 1 );
}
<RL_REGEXP,RL_REGEXP_OR>\\b		{
	return emitToken( RE_Char, "\b", 1 );
}
<RL_REGEXP,RL_REGEXP_OR>\\t		{
	return emitToken( RE_Char, "\t", 1 );
}
<RL_REGEXP,RL_REGEXP_OR>\\n		{
	return emitToken( RE_Char, "\n", 1 );
}
<RL_REGEXP,RL_REGEXP_OR>\\v		{
	return emitToken( RE_Char, "\v", 1 );
}
<RL_REGEXP,RL_REGEXP_OR>\\f		{
	return emitToken( RE_Char, "\f", 1 );
}
<RL_REGEXP,RL_REGEXP_OR>\\r		{
	return emitToken( RE_Char, "\r", 1 );
}
<RL_REGEXP,RL_REGEXP_OR>\\\n	{
	garble();
}
<RL_REGEXP,RL_REGEXP_OR>\\.		{
	return emitToken( RE_Char, yytext+1, 1 );
}

	/* Special characters in a regular expression. */
<RL_REGEXP>\.		{
	return emitNoData( RE_Dot );
}
<RL_REGEXP>\*		{
	return emitNoData( RE_Star );
}
<RL_REGEXP>"\[^"	{
	BEGIN(RL_REGEXP_OR);
	return emitNoData( RE_SqOpenNeg );
}
<RL_REGEXP>"\["		{
	BEGIN(RL_REGEXP_OR);
	return emitNoData( RE_SqOpen );
}

	/* Range dash in a regular expression or set. */
<RL_REGEXP_OR>-	{
	return emitNoData( RE_Dash );
}

	/* Terminate an or set or a regular expression. */
<RL_REGEXP_OR>\]	{
	BEGIN(RL_REGEXP); 
	return emitNoData( RE_SqClose );
}

	/* Characters in a regular expression. */
<RL_REGEXP,RL_REGEXP_OR>[^/]			{
	return emitToken( RE_Char, yytext, 1 );
}

	/* Terminate a regular expression */
<RL_REGEXP,RL_REGEXP_OR>\/[i]* {
	BEGIN(RL_INITIAL);
	return emitToken( RE_Slash, yytext, yyleng );
}

	/* Builtin code move to Builtin initial. */
<RL_INITIAL>"{" {
	if ( openMachineSpecBlock() ) {
		/* Plain bracket. */
		return emitNoData( *yytext );
	}
	else {
		/* Start an inline code block. Keep track of where it started in case
		 * it terminates prematurely. Return the open bracket. */
		BEGIN(IL_INITIAL);
		inlineBlockType = CurlyDelimited;
		il_code_first_line = id->last_line;
		il_code_first_column = id->last_column+1;
		builtinBrace++;
		return emitNoData( *yytext );
	}
}

<RL_INITIAL>\.\. {
	return emitNoData( TK_DotDot );
}

<RL_INITIAL>:> {
	return emitNoData( TK_ColonGt );
}

<RL_INITIAL>:>> {
	return emitNoData( TK_ColonGtGt );
}

<RL_INITIAL><: {
	return emitNoData( TK_LtColon );
}

<RL_INITIAL>-- {
	return emitNoData( TK_DashDash );
}

	/* The instantiation operator. */
<RL_INITIAL>:= {
	return emitNoData( TK_ColonEquals );
}

	/* Error actions. */
<RL_INITIAL>\>\! {
	return emitNoData( TK_StartGblError );
}
<RL_INITIAL>\$\! {
	return emitNoData( TK_AllGblError );
}
<RL_INITIAL>%\! {
	return emitNoData( TK_FinalGblError );
}
<RL_INITIAL><\! {
	return emitNoData( TK_NotStartGblError );
}
<RL_INITIAL>@\! {
	return emitNoData( TK_NotFinalGblError );
}
<RL_INITIAL><>\! {
	return emitNoData( TK_MiddleGblError );
}

	/* Local error actions. */
<RL_INITIAL>\>\^ {
	return emitNoData( TK_StartLocalError );
}
<RL_INITIAL>\$\^ {
	return emitNoData( TK_AllLocalError );
}
<RL_INITIAL>%\^ {
	return emitNoData( TK_FinalLocalError );
}
<RL_INITIAL><\^ {
	return emitNoData( TK_NotStartLocalError );
}
<RL_INITIAL>@\^ {
	return emitNoData( TK_NotFinalLocalError );
}
<RL_INITIAL><>\^ {
	return emitNoData( TK_MiddleLocalError );
}

	/* EOF Actions. */
<RL_INITIAL>\>\/ {
	return emitNoData( TK_StartEOF );
}
<RL_INITIAL>\$\/ {
	return emitNoData( TK_AllEOF );
}
<RL_INITIAL>%\/ {
	return emitNoData( TK_FinalEOF );
}
<RL_INITIAL><\/ {
	return emitNoData( TK_NotStartEOF );
}
<RL_INITIAL>@\/ {
	return emitNoData( TK_NotFinalEOF );
}
<RL_INITIAL><>\/ {
	return emitNoData( TK_MiddleEOF );
}

	/* To State Actions. */
<RL_INITIAL>\>~ {
	return emitNoData( TK_StartToState );
}
<RL_INITIAL>\$~ {
	return emitNoData( TK_AllToState );
}
<RL_INITIAL>%~ {
	return emitNoData( TK_FinalToState );
}
<RL_INITIAL><~ {
	return emitNoData( TK_NotStartToState );
}
<RL_INITIAL>@~ {
	return emitNoData( TK_NotFinalToState );
}
<RL_INITIAL><>~ {
	return emitNoData( TK_MiddleToState );
}

	/* From State Actions. */
<RL_INITIAL>\>\* {
	return emitNoData( TK_StartFromState );
}
<RL_INITIAL>\$\* {
	return emitNoData( TK_AllFromState );
}
<RL_INITIAL>%\* {
	return emitNoData( TK_FinalFromState );
}
<RL_INITIAL><\* {
	return emitNoData( TK_NotStartFromState );
}
<RL_INITIAL>@\* {
	return emitNoData( TK_NotFinalFromState );
}
<RL_INITIAL><>\* {
	return emitNoData( TK_MiddleFromState );
}

<RL_INITIAL><> {
	return emitNoData( TK_Middle );
}

<RL_INITIAL>\>\? {
	return emitNoData( TK_StartCond );
}
<RL_INITIAL>\$\? {
	return emitNoData( TK_AllCond );
}
<RL_INITIAL>%\? {
	return emitNoData( TK_LeavingCond );
}

	/* The Arrow operator. */
<RL_INITIAL>-> {
	return emitNoData( TK_Arrow );
}

	/* The double arrow operator. */
<RL_INITIAL>=> {
	return emitNoData( TK_DoubleArrow );
}

	/* Double star (longest match kleene star). */
<RL_INITIAL>\*\* {
	return emitNoData( TK_StarStar );
}

	/* Name separator. */
<RL_INITIAL>:: {
	return emitNoData( TK_NameSep );
}

	/* Opening of longest match. */
<RL_INITIAL>\|\* {
	return emitNoData( TK_BarStar );
}

	/* Catch the repetition operator now to free up the parser. Once caught,
	 * Send only the opening brace and rescan the rest so it can be broken
	 * up for the parser. */
<RL_INITIAL>\{([0-9]+(,[0-9]*)?|,[0-9]+)\} {
	yyless(1);
	return emitNoData( TK_RepOpOpen );
}

	/* Section Deliminator */
<RL_INITIAL>"}%%" {
	BEGIN(INITIAL);
	return emitNoData( TK_Section );
}

	/* Whitespace. */
<RL_INITIAL>[\t\v\f\r ]		garble();
<RL_INITIAL>\n {
	if ( multiline )
		garble();
	else {
		BEGIN(INITIAL);
		return emitNoData( TK_SectionNL );
	}
}

	/* Any other characters. */
<RL_INITIAL>. {
	return emitNoData( *yytext );
}

	/* End of input in a literal is an error. */
<RL_SLIT,RL_DLIT><<EOF>> {
	error(id->first_line, id->first_column) << "unterminated literal" << endl;
	exit(1);
}
	
	/* End of input in a comment is an error. */
<RL_SHELL_COM><<EOF>> {
	error(id->first_line, id->first_column) << "unterminated comment" << endl;
	exit(1);
}

	/* Begin a C style comment. */
<IL_INITIAL>"/*" {
	BEGIN(IL_C_COM);
	il_comm_lit_first_line = id->last_line;
	il_comm_lit_first_column = id->last_column+1;
	extendToken( yytext, yyleng );
}
	/* Data in a C style comment. */
<IL_C_COM>\n 	extendToken( yytext, 1 );
<IL_C_COM>. 	extendToken( yytext, 1 );

	/* Terminate a C style comment. */
<IL_C_COM>"*/" {
	BEGIN(IL_INITIAL);
	return emitToken( IL_Comment, yytext, 2 );
}

	/* Begin a C++ style comment. */
<IL_INITIAL>"//" {
	BEGIN(IL_CXX_COM);
	il_comm_lit_first_line = id->last_line;
	il_comm_lit_first_column = id->last_column+1;
	extendToken( yytext, yyleng );
}
	/* Data in a C++ style comment. */
<IL_CXX_COM>[^\n]+ {
	extendToken( yytext, yyleng );
}
	/* Terminate a C++ style comment. */
<IL_CXX_COM>\n {
	BEGIN(IL_INITIAL);
	return emitToken( IL_Comment, yytext, 1 );
}


	/* Start literals. */
<IL_INITIAL>' {
	BEGIN(IL_SGL_LIT);
	il_comm_lit_first_line = id->last_line;
	il_comm_lit_first_column = id->last_column+1;
	extendToken( yytext, 1 );
}
<IL_INITIAL>\" {
	BEGIN(IL_DBL_LIT);
	il_comm_lit_first_line = id->last_line;
	il_comm_lit_first_column = id->last_column+1;
	extendToken( yytext, 1 );
}
	/* Various escape sequences in literals. We don't need to get them
	 * all here. We just need to pick off the ones that could confuse us
	 * about the literal we are matching */
<IL_SGL_LIT,IL_DBL_LIT>\\'		extendToken( yytext, yyleng );
<IL_SGL_LIT,IL_DBL_LIT>\\\"		extendToken( yytext, yyleng );
<IL_SGL_LIT,IL_DBL_LIT>\\\\		extendToken( yytext, yyleng );
	/* Characters in literals. */
<IL_DBL_LIT>[^\"]				extendToken( yytext, 1 );
<IL_SGL_LIT>[^']				extendToken( yytext, 1 );

	/* Terminate a double literal */
<IL_DBL_LIT>\" {
	BEGIN(IL_INITIAL);
	return emitToken( IL_Literal, yytext, 1 );
}
	/* Terminate a single literal. */
<IL_SGL_LIT>' {
	BEGIN(IL_INITIAL);
	return emitToken( IL_Literal, yytext, 1 );
}

	/* Open Brace, increment count of open braces. */
<IL_INITIAL>"{" {
	builtinBrace++;
	return emitToken( IL_Symbol, yytext, 1 );
}

	/* Close brace, decrement count of open braces. */
<IL_INITIAL>"}" {
	builtinBrace--;
	if ( inlineBlockType == CurlyDelimited && builtinBrace == 0 ) {
		/* Inline code block ends. */
		BEGIN(RL_INITIAL);
		inlineWhitespace = true;
		return emitNoData( *yytext );
	}
	else {
		/* Either a semi terminated inline block or only the closing brace of
		 * some inner scope, not the block's closing brace. */
		return emitToken( IL_Symbol, yytext, 1 );
	}
}

	/* May need to terminate the inline block. */
<IL_INITIAL>; {
	if ( inlineBlockType == SemiTerminated ) {
		/* Inline code block ends. */
		BEGIN(RL_INITIAL);
		inlineWhitespace = true;
		return emitNoData( TK_Semi );
	}
	else {
		/* Not ending. The semi is sent as a token, not a generic symbol. */
		return emitNoData( *yytext );
	}
}

	/* Catch some symbols so they can be 
	 * sent as tokens instead as generic symbols. */
<IL_INITIAL>[*()] {
	return emitNoData( *yytext );
}
<IL_INITIAL>:: {
	return emitNoData( TK_NameSep );
}

	/* Whitespace. */
<IL_INITIAL>{WSCHAR}+ {
	if ( inlineWhitespace )
		return emitToken( IL_WhiteSpace, yytext, yyleng );
}

	/* Any other characters. */
<IL_INITIAL>. {
	return emitToken( IL_Symbol, yytext, 1 );
}

<INITIAL><<EOF>> {
	/* If we are not at the bottom of the include stack, then pop the current
	 * file that we are scanning. Since we are always returning 0 to the parser
	 * it will exit and return to the parser that called it. */
	if ( inc_stack_ptr > 0 )
		popInclude();
	return 0;
}

	/* End of input in a literal is an error. */
<IL_SGL_LIT,IL_DBL_LIT><<EOF>>		{
	error(il_comm_lit_first_line, il_comm_lit_first_column) << 
			"unterminated literal" << endl;
	exit(1);
}
	
	/* End of input in a comment is an error. */
<IL_C_COM,IL_CXX_COM><<EOF>>	{
	error(il_comm_lit_first_line, il_comm_lit_first_column) <<
			"unterminated comment" << endl;
	exit(1);
}

	/* End of intput in a code block. */
<IL_INITIAL><<EOF>> {
	error(il_code_first_line, il_code_first_column) <<
			"unterminated code block" << endl;
	exit(1);
}

%%

/* Write out token data, escaping special charachters. */
#ifdef WANT_TOKEN_WRITE
void writeToken( int token, char *data )
{
	cout << "token id " << token << " at " << id->fileName << ":" <<
			yylloc->first_line << ":" << yylloc->first_column << "-" <<
			yylloc->last_line << ":" << yylloc->last_column << " ";

	if ( data != 0 ) {
		while ( *data != 0 ) {
			switch ( *data ) {
			case '\n':	cout << "\\n"; break;
			case '\t':	cout << "\\t"; break;
			default:	cout << *data; break;
			}
			data += 1;
		}
	}
	cout << endl;
}
#endif

/* Caclulate line info from yytext. Called on every pattern match. */
void updateLineInfo()
{
	/* yytext should always have at least one char. */
	assert( yytext[0] != 0 );

	/* Scan through yytext up to the last character. */
	char *p = yytext;
	for ( ; p[1] != 0; p++ ) {
		if ( p[0] == '\n' ) {
			id->last_line += 1;
			id->last_column = 0;
		}
		else {
			id->last_column += 1;
		}
	}

	/* Always consider the last character as not a newline. Newlines at the
	 * end of a token are as any old character at the end of the line. */
	id->last_column += 1;

	/* The caller may be about to emit a token, be prepared to pass the line
	 * info to the parser. */
	yylloc->first_line = id->first_line;
	yylloc->first_column = id->first_column;
	yylloc->last_line = id->last_line;
	yylloc->last_column = id->last_column;

	/* If the last character was indeed a newline, then wrap ahead now. */
	if ( p[0] == '\n' ) {
		id->last_line += 1;
		id->last_column = 0;
	}
}

/* Eat up a matched pattern that will not be part of a token. */
void garble() 
{
	/* Update line information from yytext. */
	updateLineInfo();

	/* The next token starts ahead of the last token. */
	id->first_line = id->last_line;
	id->first_column = id->last_column + 1;
}

/* Append data to the end of the token. More token data expected. */
void extendToken( char *data, int len )
{
	if ( data != 0 && len > 0 )
		tokbuf.append( data, len );

	/* Update line information from yytext. */
	updateLineInfo();
}

/* Extend, but with no data, more data to come. */
void extendToken() 
{
	/* Update line information from yytext. */
	updateLineInfo();
}


/* Possibly process include data. */
void processInclude( int token )
{
	static char *incFileName = 0;

	if ( handlingInclude ) {
		if ( token == KW_Include )
			incFileName = 0;
		else if ( token == TK_Literal )
			incFileName = yylval->data.data;
		else if ( token == ';' ) {
			/* Terminate the include statement. Start reading from included file. */
			handlingInclude = false;

			if ( id->active && includeDepth < INCLUDE_STACK_SIZE ) {
				/* If there is no section name or input file, default to the curren values. */
				if ( incFileName == 0 )
					incFileName = id->fileName;

				/* Make the new buffer and switch to it. */
				FILE *incFile = fopen( incFileName, "rt" );
				if ( incFile != 0 ) {
					buff_stack[inc_stack_ptr] = YY_CURRENT_BUFFER;
					multiline_stack[inc_stack_ptr] = multiline;
					inc_stack_ptr += 1;
					yy_switch_to_buffer( yy_create_buffer( incFile, YY_BUF_SIZE ) );
					BEGIN(INITIAL);
				}
				else {
					error(*yylloc) << "could not locate include file \"" << incFileName 
							<< "\"" << endl;
				}
			}
		}
	}
}

void popInclude()
{
	/* Free the current buffer and move to the previous. */
	yy_delete_buffer( YY_CURRENT_BUFFER );
	inc_stack_ptr -= 1;
	yy_switch_to_buffer( buff_stack[inc_stack_ptr] );
	multiline = multiline_stack[inc_stack_ptr];

	/* Includes get called only from RL_INITIAL. */
	BEGIN(RL_INITIAL);
}


/* Append data to the end of a token and emitToken it to the parser. */
int emitToken( int token, char *data, int len )
{
	/* Append any new data. */
	if ( data != 0 && len > 0 )
		tokbuf.append( data, len );

	/* Duplicate the buffer. */
	yylval->data.length = tokbuf.length;
	yylval->data.data = new char[tokbuf.length+1];
	memcpy( yylval->data.data, tokbuf.data, tokbuf.length );
	yylval->data.data[tokbuf.length] = 0;

	/* Update line information from yytext. */
	updateLineInfo();

	/* Write token info. */
#ifdef WANT_TOKEN_WRITE
	writeToken( token, tokbuf.data );
#endif

	/* Clear out the buffer. */
	tokbuf.clear();

	/* The next token starts ahead of the last token. */
	id->first_line = id->last_line;
	id->first_column = id->last_column + 1;

	/* Maintain a record of two tokens back. */
	previous_tokens[1] = previous_tokens[0];
	previous_tokens[0] = token;

	/* Possibly process the include statement; */
	processInclude( token );

	return token;
}

/* Emit a token with no data to the parser. */
int emitNoData( int token ) 
{
	/* Return null to the parser. */
	yylval->data.data = 0;
	yylval->data.length = 0;

	/* Update line information from yytext. */
	updateLineInfo();

	/* Write token info. */
#ifdef WANT_TOKEN_WRITE
	writeToken( token, 0 );
#endif

	/* Clear out the buffer. */
	tokbuf.clear();

	/* The next token starts ahead of the last token. */
	id->first_line = id->last_line;
	id->first_column = id->last_column + 1;

	/* Maintain a record of two tokens back. */
	previous_tokens[1] = previous_tokens[0];
	previous_tokens[0] = token;

	/* Possibly process the include statement; */
	processInclude( token );

	return token;
}

/* Pass tokens in outter code through to the output. */
void passThrough( char *data )
{
	/* If no errors and we are at the bottom of the include stack (the source
	 * file listed on the command line) then write out the data. */
	if ( gblErrorCount == 0 && inc_stack_ptr == 0 && 
			machineSpec == 0 && machineName == 0 )
	{
		xmlEscapeHost( *outStream, data );
	}
}

/* Init a buffer. */
Buffer::Buffer() 
:
	data(0), 
	length(0),
	allocated(0)
{
}

/* Empty out a buffer on destruction. */
Buffer::~Buffer()
{
	empty();
}

/* Free the space allocated for the buffer. */
void Buffer::empty()
{
	if ( data != 0 ) {
		free( data );

		data = 0;
		length = 0;
		allocated = 0;
	}
}

/* Grow the buffer when to len allocation. */
void Buffer::upAllocate( int len )
{
	if ( data == 0 )
		data = (char*) malloc( len );
	else
		data = (char*) realloc( data, len );
	allocated = len;
}

int yywrap()
{
	/* Once processessing of the input is done, signal no more. */
	return 1;
}

/* Here simply to suppress the unused yyunpt warning. */
void thisFuncIsNeverCalled()
{
	yyunput(0, 0);
}

/* Put the scannner back into the outside code start state. */
void beginOutsideCode()
{
	BEGIN(INITIAL);
}

/* Determine if we are opening a machine specification block. */
bool openMachineSpecBlock()
{
	if ( previous_tokens[1] == TK_Section && previous_tokens[0] == TK_Word )
		return true;
	else if ( previous_tokens[0] == TK_Section )
		return true;
	return false;
}

/* Wrapper for the lexer which stores the locations of the value and location
 * variables of the parser into globals. The parser is reentrant, however the scanner
 * does not need to be, so globals work fine. This saves us passing them around
 * all the helper functions. */
int yylex( YYSTYPE *yylval, YYLTYPE *yylloc )
{
	::yylval = yylval;
	::yylloc = yylloc;
	return ragel_lex();
}

