/*
 *  Copyright 2006-2007 Adrian Thurston <thurston@cs.queensu.ca>
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

#include <iostream>
#include <fstream>
#include <string.h>

#include "ragel.h"
#include "rlparse.h"
#include "parsedata.h"
#include "avltree.h"
#include "vector.h"


using std::ifstream;
using std::istream;
using std::ostream;
using std::cout;
using std::cerr;
using std::endl;

/* This is used for tracking the current stack of include file/machine pairs. It is
 * is used to detect and recursive include structure. */
struct IncludeStackItem
{
	IncludeStackItem( char *fileName, char *sectionName )
		: fileName(fileName), sectionName(sectionName) {}

	char *fileName;
	char *sectionName;
};

typedef Vector<IncludeStackItem> IncludeStack;

enum InlineBlockType
{
	CurlyDelimited,
	SemiTerminated
};

struct Scanner
{
	Scanner( char *fileName, istream &input, ostream &output,
			Parser *inclToParser, char *inclSectionTarg,
			int includeDepth )
	: 
		fileName(fileName), input(input), output(output),
		inclToParser(inclToParser),
		inclSectionTarg(inclSectionTarg),
		includeDepth(includeDepth),
		line(1), column(1), lastnl(0), 
		parser(0), active(false), 
		parserExistsError(false),
		whitespaceOn(true)
		{}

	bool recursiveInclude( char *inclFileName, char *inclSectionName );

	char *prepareFileName( char *fileName, int len )
	{
		bool caseInsensitive;
		Token tokenFnStr, tokenRes;
		tokenFnStr.data = fileName;
		tokenFnStr.length = len;
		tokenFnStr.prepareLitString( tokenRes, caseInsensitive );
		return tokenRes.data;
	}

	void init();
	void token( int type, char *start, char *end );
	void token( int type, char c );
	void token( int type );
	void updateCol();
	void startSection();
	void endSection();
	void do_scan();
	bool parserExists();
	ostream &scan_error();

	char *fileName;
	istream &input;
	ostream &output;
	Parser *inclToParser;
	char *inclSectionTarg;
	int includeDepth;

	int cs;
	int line;
	char *word, *lit;
	int word_len, lit_len;
	InputLoc sectionLoc;
	char *tokstart, *tokend;
	int column;
	char *lastnl;

	/* Set by machine statements, these persist from section to section
	 * allowing for unnamed sections. */
	Parser *parser;
	bool active;
	IncludeStack includeStack;

	/* This is set if ragel has already emitted an error stating that
	 * no section name has been seen and thus no parser exists. */
	bool parserExistsError;

	/* This is for inline code. By default it is on. It goes off for
	 * statements and values in inline blocks which are parsed. */
	bool whitespaceOn;
};

%%{
	machine section_parse;
	alphtype int;
	write data;
}%%

void Scanner::init( )
{
	%% write init;
}

bool Scanner::parserExists()
{
	if ( parser != 0 )
		return true;

	if ( ! parserExistsError ) {
		scan_error() << "include: there is no previous specification name" << endl;
		parserExistsError = true;
	}
	return false;
}

ostream &Scanner::scan_error()
{
	/* Maintain the error count. */
	gblErrorCount += 1;
	cerr << fileName << ":" << line << ":" << column << ": ";
	return cerr;
}

bool Scanner::recursiveInclude( char *inclFileName, char *inclSectionName )
{
	for ( IncludeStack::Iter si = includeStack; si.lte(); si++ ) {
		if ( strcmp( si->fileName, inclFileName ) == 0 &&
				strcmp( si->sectionName, inclSectionName ) == 0 )
		{
			return true;
		}
	}
	return false;	
}

void Scanner::updateCol()
{
	char *from = lastnl;
	if ( from == 0 )
		from = tokstart;
	//cerr << "adding " << tokend - from << " to column" << endl;
	column += tokend - from;
	lastnl = 0;
}

void Scanner::token( int type, char c )
{
	token( type, &c, &c + 1 );
}

void Scanner::token( int type )
{
	token( type, 0, 0 );
}

%%{
	machine section_parse;

	# This relies on the the kelbt implementation and the order
	# that tokens are declared.
	KW_Machine = 128;
	KW_Include = 129;
	KW_Write = 130;
	TK_Word = 131;
	TK_Literal = 132;

	action clear_words { word = lit = 0; word_len = lit_len = 0; }
	action store_word { word = tokdata; word_len = toklen; }
	action store_lit { lit = tokdata; lit_len = toklen; }

	action mach_err { scan_error() << "bad machine statement" << endl; }
	action incl_err { scan_error() << "bad include statement" << endl; }
	action write_err { scan_error() << "bad write statement" << endl; }

	action handle_machine
	{
		/* Assign a name to the machine. */
		char *machine = word;

		if ( inclSectionTarg == 0 ) {
			active = true;

			ParserDictEl *pdEl = parserDict.find( machine );
			if ( pdEl == 0 ) {
				pdEl = new ParserDictEl( machine );
				pdEl->value = new Parser( fileName, machine, sectionLoc );
				pdEl->value->init();
				parserDict.insert( pdEl );
			}

			parser = pdEl->value;
		}
		else if ( strcmp( inclSectionTarg, machine ) == 0 ) {
			/* found include target */
			active = true;
			parser = inclToParser;
		}
		else {
			/* ignoring section */
			active = false;
			parser = 0;
		}
	}

	machine_stmt =
		( KW_Machine TK_Word @store_word ';' ) @handle_machine
		<>err mach_err <>eof mach_err;

	action handle_include
	{
		if ( active && parserExists() ) {
			char *inclSectionName = word;
			char *inclFileName = 0;

			/* Implement defaults for the input file and section name. */
			if ( inclSectionName == 0 )
				inclSectionName = parser->sectionName;

			if ( lit != 0 ) 
				inclFileName = prepareFileName( lit, lit_len );
			else
				inclFileName = fileName;

			/* Check for a recursive include structure. Add the current file/section
			 * name then check if what we are including is already in the stack. */
			includeStack.append( IncludeStackItem( fileName, parser->sectionName ) );

			if ( recursiveInclude( inclFileName, inclSectionName ) )
				scan_error() << "include: this is a recursive include operation" << endl;
			else {
				/* Open the input file for reading. */
				ifstream *inFile = new ifstream( inclFileName );
				if ( ! inFile->is_open() ) {
					scan_error() << "include: could not open " << 
							inclFileName << " for reading" << endl;
				}

				Scanner scanner( inclFileName, *inFile, output, parser,
						inclSectionName, includeDepth+1 );
				scanner.init();
				scanner.do_scan( );
				delete inFile;
			}

			/* Remove the last element (len-1) */
			includeStack.remove( -1 );
		}
	}

	include_names = (
		TK_Word @store_word ( TK_Literal @store_lit )? |
		TK_Literal @store_lit
	) >clear_words;

	include_stmt =
		( KW_Include include_names ';' ) @handle_include
		<>err incl_err <>eof incl_err;

	action write_command
	{
		if ( active && machineSpec == 0 && machineName == 0 ) {
			output << "<write"
					" def_name=\"" << parser->sectionName << "\""
					" line=\"" << line << "\""
					" col=\"" << column << "\""
					">";
		}
	}

	action write_arg
	{
		if ( active && machineSpec == 0 && machineName == 0 )
			output << "<arg>" << tokdata << "</arg>";
	}

	action write_close
	{
		if ( active && machineSpec == 0 && machineName == 0 )
			output << "</write>\n";
	}

	write_stmt =
		( KW_Write @write_command 
		( TK_Word @write_arg )+ ';' @write_close )
		<>err write_err <>eof write_err;

	action handle_token
	{
		/* Send the token off to the parser. */
		if ( active && parserExists() ) {
			InputLoc loc;

			#if 0
			cerr << "scanner:" << line << ":" << column << 
					": sending token to the parser " << lelNames[*p];
			cerr << " " << toklen;
			if ( tokdata != 0 )
				cerr << " " << tokdata;
			cerr << endl;
			#endif

			loc.fileName = fileName;
			loc.line = line;
			loc.col = column;

			parser->token( loc, type, tokdata, toklen );
		}
	}

	# Catch everything else.
	everything_else = ^( KW_Machine | KW_Include | KW_Write ) @handle_token;

	main := ( 
		machine_stmt |
		include_stmt |
		write_stmt |
		everything_else
	)*;
}%%

void Scanner::token( int type, char *start, char *end )
{
	char *tokdata = 0;
	int toklen = 0;
	int *p = &type;
	int *pe = &type + 1;

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

void Scanner::startSection( )
{
	parserExistsError = false;

	if ( includeDepth == 0 ) {
		if ( machineSpec == 0 && machineName == 0 )
			output << "</host>\n";
	}

	sectionLoc.fileName = fileName;
	sectionLoc.line = line;
	sectionLoc.col = 0;
}

void Scanner::endSection( )
{
	/* Execute the eof actions for the section parser. */
	%%{
		machine section_parse;
		write eof;
	}%%

	/* Close off the section with the parser. */
	if ( active && parserExists() ) {
		InputLoc loc;
		loc.fileName = fileName;
		loc.line = line;
		loc.col = 0;

		parser->token( loc, TK_EndSection, 0, 0 );
	}

	if ( includeDepth == 0 ) {
		if ( machineSpec == 0 && machineName == 0 ) {
			/* The end section may include a newline on the end, so
			 * we use the last line, which will count the newline. */
			output << "<host line=\"" << line << "\">";
		}
	}
}

%%{
	machine rlscan;

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

	c_comment = 
		'/*' ( any | NL )* :>> '*/';

	cpp_comment =
		'//' [^\n]* NL;

	c_cpp_comment = c_comment | cpp_comment;

	# These literal forms are common to C-like host code and ragel.
	s_literal = "'" ([^'\\] | NL | '\\' (any | NL))* "'";
	d_literal = '"' ([^"\\] | NL | '\\' (any | NL))* '"';

	whitespace = [ \t] | NL;
	pound_comment = '#' [^\n]* NL;

	# An inline block of code. This is specified as a scanned, but is sent to
	# the parser as one long block. The inline_block pointer is used to handle
	# the preservation of the data.
	inline_code := |*
		# Inline expression keywords.
		"fpc" => { token( KW_PChar ); };
		"fc" => { token( KW_Char ); };
		"fcurs" => { token( KW_CurState ); };
		"ftargs" => { token( KW_TargState ); };
		"fentry" => { 
			whitespaceOn = false; 
			token( KW_Entry );
		};

		# Inline statement keywords.
		"fhold" => { 
			whitespaceOn = false; 
			token( KW_Hold );
		};
		"fexec" => { token( KW_Exec, 0, 0 ); };
		"fgoto" => { 
			whitespaceOn = false; 
			token( KW_Goto );
		};
		"fnext" => { 
			whitespaceOn = false; 
			token( KW_Next );
		};
		"fcall" => { 
			whitespaceOn = false; 
			token( KW_Call );
		};
		"fret" => { 
			whitespaceOn = false; 
			token( KW_Ret );
		};
		"fbreak" => { 
			whitespaceOn = false; 
			token( KW_Break );
		};

		ident => { token( TK_Word, tokstart, tokend ); };

		number => { token( TK_UInt, tokstart, tokend ); };
		hex_number => { token( TK_Hex, tokstart, tokend ); };

		( s_literal | d_literal ) 
			=> { token( IL_Literal, tokstart, tokend ); };

		whitespace+ => { 
			if ( whitespaceOn ) 
				token( IL_WhiteSpace, tokstart, tokend );
		};
		c_cpp_comment => { token( IL_Comment, tokstart, tokend ); };

		"::" => { token( TK_NameSep, tokstart, tokend ); };

		# Some symbols need to go to the parser as with their cardinal value as
		# the token type (as opposed to being sent as anonymous symbols)
		# because they are part of the sequences which we interpret. The * ) ;
		# symbols cause whitespace parsing to come back on. This gets turned
		# off by some keywords.

		";" => {
			whitespaceOn = true;
			token( *tokstart, tokstart, tokend );
			if ( inlineBlockType == SemiTerminated )
				fgoto parser_def;
		};

		[*)] => { 
			whitespaceOn = true;
			token( *tokstart, tokstart, tokend );
		};

		[,(] => { token( *tokstart, tokstart, tokend ); };

		'{' => { 
			token( IL_Symbol, tokstart, tokend );
			curly_count += 1; 
		};

		'}' => { 
			if ( --curly_count == 0 && inlineBlockType == CurlyDelimited ) {
				/* Inline code block ends. */
				token( '}' );
				fgoto parser_def;
			}
			else {
				/* Either a semi terminated inline block or only the closing
				 * brace of some inner scope, not the block's closing brace. */
				token( IL_Symbol, tokstart, tokend );
			}
		};

		EOF => {
			scan_error() << "unterminated code block" << endl;
		};

		# Send every other character as a symbol.
		any => { token( IL_Symbol, tokstart, tokend ); };
	*|;

	or_literal := |*
		# Escape sequences in OR expressions.
		'\\0' => { token( RE_Char, '\0' ); };
		'\\a' => { token( RE_Char, '\a' ); };
		'\\b' => { token( RE_Char, '\b' ); };
		'\\t' => { token( RE_Char, '\t' ); };
		'\\n' => { token( RE_Char, '\n' ); };
		'\\v' => { token( RE_Char, '\v' ); };
		'\\f' => { token( RE_Char, '\f' ); };
		'\\r' => { token( RE_Char, '\r' ); };
		'\\\n' => { updateCol(); };
		'\\' any => { token( RE_Char, tokstart+1, tokend ); };

		# Range dash in an OR expression.
		'-' => { token( RE_Dash, 0, 0 ); };

		# Terminate an OR expression.
		']'	=> { token( RE_SqClose ); fret; };

		EOF => {
			scan_error() << "unterminated OR literal" << endl;
		};

		# Characters in an OR expression.
		[^\]] => { token( RE_Char, tokstart, tokend ); };

	*|;

	re_literal := |*
		# Escape sequences in regular expressions.
		'\\0' => { token( RE_Char, '\0' ); };
		'\\a' => { token( RE_Char, '\a' ); };
		'\\b' => { token( RE_Char, '\b' ); };
		'\\t' => { token( RE_Char, '\t' ); };
		'\\n' => { token( RE_Char, '\n' ); };
		'\\v' => { token( RE_Char, '\v' ); };
		'\\f' => { token( RE_Char, '\f' ); };
		'\\r' => { token( RE_Char, '\r' ); };
		'\\\n' => { updateCol(); };
		'\\' any => { token( RE_Char, tokstart+1, tokend ); };

		# Terminate an OR expression.
		'/' [i]? => { 
			token( RE_Slash, tokstart, tokend ); 
			fgoto parser_def;
		};

		# Special characters.
		'.' => { token( RE_Dot ); };
		'*' => { token( RE_Star ); };

		'[' => { token( RE_SqOpen ); fcall or_literal; };
		'[^' => { token( RE_SqOpenNeg ); fcall or_literal; };

		EOF => {
			scan_error() << "unterminated regular expression" << endl;
		};

		# Characters in an OR expression.
		[^\/] => { token( RE_Char, tokstart, tokend ); };
	*|;

	# We need a separate token space here to avoid the ragel keywords.
	write_statement := |*
		ident => { token( TK_Word, tokstart, tokend ); } ;
		[ \t\n]+ => { updateCol(); };
		';' => { token( ';' ); fgoto parser_def; };

		EOF => {
			scan_error() << "unterminated write statement" << endl;
		};
	*|;

	# Parser definitions. 
	parser_def := |*
		'machine' => { token( KW_Machine ); };
		'include' => { token( KW_Include ); };
		'write' => { 
			token( KW_Write );
			fgoto write_statement;
		};
		'action' => { token( KW_Action ); };
		'alphtype' => { token( KW_AlphType ); };

		# FIXME: Enable this post 5.17.
		# 'range' => { token( KW_Range ); };

		'getkey' => { 
			token( KW_GetKey );
			inlineBlockType = SemiTerminated;
			fgoto inline_code;
		};
		'access' => { 
			token( KW_Access );
			inlineBlockType = SemiTerminated;
			fgoto inline_code;
		};
		'variable' => { 
			token( KW_Variable );
			inlineBlockType = SemiTerminated;
			fgoto inline_code;
		};
		'when' => { token( KW_When ); };
		'eof' => { token( KW_Eof ); };
		'err' => { token( KW_Err ); };
		'lerr' => { token( KW_Lerr ); };
		'to' => { token( KW_To ); };
		'from' => { token( KW_From ); };

		# Identifiers.
		ident => { token( TK_Word, tokstart, tokend ); } ;

		# Numbers
		number => { token( TK_UInt, tokstart, tokend ); };
		hex_number => { token( TK_Hex, tokstart, tokend ); };

		# Literals, with optionals.
		( s_literal | d_literal ) [i]? 
			=> { token( TK_Literal, tokstart, tokend ); };

		'[' => { token( RE_SqOpen ); fcall or_literal; };
		'[^' => { token( RE_SqOpenNeg ); fcall or_literal; };

		'/' => { token( RE_Slash ); fgoto re_literal; };

		# Ignore.
		pound_comment => { updateCol(); };

		':=' => { token( TK_ColonEquals ); };

		# To State Actions.
		">~" => { token( TK_StartToState ); };
		"$~" => { token( TK_AllToState ); };
		"%~" => { token( TK_FinalToState ); };
		"<~" => { token( TK_NotStartToState ); };
		"@~" => { token( TK_NotFinalToState ); };
		"<>~" => { token( TK_MiddleToState ); };

		# From State actions
		">*" => { token( TK_StartFromState ); };
		"$*" => { token( TK_AllFromState ); };
		"%*" => { token( TK_FinalFromState ); };
		"<*" => { token( TK_NotStartFromState ); };
		"@*" => { token( TK_NotFinalFromState ); };
		"<>*" => { token( TK_MiddleFromState ); };

		# EOF Actions.
		">/" => { token( TK_StartEOF ); };
		"$/" => { token( TK_AllEOF ); };
		"%/" => { token( TK_FinalEOF ); };
		"</" => { token( TK_NotStartEOF ); };
		"@/" => { token( TK_NotFinalEOF ); };
		"<>/" => { token( TK_MiddleEOF ); };

		# Global Error actions.
		">!" => { token( TK_StartGblError ); };
		"$!" => { token( TK_AllGblError ); };
		"%!" => { token( TK_FinalGblError ); };
		"<!" => { token( TK_NotStartGblError ); };
		"@!" => { token( TK_NotFinalGblError ); };
		"<>!" => { token( TK_MiddleGblError ); };

		# Local error actions.
		">^" => { token( TK_StartLocalError ); };
		"$^" => { token( TK_AllLocalError ); };
		"%^" => { token( TK_FinalLocalError ); };
		"<^" => { token( TK_NotStartLocalError ); };
		"@^" => { token( TK_NotFinalLocalError ); };
		"<>^" => { token( TK_MiddleLocalError ); };

		# Middle.
		"<>" => { token( TK_Middle ); };

		# Conditions. 
		'>?' => { token( TK_StartCond ); };
		'$?' => { token( TK_AllCond ); };
		'%?' => { token( TK_LeavingCond ); };

		'..' => { token( TK_DotDot ); };
		'**' => { token( TK_StarStar ); };
		'--' => { token( TK_DashDash ); };
		'->' => { token( TK_Arrow ); };
		'=>' => { token( TK_DoubleArrow ); };

		":>"  => { token( TK_ColonGt ); };
		":>>" => { token( TK_ColonGtGt ); };
		"<:"  => { token( TK_LtColon ); };

		# Opening of longest match.
		"|*" => { token( TK_BarStar ); };

		'}%%' => { 
			updateCol();
			endSection();
			fgoto main;
		};

		[ \t]+ => { updateCol(); };

		# If we are in a single line machine then newline may end the spec.
		NL => {
			updateCol();
			if ( singleLineSpec ) {
				endSection();
				fgoto main;
			}
		};

		'{' => { 
			token( '{' );
			curly_count = 1; 
			inlineBlockType = CurlyDelimited;
			fgoto inline_code;
		};

		EOF => {
			scan_error() << "unterminated ragel section" << endl;
		};

		any => { token( *tokstart ); } ;
	*|;

	action pass {
		updateCol();

		/* If no errors and we are at the bottom of the include stack (the
		 * source file listed on the command line) then write out the data. */
		if ( includeDepth == 0 && machineSpec == 0 && machineName == 0 )
			xmlEscapeHost( output, tokstart, tokend-tokstart );
	}

	# Outside code scanner. These tokens get passed through.
	main := |*
		ident => pass;
		number => pass;
		c_cpp_comment => pass;
		s_literal | d_literal => pass;
		'%%{' => { 
			updateCol();
			singleLineSpec = false;
			startSection();
			fgoto parser_def;
		};
		'%%' => { 
			updateCol();
			singleLineSpec = true;
			startSection();
			fgoto parser_def;
		};
		whitespace+ => pass;
		EOF;
		any => pass;
	*|;

}%%

%% write data;

void Scanner::do_scan()
{
	int bufsize = 8;
	char *buf = new char[bufsize];
	const char last_char = 0;
	int cs, act, have = 0;
	int top, stack[1];
	int curly_count = 0;
	bool execute = true;
	bool singleLineSpec = false;
	InlineBlockType inlineBlockType;

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
			if ( tokstart != 0 )
				tokstart = newbuf + ( tokstart - buf );
			tokend = newbuf + ( tokend - buf );

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
		%% write exec;

		/* Check if we failed. */
		if ( cs == rlscan_error ) {
			/* Machine failed before finding a token. I'm not yet sure if this
			 * is reachable. */
			scan_error() << "scanner error" << endl;
			exit(1);
		}

		/* Decide if we need to preserve anything. */
		char *preserve = tokstart;

		/* Now set up the prefix. */
		if ( preserve == 0 )
			have = 0;
		else {
			/* There is data that needs to be shifted over. */
			have = pe - preserve;
			memmove( buf, preserve, have );
			unsigned int shiftback = preserve - buf;
			if ( tokstart != 0 )
				tokstart -= shiftback;
			tokend -= shiftback;

			preserve = buf;
		}
	}

	delete[] buf;
}

void scan( char *fileName, istream &input, ostream &output )
{
	Scanner scanner( fileName, input, output, 0, 0, 0 );
	scanner.init();
	scanner.do_scan();

	InputLoc eofLoc;
	eofLoc.fileName = fileName;
	eofLoc.col = 1;
	eofLoc.line = scanner.line;
}
