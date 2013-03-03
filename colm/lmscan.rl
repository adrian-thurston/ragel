/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <iostream>
#include <fstream>
#include <string.h>

#include "global.h"
#include "lmscan.h"
#include "lmparse.h"
#include "parsedata.h"
#include "avltree.h"
#include "vector.h"

//#define PRINT_TOKENS

using std::ifstream;
using std::istream;
using std::ostream;
using std::cout;
using std::cerr;
using std::endl;

%%{
	machine section_parse;
	alphtype int;
	write data;
}%%

void ColmScanner::sectionParseInit()
{
	%% write init;
}

ostream &ColmScanner::scan_error()
{
	/* Maintain the error count. */
	gblErrorCount += 1;
	cerr << fileName << ":" << line << ":" << column << ": ";
	return cerr;
}

bool ColmScanner::recursiveInclude( const char *inclFileName )
{
	for ( IncludeStack::Iter si = includeStack; si.lte(); si++ ) {
		if ( strcmp( si->fileName, inclFileName ) == 0 )
			return true;
	}
	return false;	
}

void ColmScanner::updateCol()
{
	char *from = lastnl;
	if ( from == 0 )
		from = ts;
	//cerr << "adding " << te - from << " to column" << endl;
	column += te - from;
	lastnl = 0;
}

void ColmScanner::token( int type, char c )
{
	token( type, &c, &c + 1 );
}

void ColmScanner::token( int type )
{
	token( type, 0, 0 );
}

bool isAbsolutePath( const char *path )
{
	return path[0] == '/';
}

ifstream *ColmScanner::tryOpenInclude( char **pathChecks, long &found )
{
	char **check = pathChecks;
	ifstream *inFile = new ifstream;
	
	while ( *check != 0 ) {
		inFile->open( *check );
		if ( inFile->is_open() ) {
			found = check - pathChecks;
			return inFile;
		}
		check += 1;
	}

	found = -1;
	delete inFile;
	return 0;
}

char **ColmScanner::makeIncludePathChecks( const char *thisFileName, const char *fileName )
{
	char **checks = 0;
	long nextCheck = 0;
	char *data = strdup(fileName);
	long length = strlen(fileName);

	/* Absolute path? */
	if ( isAbsolutePath( data ) ) {
		checks = new char*[2];
		checks[nextCheck++] = data;
	}
	else {
		/* Search from the the location of the current file. */
		checks = new char *[2 + includePaths.length()];
		const char *lastSlash = strrchr( thisFileName, '/' );
		if ( lastSlash == 0 )
			checks[nextCheck++] = data;
		else {
			long givenPathLen = (lastSlash - thisFileName) + 1;
			long checklen = givenPathLen + length;
			char *check = new char[checklen+1];
			memcpy( check, thisFileName, givenPathLen );
			memcpy( check+givenPathLen, data, length );
			check[checklen] = 0;
			checks[nextCheck++] = check;
		}

		/* Search from the include paths given on the command line. */
		for ( ArgsVector::Iter incp = includePaths; incp.lte(); incp++ ) {
			long pathLen = strlen( *incp );
			long checkLen = pathLen + 1 + length;
			char *check = new char[checkLen+1];
			memcpy( check, *incp, pathLen );
			check[pathLen] = '/';
			memcpy( check+pathLen+1, data, length );
			check[checkLen] = 0;
			checks[nextCheck++] = check;
		}
	}

	checks[nextCheck] = 0;
	return checks;
}


%%{
	machine section_parse;
	import "lmparse.h";

	action clear_words { word = lit = 0; word_len = lit_len = 0; }
	action store_lit { lit = tokdata; lit_len = toklen; }

	action mach_err { scan_error() << "bad machine statement" << endl; }
	action incl_err { scan_error() << "bad include statement" << endl; }
	action write_err { scan_error() << "bad write statement" << endl; }

	action handle_include
	{
		String src( lit, lit_len );
		String fileName;
		bool unused;

		/* Need a location. */
		InputLoc here;
		here.fileName = fileName;
		here.line = line;
		here.col = column;

		prepareLitString( fileName, unused, src, here );
		char **checks = makeIncludePathChecks( this->fileName, fileName );

		/* Open the input file for reading. */
		long found = 0;
		ifstream *inFile = tryOpenInclude( checks, found );
		if ( inFile == 0 ) {
			scan_error() << "include: could not open " << 
					fileName << " for reading" << endl;
		}
		else {
			/* Only proceed with the include if it was found. */
			if ( recursiveInclude( checks[found] ) )
				scan_error() << "include: this is a recursive include operation" << endl;

			/* Check for a recursive include structure. Add the current file/section
			 * name then check if what we are including is already in the stack. */
			includeStack.append( IncludeStackItem( checks[found] ) );

			ColmScanner *scanner = new ColmScanner( fileName, *inFile, parser, includeDepth+1 );
			scanner->scan();
			delete inFile;

			/* Remove the last element (len-1) */
			includeStack.remove( -1 );

			delete scanner;
		}
	}

	include_target = 
		TK_Literal >clear_words @store_lit;

	include_stmt =
		( KW_Include include_target ) @handle_include
		<>err incl_err <>eof incl_err;

	action handle_token
	{
//	cout << Parser_lelNames[type] << " ";
//	if ( start != 0 ) {
//		cout.write( start, end-start );
//	}
//	cout << endl;

		InputLoc loc;

		#ifdef PRINT_TOKENS
		cerr << "scanner:" << line << ":" << column << 
				": sending token to the parser " << Parser_lelNames[*p];
		cerr << " " << toklen;
		if ( tokdata != 0 )
			cerr << " " << tokdata;
		cerr << endl;
		#endif

		loc.fileName = fileName;
		loc.line = line;
		loc.col = column;

		if ( tokdata != 0 && tokdata[toklen-1] == '\n' )
			loc.line -= 1;

		parser->token( loc, type, tokdata, toklen );
	}

	# Catch everything else.
	everything_else = ^( KW_Include ) @handle_token;

	main := ( 
		include_stmt |
		everything_else
	)*;
}%%

void ColmScanner::token( int type, char *start, char *end )
{
	char *tokdata = 0;
	int toklen = 0;
	int *p = &type;
	int *pe = &type + 1;
	int *eof = 0;

	if ( start != 0 ) {
		toklen = end-start;
		tokdata = new char[toklen+1];
		memcpy( tokdata, start, toklen );
		tokdata[toklen] = 0;
	}

	%%{
		machine section_parse;
		write exec;
	}%%

	updateCol();
}

void ColmScanner::endSection( )
{
	/* Execute the eof actions for the section parser. */
	/* Probably use: token( -1 ); */
}

%%{
	machine lmscan;

	# This is sent by the driver code.
	EOF = 0;
	
	action inc_nl { 
		lastnl = p; 
		column = 0;
		line++;
	}
	NL = '\n' @inc_nl;

	# Identifiers, numbers, commetns, and other common things.
	ident = ( alpha | '_' ) ( alpha |digit |'_' )*;
	number = digit+;
	hex_number = '0x' [0-9a-fA-F]+;

	# These literal forms are common to C-like host code and ragel.
	s_literal = "'" ([^'\\] | NL | '\\' (any | NL))* "'";
	d_literal = '"' ([^"\\] | NL | '\\' (any | NL))* '"';

	whitespace = [ \t] | NL;
	pound_comment = '#' [^\n]* NL;

	or_literal := |*
		# Escape sequences in OR expressions.
		'\\0' => { token( TK_ReChar, '\0' ); };
		'\\a' => { token( TK_ReChar, '\a' ); };
		'\\b' => { token( TK_ReChar, '\b' ); };
		'\\t' => { token( TK_ReChar, '\t' ); };
		'\\n' => { token( TK_ReChar, '\n' ); };
		'\\v' => { token( TK_ReChar, '\v' ); };
		'\\f' => { token( TK_ReChar, '\f' ); };
		'\\r' => { token( TK_ReChar, '\r' ); };
		'\\\n' => { updateCol(); };
		'\\' any => { token( TK_ReChar, ts+1, te ); };

		# Range dash in an OR expression.
		'-' => { token( TK_Dash, 0, 0 ); };

		# Terminate an OR expression.
		']'	=> { token( TK_SqClose ); fret; };

		EOF => {
			scan_error() << "unterminated OR literal" << endl;
		};

		# Characters in an OR expression.
		[^\]] => { token( TK_ReChar, ts, te ); };

	*|;

	regular_type := |*
		# Identifiers.
		ident => { token( TK_Word, ts, te ); } ;

		# Numbers
		number => { token( TK_UInt, ts, te ); };
		hex_number => { token( TK_Hex, ts, te ); };

		# Literals, with optionals.
		( s_literal | d_literal ) [i]? 
			=> { token( TK_Literal, ts, te ); };

		'[' => { token( TK_SqOpen ); fcall or_literal; };
		'[^' => { token( TK_SqOpenNeg ); fcall or_literal; };

		'/' => { token( '/'); fret; };

		# Ignore.
		pound_comment => { updateCol(); };

		'..' => { token( TK_DotDot ); };
		'**' => { token( TK_StarStar ); };
		'--' => { token( TK_DashDash ); };

		':>'  => { token( TK_ColonGt ); };
		':>>' => { token( TK_ColonGtGt ); };
		'<:'  => { token( TK_LtColon ); };

		# Whitespace other than newline.
		[ \t\r]+ => { updateCol(); };

		# If we are in a single line machine then newline may end the spec.
		NL => { updateCol(); };

		# Consume eof.
		EOF;

		any => { token( *ts ); } ;
	*|;

	literal_pattern := |*
		'\\' '0' { litBuf.append( '\0' ); };
		'\\' 'a' { litBuf.append( '\a' ); };
		'\\' 'b' { litBuf.append( '\b' ); };
		'\\' 't' { litBuf.append( '\t' ); };
		'\\' 'n' { litBuf.append( '\n' ); };
		'\\' 'v' { litBuf.append( '\v' ); };
		'\\' 'f' { litBuf.append( '\f' ); };
		'\\' 'r' { litBuf.append( '\r' ); };

		'\\' any {
			litBuf.append( ts[1] );
		};
		'"' => {
			if ( litBuf.length > 0 ) {
				token( TK_LitPat, litBuf.data, litBuf.data+litBuf.length );
				litBuf.clear();
			}
			token( '"' );
			fret;
		};
		NL => {
			litBuf.append( '\n' );
			token( TK_LitPat, litBuf.data, litBuf.data+litBuf.length );
			litBuf.clear();
			token( '"' );
			fret;
		};
		'[' => { 
			if ( litBuf.length > 0 ) {
				token( TK_LitPat, litBuf.data, litBuf.data+litBuf.length );
				litBuf.clear();
			}
			token( '[' );
			fcall main;
		};
		any => { 
			litBuf.append( *ts );
		};
	*|;

	# Parser definitions. 
	main := |*
		'lex' => { token( KW_Lex ); };
		'commit' => { token( KW_Commit ); };
		'token' => { token( KW_Token ); };
		'literal' => { token( KW_Literal ); };
		'rl' => { token( KW_Rl ); };
		'def' => { token( KW_Def ); };
		'ignore' => { token( KW_Ignore ); };
		'construct' => { token( KW_Construct ); };
		'cons' => { token( KW_Construct ); };
		'new' => { token( KW_New ); };
		'if' => { token( KW_If ); };
		'reject' => { token( KW_Reject ); };
		'while' => { token( KW_While ); };
		'else' => { token( KW_Else ); };
		'elsif' => { token( KW_Elsif ); };
		'match' => { token( KW_Match ); };
		'for' => { token( KW_For ); };
		'iter' => { token( KW_Iter ); };
		'prints' => { token( KW_PrintStream ); };
		'print' => { token( KW_Print ); };
		'print_xml_ac' => { token( KW_PrintXMLAC ); };
		'print_xml' => { token( KW_PrintXML ); };
		'namespace' => { token( KW_Namespace ); };
		'lex' => { token( KW_Lex ); };
		'end' => { token( KW_End ); };
		'map' => { token( KW_Map ); };
		'list' => { token( KW_List ); };
		'vector' => { token( KW_Vector ); };
		'accum' => { token( KW_Parser ); };
		'parser' => { token( KW_Parser ); };
		'return' => { token( KW_Return ); };
		'break' => { token( KW_Break ); };
		'yield' => { token( KW_Yield ); };
		'typeid' => { token( KW_TypeId ); };
		'make_token' => { token( KW_MakeToken ); };
		'make_tree' => { token( KW_MakeTree ); };
		'reducefirst' => { token( KW_ReduceFirst ); };
		'for' => { token( KW_For ); };
		'in' => { token( KW_In ); };
		'nil' => { token( KW_Nil ); };
		'true' => { token( KW_True ); };
		'false' => { token( KW_False ); };
		'parse' => { token( KW_Parse ); };
		'parse_stop' => { token( KW_ParseStop ); };
		'global' => { token( KW_Global ); };
		'export' => { token( KW_Export ); };
		'ptr' => { token( KW_Ptr ); };
		'ref' => { token( KW_Ref ); };
		'deref' => { token( KW_Deref ); };
		'require' => { token( KW_Require ); };
		'preeof' => { token( KW_Preeof ); };
		'left' => { token( KW_Left ); };
		'right' => { token( KW_Right ); };
		'nonassoc' => { token( KW_Nonassoc ); };
		'prec' => { token( KW_Prec ); };
		'include' => { token( KW_Include ); };
		'context' => { token( KW_Context ); };
		'alias' => { token( KW_Alias ); };
		'send' => { token( KW_Send ); };
		'ni' => { token( KW_Ni ); };

		# Identifiers.
		ident => { token( TK_Word, ts, te ); } ;

		number => { token( TK_Number, ts, te ); };

		'/' => { 
			token( '/' ); 
			if ( parser->enterRl )
				fcall regular_type;
		};

		"~" [^\n]* NL => { 
			token( '"' );
			token( TK_LitPat, ts+1, te );
			token( '"' );
		};

		"'" ([^'\\\n] | '\\' (any | NL))* ( "'" | NL ) => {
			token( TK_Literal, ts, te );
		};

		'"' => { 
			token( '"' );
			litBuf.clear(); 
			fcall literal_pattern;
		};
		'[' => { 
			token( '[' ); 
			fcall main;
		};

		']' => {
			token( ']' );
			if ( top > 0 )
				fret;
		};

		# Ignore.
		pound_comment => { updateCol(); };

		'=>' => { token( TK_DoubleArrow ); };
		'==' => { token( TK_DoubleEql ); };
		'!=' => { token( TK_NotEql ); };
		'::' => { token( TK_DoubleColon ); };
		'<=' => { token( TK_LessEql ); };
		'>=' => { token( TK_GrtrEql ); };
		'->' => { token( TK_RightArrow ); };
		'&&' => { token( TK_AmpAmp ); };
		'||' => { token( TK_BarBar ); };
		'<<' => { token( TK_LtLt ); };
		
		( '+' | '-' | '*' | '/' | '(' | ')' | '@' | '$' | '^' ) => { token( *ts ); };


		# Whitespace other than newline.
		[ \t\r]+ => { updateCol(); };
		NL => { updateCol(); };

		# Consume eof.
		EOF;

		any => { token( *ts ); } ;
	*|;
}%%

%% write data;

void ColmScanner::scan()
{
	int bufsize = 8;
	char *buf = new char[bufsize];
	const char last_char = 0;
	int cs, act, have = 0;
	int top, stack[32];
	bool execute = true;

	sectionParseInit();
	%% write init;

	while ( execute ) {
		char *p = buf + have;
		int space = bufsize - have;

		if ( space == 0 ) {
			/* We filled up the buffer trying to scan a token. Grow it. */
			bufsize = bufsize * 2;
			char *newbuf = new char[bufsize];

			/* Recompute p and space. */
			p = newbuf + have;
			space = bufsize - have;

			/* Patch up pointers possibly in use. */
			if ( ts != 0 )
				ts = newbuf + ( ts - buf );
			te = newbuf + ( te - buf );

			/* Copy the new buffer in. */
			memcpy( newbuf, buf, have );
			delete[] buf;
			buf = newbuf;
		}

		input.read( p, space );
		int len = input.gcount();

		/* If we see eof then append the EOF char. */
	 	if ( len == 0 ) {
			p[0] = last_char, len = 1;
			execute = false;
		}

		char *pe = p + len;
		char *eof = 0;
		%% write exec;

		/* Check if we failed. */
		if ( cs == lmscan_error ) {
			/* Machine failed before finding a token. I'm not yet sure if this
			 * is reachable. */
			scan_error() << "colm scanner error (metalanguage)" << endl;
			exit(1);
		}

		/* Decide if we need to preserve anything. */
		char *preserve = ts;

		/* Now set up the prefix. */
		if ( preserve == 0 )
			have = 0;
		else {
			/* There is data that needs to be shifted over. */
			have = pe - preserve;
			memmove( buf, preserve, have );
			unsigned int shiftback = preserve - buf;
			if ( ts != 0 )
				ts -= shiftback;
			te -= shiftback;

			preserve = buf;
		}
	}
	delete[] buf;
}

void ColmScanner::eof()
{
	InputLoc loc;
	loc.fileName = "<EOF>";
	loc.line = line;
	loc.col = 1;
	parser->token( loc, ColmParser_tk_eof, 0, 0 );
}
