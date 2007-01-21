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
#include "vector.h"
#include "rlcodegen.h"
#include "xmlparse.h"
#include "buffer.h"

using std::cout;
using std::cerr;
using std::endl;

Buffer tokbuf;
int builtinBrace = 0;
bool inlineWhitespace = true;
bool handlingInclude = false;

YYSTYPE *yylval;
YYLTYPE *yylloc;

void garble();

void extendToken();
void extendToken( char *data, int len );

int emitToken( int token, char *data, int len );
int emitNoData( int token );
int emitTag( char *data, int len, bool isOpen );
void passThrough( char *data );
void popInclude();
void scannerInit();

enum InlineBlockType {
	CurlyDelimited,
	SemiTerminated
} inlineBlockType;

/* Using a wrapper for the parser, must the lex declaration. */
#define YY_DECL int rlcodegen_lex()

class Perfect_Hash
{
private:
    static inline unsigned int hash (const char *str, unsigned int len);

public:
    static struct XMLTagHashPair *in_word_set (const char *str, unsigned int len);
};

Vector<bool> shouldEmitXMLData;

int first_line = 1;
int first_column = 1;
int last_line = 1;
int last_column = 0;

Buffer xmlData;

%}

%x OPEN_TAG
%x CLOSE_TAG1
%x CLOSE_TAG2
%x ATTR_LIST
%x ATTR_LITERAL

WSCHAR [\t\n\v\f\r ]
IDENT [a-zA-Z_][a-zA-Z_0-9\-]*

%%

	/* Numbers in outter code. */
<INITIAL>"<" {
	BEGIN(OPEN_TAG);
	shouldEmitXMLData.prepend( false );
	return emitNoData( *yytext );
}

<INITIAL>[^<&]+ {
	if ( shouldEmitXMLData[0] )
		xmlData.append( yytext, yyleng );
	garble();
}
<INITIAL>"&amp;" {
	if ( shouldEmitXMLData[0] )
		xmlData.append( "&", 1 );
	garble();
}
<INITIAL>"&lt;" {
	if ( shouldEmitXMLData[0] )
		xmlData.append( "<", 1 );
	garble();
}
<INITIAL>"&gt;" {
	if ( shouldEmitXMLData[0] )
		xmlData.append( ">", 1 );
	garble();
}

	/* 
	 * Tags
	 */

<OPEN_TAG>"/" {
	BEGIN(CLOSE_TAG1);
	xmlData.append(0);
	return emitNoData( *yytext );
}

<OPEN_TAG>{IDENT} {
	BEGIN( ATTR_LIST );
	return emitTag( yytext, yyleng, true );
}

<OPEN_TAG,CLOSE_TAG1>{WSCHAR}+ {
	garble();
}

<CLOSE_TAG1>{IDENT} {
	BEGIN( CLOSE_TAG2 );
	return emitTag( yytext, yyleng, false );
}

<CLOSE_TAG2>">" {
	shouldEmitXMLData.remove( 0 );
	BEGIN(INITIAL);
	return emitNoData( *yytext );
}

<ATTR_LIST>{IDENT} {
	return emitToken( XML_Word, yytext, yyleng );
}

<ATTR_LIST>\" {
	BEGIN(ATTR_LITERAL);
	extendToken();
}
<ATTR_LITERAL>\\.				extendToken( yytext+1, 1 );
<ATTR_LITERAL>\\\n				extendToken( yytext+1, 1 );
<ATTR_LITERAL>[^\\"]+			extendToken( yytext, yyleng );

	/* Terminate a double literal */
<ATTR_LITERAL>\" {
	BEGIN(ATTR_LIST);
	return emitToken( XML_Literal, 0, 0 );
}

<ATTR_LIST>{WSCHAR}+ {
	garble();
}

<ATTR_LIST>">" {
	BEGIN(INITIAL);
	return emitNoData( *yytext );
}

<ATTR_LIST>. {
	return emitNoData( *yytext );
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
	/* yytext should always have at least wone char. */
	assert( yytext[0] != 0 );

	/* Scan through yytext up to the last character. */
	char *p = yytext;
	for ( ; p[1] != 0; p++ ) {
		if ( p[0] == '\n' ) {
			last_line += 1;
			last_column = 0;
		}
		else {
			last_column += 1;
		}
	}

	/* Always consider the last character as not a newline. Newlines at the
	 * end of a token are as any old character at the end of the line. */
	last_column += 1;

	/* The caller may be about to emit a token, be prepared to pass the line
	 * info to the parser. */
	yylloc->first_line = first_line;
	yylloc->first_column = first_column;
	yylloc->last_line = last_line;
	yylloc->last_column = last_column;

	/* If the last character was indeed a newline, then wrap ahead now. */
	if ( p[0] == '\n' ) {
		last_line += 1;
		last_column = 0;
	}
}


/* Eat up a matched pattern that will not be part of a token. */
void garble() 
{
	/* Update line information from yytext. */
	updateLineInfo();

	/* The next token starts ahead of the last token. */
	first_line = last_line;
	first_column = last_column + 1;
}

/* Extend a token, but don't add any data to it, more token data expected. */
void extendToken() 
{
	/* Update line information from yytext. */
	updateLineInfo();
}

/* Append data to the end of the token. More token data expected. */
void extendToken( char *data, int len )
{
	if ( data != 0 && len > 0 )
		tokbuf.append( data, len );

	/* Update line information from yytext. */
	updateLineInfo();
}


/* Append data to the end of a token and emitToken it to the parser. */
int emitToken( int token, char *data, int len )
{
	/* Append the data and null terminate. */
	if ( data != 0 && len > 0 )
		tokbuf.append( data, len );
	tokbuf.append( 0 );

	/* Duplicate the buffer. */
	yylval->data = new char[tokbuf.length];
	strcpy( yylval->data, tokbuf.data );

	/* Update line information from yytext. */
	updateLineInfo();

	/* Write token info. */
#ifdef WANT_TOKEN_WRITE
	writeToken( token, tokbuf.data );
#endif

	/* Clear out the buffer. */
	tokbuf.clear();

	/* The next token starts ahead of the last token. */
	first_line = last_line;
	first_column = last_column + 1;

	return token;
}

/* Append data to the end of a token and emitToken it to the parser. */
int emitTag( char *data, int len, bool isOpen )
{
	/* Lookup the tag. */
	int token = TAG_unknown;

	XMLTagHashPair *tag = Perfect_Hash::in_word_set( data, len );
	if ( tag != 0 )
		token = tag->id;

	if ( isOpen ) {
		switch ( token ) {
		case TAG_host: case TAG_t: case TAG_start_state:
		case TAG_action_table: 
		case TAG_alphtype: case TAG_state_actions: 
		case TAG_entry_points:
		case TAG_text: case TAG_goto: 
		case TAG_call: case TAG_next:
		case TAG_set_act: case TAG_set_tokend:
		case TAG_entry: case TAG_option:
		case TAG_cond_space: case TAG_c:
			shouldEmitXMLData[0] = true;
			xmlData.clear();
		}
	}

	return emitToken( token, data, len );
}

/* Emit a token with no data to the parser. */
int emitNoData( int token ) 
{
	/* Return null to the parser. */
	yylval->data = 0;

	/* Update line information from yytext. */
	updateLineInfo();

	/* Write token info. */
#ifdef WANT_TOKEN_WRITE
	writeToken( token, 0 );
#endif

	/* Clear out the buffer. */
	tokbuf.clear();

	/* The next token starts ahead of the last token. */
	first_line = last_line;
	first_column = last_column + 1;

	return token;
}

/* Pass tokens in outter code through to the output. */
void passThrough( char *data )
{
	/* If no errors, we are emitting code and we are at the bottom of the
	 * include stack (the source file listed on the command line) then write
	 * out the data. */
	if ( gblErrorCount == 0 && outputFormat == OutCode )
		*outStream << data;
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

void scannerInit()
{
	/* Set this up in case we are initially given something other
	 * than an opening tag. */
	shouldEmitXMLData.prepend( false );
}

/* Wrapper for the lexer which stores the locations of the value and location
 * variables of the parser into globals. The parser is reentrant, however the scanner
 * does not need to be, so globals work fine. This saves us passing them around
 * all the helper functions. */
int yylex( YYSTYPE *yylval, YYLTYPE *yylloc )
{
	::yylval = yylval;
	::yylloc = yylloc;
	return rlcodegen_lex();
}
